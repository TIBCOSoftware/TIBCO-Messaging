/*
 * Copyright (c) 2009-$Date: 2018-09-04 17:57:51 -0500 (Tue, 04 Sep 2018) $ TIBCO Software Inc.
 * Licensed under a BSD-style license. Refer to [LICENSE]
 * For more information, please contact:
 * TIBCO Software Inc., Palo Alto, California, USA
 *
 * $Id: WebSocketConnection.cs 103512 2018-09-04 22:57:51Z bpeterse $
 */
using System;
using System.Collections;
using System.Collections.Concurrent;
using System.Linq;
using System.Threading;
using System.Threading.Tasks;
using System.Timers;
using System.Net;
using System.Net.Security;

namespace TIBCO.EFTL 
{
    public class WebSocketConnection : TIBCO.EFTL.IConnection, TIBCO.EFTL.WebSocketListener, IDisposable 
    {
        Uri                 uri;
        IConnectionListener listener;
        long                connecting;
        long                connected;
        long                reconnecting;
        long                messageIdGenerator;
        long                subscriptionIdGenerator;
        protected String    clientId;
        protected String    reconnectId = null;
        WebSocket           webSocket;

        Hashtable           props;
        protected Object    writeLock = new Object();
        protected Object    processLock = new Object();
        protected bool      qos;
        protected long      maxMessageSize;
        protected int       reconnectAttempts = 0;
        protected long      lastSequenceNumber;
        protected Thread    writer;
        protected System.Timers.Timer     timer;

        BlockingCollection<RequestContext> writeQueue = new BlockingCollection<RequestContext>();
        ConcurrentDictionary<String, BasicSubscription> subscriptions = new ConcurrentDictionary<String, BasicSubscription>();
        internal SkipList<RequestContext> requests = new SkipList<RequestContext>();

        private static readonly RequestContext DISCONNECT = new RequestContext("");
        private static readonly int defaultAutoReconnectAttempts = 5;
        private static readonly int defaultAutoReconnectMaxDelay = 30000;

        public WebSocketConnection(String uri, IConnectionListener listener) 
        {
            try 
            {
                this.uri = new Uri(uri);
                this.listener = listener;
                this.props = new Hashtable();

                props.Add(ProtocolConstants.QOS_FIELD, "true");

                if (String.Compare("wss", this.uri.Scheme, true) == 0) 
                {
                    ServicePointManager.SecurityProtocol = SecurityProtocolType.Tls12;
                    ServicePointManager.MaxServicePointIdleTime = 1000;
                }
            } 
            catch(Exception) 
            {
                throw new System.ArgumentException(uri);
            }
        }

        private String getHost() 
        {
            return uri.Host;
        }

        public void Connect(System.Collections.Hashtable p) 
        {
            if (Interlocked.CompareExchange(ref connecting, 1, 0) == 0) 
            {
                if (p != null) 
                {
                    foreach (String key in p.Keys) 
                    {
                        if (!props.ContainsKey(key))
                            this.props.Add(key, p[key]);
                    }
                }

                if (String.Compare("wss", this.uri.Scheme, true) == 0) 
                {
                    bool trustAll = false;

                    if (props.ContainsKey(EFTL.PROPERTY_TRUST_ALL))
                        trustAll = (props[EFTL.PROPERTY_TRUST_ALL] as bool?) ?? false;

                    if(trustAll) 
                    {
                        // trust any server certificate by always returning true
                        ServicePointManager.ServerCertificateValidationCallback =
                           (sender, cert, chain, sslPolicyErrors) => true;
                    } 
                    else 
                    {
                        ServicePointManager.ServerCertificateValidationCallback =
                            (sender, cert, chain, sslPolicyErrors) => 
                            {
                                return (sslPolicyErrors == SslPolicyErrors.None);
                            };
                    }
                }

                webSocket = new WebSocket(uri, this);

                webSocket.setProtocol(ProtocolConstants.EFTL_WS_PROTOCOL);
                webSocket.Open();
            }
        }

        /**
         * Force the client to disconnect bypassing all eFTL and WebSocket 
         * protocols. This method primarily exists for testing various 
         * reconnect scenarios.
         */
        public void ForceDisconnect()
        {
            if (webSocket != null)
                webSocket.forceClose();
        }

        public String GetClientId() 
        {
            return clientId;
        }

        public void Reconnect(System.Collections.Hashtable props) 
        {
            if (IsConnected())
                return;

            // set only when auto-reconnecting
            Interlocked.Exchange(ref reconnecting, 0);

            Connect(props);
        }

        public void Disconnect() 
        {
            if (Interlocked.CompareExchange(ref connected, 0, 1) == 1) 
            {
                if (cancelReconnect()) 
                {
                    // clear the unacknowledged messages
                    clearRequests(CompletionListenerConstants.PUBLISH_FAILED, "Disconnected");

                    // a pending reconnect was cancelled, 
                    // tell the user we've disconnected
                    listener.OnDisconnect(this, ConnectionListenerConstants.NORMAL, null);
                } 
                else 
                {
                    // invoked in a task to prevent potential
                    // deadlocks with message callback
                    new Task(() => {
                        // synchronize to prevent a disconnect
                        // from occurring between the message
                        // callback and message acknowledgment
                        lock(processLock) 
                        {
                            Queue(DISCONNECT);
                        }
                    }).Start();
                }
            }
        }

        public bool IsConnected() 
        {
            return (Interlocked.Read(ref connected) == 1);
        }

        public IMessage CreateMessage() 
        {
            return new JSONMessage();
        }

        public IKVMap CreateKVMap(String name) 
        {
            return new KVMap(this, name);
        }

        public void MapSet(String name, String key, IMessage value, IKVMapListener listener) 
        {
            if (!IsConnected())
                throw new Exception("not connected");

            JsonObject envelope = new JsonObject();

            // synchronized to ensure message sequence numbers are ordered
            lock(writeLock) 
            {
                long seqNum = Interlocked.Increment(ref messageIdGenerator);

                envelope.Add(ProtocolConstants.OP_FIELD, ProtocolOpConstants.OP_MAP_SET);
                if (qos)
                    envelope.Add(ProtocolConstants.SEQ_NUM_FIELD, seqNum);
                if (name != null)
                    envelope.Add(ProtocolConstants.MAP_FIELD, name);
                if (key != null)
                    envelope.Add(ProtocolConstants.KEY_FIELD, key);
                if (value != null)
                    envelope.Add(ProtocolConstants.VALUE_FIELD, ((JSONMessage) value)._rawData());

                MapContext ctx = new MapContext(seqNum, envelope.ToString(), key, value, listener);

                if (maxMessageSize > 0 && ctx.GetJson().Length > maxMessageSize)
                    throw new Exception("maximum message size exceeded");

                requests.Put(seqNum, ctx);

                Queue(ctx);
            }
        }

        public void MapGet(String name, String key, IKVMapListener listener) 
        {
            if (!IsConnected())
                throw new Exception("not connected");

            JsonObject envelope = new JsonObject();

            // synchronized to ensure message sequence numbers are ordered
            lock(writeLock) 
            {
                long seqNum = Interlocked.Increment(ref messageIdGenerator);

                envelope.Add(ProtocolConstants.OP_FIELD, ProtocolOpConstants.OP_MAP_GET);
                if (qos)
                    envelope.Add(ProtocolConstants.SEQ_NUM_FIELD, seqNum);
                if (name != null)
                    envelope.Add(ProtocolConstants.MAP_FIELD, name);
                if (key != null)
                    envelope.Add(ProtocolConstants.KEY_FIELD, key);

                MapContext ctx = new MapContext(seqNum, envelope.ToString(), key, null, listener);

                requests.Put(seqNum, ctx);

                Queue(ctx);
            }
        }

        public void MapRemove(String name, String key, IKVMapListener listener) 
        {
            if (!IsConnected())
                throw new Exception("not connected");

            JsonObject envelope = new JsonObject();

            // synchronized to ensure message sequence numbers are ordered
            lock(writeLock) 
            {
                long seqNum = Interlocked.Increment(ref messageIdGenerator);

                envelope.Add(ProtocolConstants.OP_FIELD, ProtocolOpConstants.OP_MAP_REMOVE);
                if (qos)
                    envelope.Add(ProtocolConstants.SEQ_NUM_FIELD, seqNum);
                if (name != null)
                    envelope.Add(ProtocolConstants.MAP_FIELD, name);
                if (key != null)
                    envelope.Add(ProtocolConstants.KEY_FIELD, key);

                MapContext ctx = new MapContext(seqNum, envelope.ToString(), key, null, listener);

                requests.Put(seqNum, ctx);

                Queue(ctx);
            }
        }

        public void Publish(IMessage message) 
        {
            Publish(message, null);
        }

        public void Publish(IMessage message, ICompletionListener listener) 
        {
            if (!IsConnected())
                throw new Exception("not connected");

            JsonObject envelope = new JsonObject();

            // synchronized to ensure message sequence numbers are ordered
            lock(writeLock) 
            {
                long seqNum = Interlocked.Increment(ref messageIdGenerator);

                envelope.Add(ProtocolConstants.OP_FIELD, ProtocolOpConstants.OP_MESSAGE);
                envelope.Add(ProtocolConstants.BODY_FIELD, ((JSONMessage) message)._rawData());

                if (qos) 
                {
                    envelope.Add(ProtocolConstants.SEQ_NUM_FIELD, seqNum);
                }

                PublishContext ctx = new PublishContext(seqNum, envelope.ToString(), message, listener);

                if (maxMessageSize > 0 && ctx.GetJson().Length > maxMessageSize)
                    throw new Exception("maximum message size exceeded");

                requests.Put(seqNum, ctx);

                Queue(ctx);
            }
        }

        public String Subscribe(String matcher, ISubscriptionListener listener) 
        {
            return Subscribe(matcher, null, null, listener);
        }

        public String Subscribe(String matcher, String durable, ISubscriptionListener listener) 
        {
            return Subscribe(matcher, durable, null, listener);
        }

        public String Subscribe(String matcher, String durable, Hashtable props, ISubscriptionListener listener) 
        {
            if (!IsConnected())
                throw new Exception("not connected");

            String subscriptionId = clientId + ".s." + Interlocked.Increment(ref subscriptionIdGenerator);
            Subscribe(subscriptionId, matcher, durable, props, listener);
            return subscriptionId;
        }

        private void Subscribe(String subscriptionId, String matcher, String durable, Hashtable props, ISubscriptionListener listener) 
        {
            BasicSubscription subscription = new BasicSubscription(subscriptionId, matcher, durable, props, listener);
            subscription.setPending(true);
            subscriptions.TryAdd(subscription.getSubscriptionId(), subscription);

            Subscribe(subscription);
        }

        private void Subscribe(BasicSubscription subscription) 
        {
            JsonObject message = new JsonObject();

            message[ProtocolConstants.OP_FIELD] = ProtocolOpConstants.OP_SUBSCRIBE;
            message[ProtocolConstants.ID_FIELD] = subscription.getSubscriptionId();
            if (subscription.getContentMatcher() != null)
                message[ProtocolConstants.MATCHER_FIELD] = subscription.getContentMatcher();
            if (subscription.getDurable() != null)
                message[ProtocolConstants.DURABLE_FIELD] = subscription.getDurable();
            if (subscription.getProperties() != null) 
            {
                foreach (String key in subscription.getProperties().Keys) 
                {
                    message[key] = subscription.getProperties() [key];
                }
            }

            Queue(message.ToString());
        }

        public void Unsubscribe(String subscriptionId) 
        {
            BasicSubscription subscription = null;
            if (!IsConnected())
                throw new Exception("not connected");

            bool removed = subscriptions.TryRemove(subscriptionId, out subscription);
            if (removed && subscription != null) 
            {
                JsonObject message = new JsonObject();

                message[ProtocolConstants.OP_FIELD] = ProtocolOpConstants.OP_UNSUBSCRIBE;
                message[ProtocolConstants.ID_FIELD] = subscription.getSubscriptionId();

                Queue(message.ToString());
            }
        }

        public void UnsubscribeAll() 
        {
            foreach (String subscriptionId in subscriptions.Keys) 
            {
                Unsubscribe(subscriptionId);
            }
        }

        // implemtation if WebSocketListener.
        public void OnOpen() 
        {
            try 
            {
                String user =(String) props[EFTL.PROPERTY_USERNAME];
                String password =(String) props[EFTL.PROPERTY_PASSWORD];
                String identifier =(String) props[EFTL.PROPERTY_CLIENT_ID];

                String userInfo = uri.UserInfo;

                if (userInfo != null && userInfo.Length > 0) 
                {
                    String[] tokens = userInfo.Split(':');

                    user = tokens[0];

                    if (tokens.Length > 1)
                        password = tokens[1];
                }

                String query = uri.Query;

                if (query != null && query.Length > 0) 
                {
                    // remove leading '?'
                    if (query[0] == '?')
                        query = query.Substring(1);

                    String[] tokens = query.Split('&');

                    for (int i = 0; i < tokens.Length; i++) 
                    {
                        if (tokens[i].StartsWith("clientId=")) 
                        {
                            identifier = tokens[i].Substring("clientId=".Length);
                        }
                    }
                }

                JsonObject message = new JsonObject();

                message[ProtocolConstants.OP_FIELD]              = ProtocolOpConstants.OP_LOGIN;
                message[ProtocolConstants.CLIENT_TYPE_FIELD]     = "c#";
                message[ProtocolConstants.CLIENT_VERSION_FIELD]  = Version.EFTL_VERSION_STRING_SHORT;

                if (user != null)
                    message[ProtocolConstants.USER_FIELD] = user;
                if (password != null)
                    message[ProtocolConstants.PASSWORD_FIELD] = password;

                if (clientId != null && reconnectId != null) 
                {
                    message[ProtocolConstants.CLIENT_ID_FIELD] = clientId;
                    message[ProtocolConstants.ID_TOKEN_FIELD] = reconnectId;
                } 
                else if (identifier != null) 
                {
                    message[ProtocolConstants.CLIENT_ID_FIELD] = identifier;
                }

                JsonObject loginOptions = new JsonObject();

                // add resume when auto-reconnecting
                if (Interlocked.Read(ref reconnecting) == 1)
                    loginOptions[ProtocolConstants.RESUME_FIELD] = "true";

                try 
                {
                    // add user properties
                    foreach (String key in props.Keys) 
                    {
                        if (key.Equals(EFTL.PROPERTY_USERNAME) ||
                            key.Equals(EFTL.PROPERTY_PASSWORD) ||
                            key.Equals(EFTL.PROPERTY_CLIENT_ID))
                            continue;

                        String val = (String) props[key];

                        if (String.Compare("true", val) == 0)
                            loginOptions[key] = "true";
                        else
                            loginOptions[key] = val;
                    }
                } 
                catch(Exception) 
                {
                    //Console.WriteLine(e.StackTrace);
                }

                message[ProtocolConstants.LOGIN_OPTIONS_FIELD] = loginOptions;

                try
                {
                    webSocket.send(message.ToString());
                } 
                catch(Exception) 
                {

                }
            } 
            catch(Exception) 
            {
                // Console.WriteLine(e.StackTrace);
            }
        }

        public void OnClose(int code, String reason) 
        {
            if (Interlocked.CompareExchange(ref connecting, 0, 1) == 1) 
            {
                // no longer connected
                Interlocked.Exchange(ref connected, 0);

                // stop the writer thread
                if (writer != null)
                    writer.Interrupt();

                // clear the unacknowledged messages
                clearRequests(CompletionListenerConstants.PUBLISH_FAILED, "Closed");

                listener.OnDisconnect(this, code, reason);
            }
        }

        public void OnError(Exception cause) 
        {
            if (Interlocked.CompareExchange(ref connecting, 0, 1) == 1) 
            {
                if (!scheduleReconnect()) 
                {
                    // no longer connected
                    Interlocked.Exchange(ref connected, 0);

                    // stop the writer thread
                    if (writer != null)
                        writer.Interrupt();

                    // clear the unacknowledged messages
                    clearRequests(CompletionListenerConstants.PUBLISH_FAILED, "Error");

                    listener.OnDisconnect(this, ConnectionListenerConstants.CONNECTION_ERROR, cause.Message);
                } 
                else 
                {
                    // stop the writer thread
                    if (writer != null)
                        writer.Interrupt();
                }
            }
        }

        public void OnMessage(String text) 
        {
            object obj = JsonValue.Parse(text);

            if (obj is JsonArray) 
            {
                handleMessages((JsonArray) obj);
            } 
            else if (obj is JsonObject) 
            {
                JsonObject message = (JsonObject) obj;

                object op;

                if (message.TryGetValue(ProtocolConstants.OP_FIELD, out op)) 
                {
                    switch (Convert.ToInt32(op)) 
                    {
                        case ProtocolOpConstants.OP_HEARTBEAT:
                            handleHeartbeat(message);
                            break;
                        case ProtocolOpConstants.OP_WELCOME:
                            handleWelcome(message);
                            break;
                        case ProtocolOpConstants.OP_SUBSCRIBED:
                            handleSubscribed(message);
                            break;
                        case ProtocolOpConstants.OP_UNSUBSCRIBED:
                            handleUnsubscribed(message);
                            break;
                        case ProtocolOpConstants.OP_EVENT:
                            handleMessage(message);
                            break;
                        case ProtocolOpConstants.OP_ERROR:
                            handleError(message);
                            break;
                        case ProtocolOpConstants.OP_GOODBYE:
                            handleGoodbye(message);
                            break;
                        case ProtocolOpConstants.OP_ACK:
                            handleAck(message);
                            break;
                        case ProtocolOpConstants.OP_MAP_RESPONSE:
                            handleMapResponse(message);
                            break;
                    }
                }
            }
        }

        public void OnMessage(byte[] data, int offset, int length) 
        {
            // ignore
        }

        public void OnPong(byte[] data, int offset, int length) 
        {
            // ignore
        }

        public void Dispose() 
        {
            // ignore
        }

        private void handleHeartbeat(JsonObject message) 
        {
            // queue message for writer thread
            Queue(message.ToString());
        }

        private void handleWelcome(JsonObject message) 
        {
            // a non-null reconnect token indicates a prior connection
            bool invokeOnReconnect = (reconnectId != null);
            bool resume = false;

            clientId = (String) message[ProtocolConstants.CLIENT_ID_FIELD];
            reconnectId = (String) message[ProtocolConstants.ID_TOKEN_FIELD];
            maxMessageSize = (long) message[ProtocolConstants.MAX_SIZE_FIELD];

            if (message.ContainsKey(ProtocolConstants.QOS_FIELD))
                qos = System.Boolean.Parse((String) message[ProtocolConstants.QOS_FIELD]);
            else
                qos = false;

            if (message.ContainsKey(ProtocolConstants.RESUME_FIELD))
                resume = System.Boolean.Parse((String) message[ProtocolConstants.RESUME_FIELD]);

            // connected
            Interlocked.Exchange(ref connected, 1);

            // reset reconnect attempts
            reconnectAttempts = 0;

            // synchronized to ensure message sequence numbers are ordered
            lock(writeLock) 
            {
                // clear the queue
                while (writeQueue.Count > 0)
                    writeQueue.Take();

                // start writer thread
                writer = new Thread(new ThreadStart(Run));
                writer.Name = "EFTL Writer";
                writer.IsBackground = true;
                writer.Start();

                // repair subscriptions
                foreach (BasicSubscription subscription in subscriptions.Values) 
                {
                    // invoke callback if not auto-reconnecting
                    if (Interlocked.Read(ref reconnecting) != 1)
                        subscription.setPending(true);

                    Subscribe(subscription);
                }

                if (resume) 
                {
                    // re-send unacknowledged messages
                    foreach (PublishContext ctx in requests.Values())
                        Queue(ctx);
                }
                else 
                {
                    // clear unacknowledged messages
                    clearRequests(CompletionListenerConstants.PUBLISH_FAILED, "Reconnect");

                    // reset the last sequence number if not a reconnect
                    lastSequenceNumber = 0;
                }
            }

            // invoke callback if not auto-reconnecting
            if (!invokeOnReconnect)
                listener.OnConnect(this);
            else if (Interlocked.Read(ref reconnecting) == 0)
                listener.OnReconnect(this);
        }

        private void handleSubscribed(JsonObject message) 
        {
            String subscriptionId = (String) message[ProtocolConstants.ID_FIELD];

            // A subscription request has succeeded.
            BasicSubscription subscription = subscriptions[subscriptionId];
            if (subscription != null && subscription.isPending()) 
            {
                subscription.setPending(false);
                subscription.getListener().OnSubscribe(subscriptionId);
            }
        }

        private void handleUnsubscribed(JsonObject message) 
        {
            String subscriptionId = (String) message[ProtocolConstants.ID_FIELD];
            long code = (long) message[ProtocolConstants.ERR_CODE_FIELD];
            String reason = (String) message[ProtocolConstants.REASON_FIELD];

            // A subscription request has failed.
            // Possible errors include:
            //   LISTENS_DISABLED listens are disabled
            //   LISTENS_DISALLOWED listens are disabled for this user
            //   SUBSCRIPTION_FAILED an internal error occurred

            BasicSubscription subscription = null;
            bool removed = subscriptions.TryRemove(subscriptionId, out subscription);
            if (removed && subscription != null) 
            {
                subscription.getListener().OnError(subscriptionId, Convert.ToInt32(code), reason);
            }
        }

        private void handleMessages(JsonArray array) 
        {
            lock(processLock) 
            {
                ArrayList messages = new ArrayList(array.Count);

                BasicSubscription currentSubscription = null;
                long seqNum = 0;
                long lastSeqNum = 0;
                int max = array.Count;

                for (int i = 0; i < max; i++) 
                {

                    JsonObject envelope = (JsonObject) array[i];

                    string to = (String) envelope[ProtocolConstants.TO_FIELD];
                    JsonObject message = (JsonObject) envelope[ProtocolConstants.BODY_FIELD];

                    object seqNumObj;

                    if (envelope.TryGetValue(ProtocolConstants.SEQ_NUM_FIELD, out seqNumObj))
                        seqNum = Convert.ToInt64(seqNumObj);

                    // The message will be processed if qos is not enabled, there is no 
                    // sequence number or if the sequence number is greater than the last 
                    // received sequence number.

                    if (!qos || seqNum == 0 || seqNum > lastSequenceNumber) 
                    {
                        BasicSubscription subscription = null;
                        bool exists = subscriptions.TryGetValue(to, out subscription);

                        if (exists && subscription != null) 
                        {
                            if (currentSubscription != null && currentSubscription != subscription) 
                            {
                                try 
                                {
                                    IMessage[] arr = new IMessage[messages.Count];

                                    for (int j = 0; j < messages.Count; j++)
                                        arr[j] = (JSONMessage) messages[j];

                                    currentSubscription.getListener().OnMessages(arr);
                                } 
                                catch(Exception) 
                                {
                                    // catch and discard exceptions thrown by the listener
                                }
                                messages.Clear();
                            }

                            currentSubscription = subscription;

                            messages.Add(new JSONMessage(message));
                        }

                        // track the last received sequence number only if qos is enabled
                        if (qos && seqNum > 0)
                            lastSequenceNumber = seqNum;
                    }

                    if (seqNum > 0)
                        lastSeqNum = seqNum;
                }

                if (currentSubscription != null && messages.Count > 0) 
                {
                    try 
                    {
                        IMessage[] arr = new IMessage[messages.Count];

                        for (int i = 0; i < messages.Count; i++)
                            arr[i] = (JSONMessage) messages[i];

                        currentSubscription.getListener().OnMessages(arr);
                    } 
                    catch(Exception) 
                    {
                        // catch and discard exceptions thrown by the listener
                        // Console.WriteLine(e.Message);
                    }
                    messages.Clear();
                }

                // Send an acknowledgment for the last sequence number in the array.
                // The server will acknowledge all messages less than or equal to
                // this sequence number.

                if (lastSeqNum > 0)
                    ack(lastSeqNum);
            }
        }

        private void handleMessage(JsonObject envelope) 
        {
            lock(processLock) 
            {
                long seqNum = 0;

                String to = (String) envelope[ProtocolConstants.TO_FIELD];
                JsonObject message = (JsonObject) envelope[ProtocolConstants.BODY_FIELD];

                object seqNumObj;

                if (envelope.TryGetValue(ProtocolConstants.SEQ_NUM_FIELD, out seqNumObj))
                    seqNum = Convert.ToInt64(seqNumObj);

                // The message will be processed if qos is not enabled, there is no 
                // sequence number or if the sequence number is greater than the last 
                // received sequence number.

                if (!qos || seqNum == 0 || seqNum > lastSequenceNumber) 
                {
                    BasicSubscription subscription = null;

                    bool exists = subscriptions.TryGetValue(to, out subscription);
                    if (exists && subscription != null) 
                    {
                        IMessage[] messages = { new JSONMessage(message) };
                        try 
                        {
                            subscription.getListener().OnMessages(messages);
                        } 
                        catch(Exception) 
                        {
                            // catch and discard exceptions thrown by the listener
                        }
                    }

                    // track the last received sequence number only if qos is enabled
                    if (qos && seqNum > 0)
                        lastSequenceNumber = seqNum;
                }

                if (seqNum > 0)
                    ack(seqNum);
            }
        }

        private void handleError(JsonObject message) 
        {
            long code = (long) message[ProtocolConstants.ERR_CODE_FIELD];
            String reason = (String) message[ProtocolConstants.REASON_FIELD];

            // The server is sending an error to the client.
            // Possible errors include:
            //   BAD_SUBSCRIBER_ID a subscription ID is null or does not start 
            //                     with the client ID
            //   SEND_DISALLOWED sending is disabled for this user

            listener.OnError(this, Convert.ToInt32(code), reason);
        }

        private void handleGoodbye(JsonObject message) 
        {
            long code = (long) message[ProtocolConstants.ERR_CODE_FIELD];
            String reason = (String) message[ProtocolConstants.REASON_FIELD];

            // The server is terminating its connection with the client.
            // Possible errors include:
            //   CLOSE_PROTOCOL for invalid messages
            //   CLOSE_POLICY_VIOLATION for authentication errors
            //   CLOSE_MESSAGE_TO_LARGE if the message is too large

            if (Interlocked.CompareExchange(ref connecting, 0, 1) == 1) 
            {
                // close the websocket connection
                webSocket.close();

                // no longer connected
                Interlocked.Exchange(ref connected, 0);

                // stop the writer thread
                if (writer != null)
                    writer.Interrupt();

                // clear the unacknowledged messages
                clearRequests(CompletionListenerConstants.PUBLISH_FAILED, "Goodbye");

                listener.OnDisconnect(this, Convert.ToInt32(code), reason);
            }
        }

        private void handleAck(JsonObject message) 
        {
            long seqNum = 0;
            int code = 0;
            string reason = null;

            object obj;

            if (message.TryGetValue(ProtocolConstants.SEQ_NUM_FIELD, out obj))
                seqNum = Convert.ToInt64(obj);
            if (message.TryGetValue(ProtocolConstants.ERR_CODE_FIELD, out obj))
                code = Convert.ToInt32(obj);
            if (message.TryGetValue(ProtocolConstants.REASON_FIELD, out obj))
                reason = Convert.ToString(obj);

            // Remove all unacknowledged messages with a sequence number equal to
            // or less than the sequence number contained within the ack message.

            if (seqNum > 0) 
            {
                if (code > 0)
                    requestError(seqNum, code, reason);
                else
                    requestSuccess(seqNum, null);
            }
        }

        private void handleMapResponse(JsonObject message) 
        {
            long seqNum = 0;
            JsonObject value = null;
            int code = 0;
            string reason = null;

            object obj;

            if (message.TryGetValue(ProtocolConstants.SEQ_NUM_FIELD, out obj))
                seqNum = Convert.ToInt64(obj);
            if (message.TryGetValue(ProtocolConstants.VALUE_FIELD, out obj))
                value =(JsonObject) obj;
            if (message.TryGetValue(ProtocolConstants.ERR_CODE_FIELD, out obj))
                code = Convert.ToInt32(obj);
            if (message.TryGetValue(ProtocolConstants.REASON_FIELD, out obj))
                reason = Convert.ToString(obj);

            // Remove all unacknowledged messages with a sequence number equal to
            // or less than the sequence number contained within the ack message.

            if (seqNum > 0) 
            {
                if (code > 0)
                    requestError(seqNum, code, reason);
                else
                    requestSuccess(seqNum, (value != null ? new JSONMessage(value) : null));
            }
        }

        private void ack(Int64 seqNum) 
        {
            if (!qos)
                return;

            JsonObject message = new JsonObject();

            message[ProtocolConstants.OP_FIELD] = ProtocolOpConstants.OP_ACK;
            message[ProtocolConstants.SEQ_NUM_FIELD] = seqNum;

            // queue message for writer thread
            Queue(message.ToString());
        }

        private void requestSuccess(Int64 seqNum, IMessage response) 
        {
            RequestContext ctx = null;
            if (requests.Remove(seqNum, out ctx)) 
            {
                if (ctx.GetListener() != null)
                    ctx.GetListener().OnSuccess(response);
            }
        }

        private void requestError(Int64 seqNum, int code, String reason) 
        {
            RequestContext ctx = null;
            if (requests.Remove(seqNum, out ctx)) 
            {
                if (ctx.GetListener() != null)
                    ctx.GetListener().OnError(code, reason);
                else
                    listener.OnError(this, code, reason);
            }
        }

        private void clearRequests(int code, String reason) 
        {
            foreach (RequestContext ctx in requests.Values()) 
            {
                if (ctx.GetListener() != null)
                    ctx.GetListener().OnError(code, reason);
                requests.Remove(ctx.GetSeqNum());
            }
        }

        private bool scheduleReconnect() 
        {
            bool scheduled = false;

            // schedule a reconnect only if previously connected and 
            // the number of reconnect attempts has not been exceeded
            if (Interlocked.Read(ref connected) == 1 && reconnectAttempts < GetReconnectAttempts()) 
            {
                try 
                {
                    // exponential backoff truncated to 30 seconds
                    long backoff = Math.Min((long) Math.Pow(2.0, reconnectAttempts++) * 1000, GetReconnectMaxDelay());

                    timer = new System.Timers.Timer();

                    // Hook up the Elapsed event for the timer.
                    timer.Elapsed += new ElapsedEventHandler(reconnectTask);

                    timer.Interval = backoff;
                    timer.Enabled = true;

                    scheduled = true;
                } 
                catch(Exception) 
                {
                    // failed to schedule reconnect
                }
            }

            return scheduled;
        }

        private void reconnectTask(object source, ElapsedEventArgs e) 
        {
            try 
            {
                System.Timers.Timer timer = (System.Timers.Timer) source;

                // set only when auto-reconnecting
                Interlocked.Exchange(ref reconnecting, 1);

                Connect(null);

                timer.Enabled = false;
            } 
            catch(Exception) 
            { 
            }
        }

        private bool cancelReconnect() 
        {
            bool cancelled = false;

            return cancelled;
        }

        private int GetConnectTimeout() 
        {
            // defaults to 15 seconds
            String ctimeout = "15.0";

            if (props.ContainsKey(EFTL.PROPERTY_TIMEOUT))
                ctimeout = (String) props[EFTL.PROPERTY_TIMEOUT];

            return (int)(Double.Parse(ctimeout) * 1000.0);
        }

        private int GetReconnectAttempts() 
        {
            // defaults to 5 attempts
            int value = defaultAutoReconnectAttempts;

            if (props.ContainsKey(EFTL.PROPERTY_AUTO_RECONNECT_ATTEMPTS)) 
            {
                string propValue = (String) props[EFTL.PROPERTY_AUTO_RECONNECT_ATTEMPTS];
                int intValue = 0;
                if (Int32.TryParse(propValue, out intValue))
                    value = intValue;
            }

            return value;
        }

        private int GetReconnectMaxDelay() 
        {
            // defaults to 30 seconds
            int value = defaultAutoReconnectMaxDelay;

            if (props.ContainsKey(EFTL.PROPERTY_AUTO_RECONNECT_MAX_DELAY)) 
            {
                string propValue = (String) props[EFTL.PROPERTY_AUTO_RECONNECT_MAX_DELAY];
                double doubleValue = 0.0;
                if (Double.TryParse(propValue, out doubleValue))
                    value = (int)(doubleValue * 1000.0);
            }

            return value;
        }

        private void Queue(String text) 
        {
            Queue(new RequestContext(text));
        }

        private void Queue(RequestContext ctx) 
        {
            writeQueue.Add(ctx);
        }

        public void Run() 
        {
            try 
            {
                while (webSocket.isConnected()) 
                {
                    RequestContext ctx = writeQueue.Take();

                    if (ctx == DISCONNECT) 
                    {
                        // send a disconnect message and close
                        JsonObject message = new JsonObject();
                        message[ProtocolConstants.OP_FIELD] = ProtocolOpConstants.OP_DISCONNECT;
                        webSocket.send(message.ToString());
                        webSocket.close();
                    } 
                    else 
                    {
                        webSocket.send(ctx.GetJson());

                        if (!qos && ctx.GetSeqNum() > 0) 
                        {
                            requestSuccess(ctx.GetSeqNum(), null);
                        }
                    }
                }
            } 
            catch(Exception) 
            {
            }
        }
    }
}
