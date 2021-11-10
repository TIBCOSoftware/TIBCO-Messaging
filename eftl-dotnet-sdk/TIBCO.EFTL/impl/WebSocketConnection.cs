/*
 * Copyright (c) 2009-$Date: 2021-02-18 09:31:00 -0800 (Thu, 18 Feb 2021) $ TIBCO Software Inc.
 * All Rights Reserved.
 * For more information, please contact:
 * TIBCO Software Inc., Palo Alto, California, USA
 *
 * $Id: WebSocketConnection.cs 131938 2021-02-18 17:31:00Z $
 */
using System;
using System.Collections;
using System.Collections.Concurrent;
using System.Collections.Generic;
using System.Linq;
using System.Threading;
using System.Threading.Tasks;
using System.Timers;
using System.Net;
using System.Net.Security;
using System.Reflection;

namespace TIBCO.EFTL 
{
    public class WebSocketConnection : TIBCO.EFTL.IConnection, TIBCO.EFTL.WebSocketListener, IDisposable 
    {
        List<Uri>           urlList = new List<Uri>();
        int                 urlIndex;
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
        protected long      protocol;
        protected long      maxMessageSize;
        protected int       reconnectAttempts = 0;
        protected Thread    writer;
        protected System.Timers.Timer     timer;

        BlockingCollection<RequestContext> writeQueue = new BlockingCollection<RequestContext>();
        ConcurrentDictionary<String, BasicSubscription> subscriptions = new ConcurrentDictionary<String, BasicSubscription>();
        internal SkipList<RequestContext> requests = new SkipList<RequestContext>();

        private static readonly RequestContext DISCONNECT = new RequestContext("");
        private static readonly double defaultConnectTimeout = 15.0;
        private static readonly int defaultAutoReconnectAttempts = 256;
        private static readonly int defaultAutoReconnectMaxDelay = 30000;

        private static readonly Random rand = new Random();

        public WebSocketConnection(String uri, IConnectionListener listener) 
        {
            try 
            {
                setURLList(uri);

                this.listener = listener;
                this.props = new Hashtable();

                props.Add(ProtocolConstants.QOS_FIELD, "true");
            } 
            catch(Exception) 
            {
                throw new System.ArgumentException(uri);
            }
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

                setState(ConnectionState.CONNECTING);

                if (String.Compare("wss", this.getURL().Scheme, true) == 0) 
                {
                    bool trustAll = false;

                    ServicePointManager.SecurityProtocol = SecurityProtocolType.Tls12;
                    ServicePointManager.MaxServicePointIdleTime = 1000;

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

                webSocket = new WebSocket(getURL(), this);

                webSocket.setProtocol(ProtocolConstants.EFTL_WS_PROTOCOL);
                webSocket.setTimeout(GetConnectTimeout());
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
            setState(ConnectionState.DISCONNECTING);

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

        public void RemoveKVMap(String name)
        {
            if (!IsConnected())
                throw new Exception("not connected");

            JsonObject envelope = new JsonObject();

            envelope.Add(ProtocolConstants.OP_FIELD, ProtocolOpConstants.OP_MAP_DESTROY);
            if (name != null)
                envelope.Add(ProtocolConstants.MAP_FIELD, name);

            Queue(envelope.ToString());
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

        public void SendRequest(IMessage request, double timeout, IRequestListener listener)
        {
            if (!IsConnected())
                throw new Exception("not connected");

            JsonObject envelope = new JsonObject();

            // synchronized to ensure message sequence numbers are ordered
            lock(writeLock) 
            {
                long seqNum = Interlocked.Increment(ref messageIdGenerator);

                envelope.Add(ProtocolConstants.OP_FIELD, ProtocolOpConstants.OP_REQUEST);
                envelope.Add(ProtocolConstants.SEQ_NUM_FIELD, seqNum);
                envelope.Add(ProtocolConstants.BODY_FIELD, ((JSONMessage) request)._rawData());

                SendRequestContext ctx = new SendRequestContext(seqNum, envelope.ToString(), request, listener);

                if (protocol < 1)
                    throw new NotSupportedException("send request is not supported with this server");

                if (maxMessageSize > 0 && ctx.GetJson().Length > maxMessageSize)
                    throw new Exception("maximum message size exceeded");

                ctx.SetTimeout(timeout, new TimerCallback(requestTimeout));

                requests.Put(seqNum, ctx);

                Queue(ctx);
            }
        }

        public void SendReply(IMessage reply, IMessage request, ICompletionListener listener)
        {
            if (!IsConnected())
                throw new Exception("not connected");

            if (((JSONMessage) request).ReplyTo == null)
                throw new ArgumentException("not a request message");

            JsonObject envelope = new JsonObject();

            // synchronized to ensure message sequence numbers are ordered
            lock(writeLock) 
            {
                long seqNum = Interlocked.Increment(ref messageIdGenerator);

                envelope.Add(ProtocolConstants.OP_FIELD, ProtocolOpConstants.OP_REPLY);
                envelope.Add(ProtocolConstants.SEQ_NUM_FIELD, seqNum);
                envelope.Add(ProtocolConstants.TO_FIELD, ((JSONMessage) request).ReplyTo);
                envelope.Add(ProtocolConstants.REQ_ID_FIELD, ((JSONMessage) request).ReqId);
                envelope.Add(ProtocolConstants.BODY_FIELD, ((JSONMessage) reply)._rawData());

                PublishContext ctx = new PublishContext(seqNum, envelope.ToString(), reply, listener);

                if (protocol < 1)
                    throw new NotSupportedException("send reply is not supported with this server");

                if (maxMessageSize > 0 && ctx.GetJson().Length > maxMessageSize)
                    throw new Exception("maximum message size exceeded");

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

            String subscriptionId =  Convert.ToString(Interlocked.Increment(ref subscriptionIdGenerator));
            Subscribe(subscriptionId, matcher, durable, props, listener);
            return subscriptionId;
        }

        private void Subscribe(String subscriptionId, String matcher, String durable, Hashtable props, ISubscriptionListener listener) 
        {
            BasicSubscription subscription = new BasicSubscription(subscriptionId, matcher, durable, props, listener);
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

        public void CloseSubscription(String subscriptionId) 
        {
            BasicSubscription subscription = null;
            if (!IsConnected())
                throw new Exception("not connected");

            if (protocol < 1)
                throw new NotSupportedException("close subscription is not supported with this server");

            bool removed = subscriptions.TryRemove(subscriptionId, out subscription);
            if (removed && subscription != null) 
            {
                JsonObject message = new JsonObject();

                message[ProtocolConstants.OP_FIELD] = ProtocolOpConstants.OP_UNSUBSCRIBE;
                message[ProtocolConstants.ID_FIELD] = subscription.getSubscriptionId();
                message[ProtocolConstants.DEL_FIELD] = "false";

                Queue(message.ToString());
            }
        }

        public void CloseAllSubscriptions() 
        {
            foreach (String subscriptionId in subscriptions.Keys) 
            {
                CloseSubscription(subscriptionId);
            }
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

        public void Acknowledge(IMessage message)
        {
            if (!IsConnected())
                throw new Exception("not connected");

            acknowledge(((JSONMessage) message).SeqNum, null);
        }

        public void AcknowledgeAll(IMessage message)
        {
            if (!IsConnected())
                throw new Exception("not connected");

            acknowledge(((JSONMessage) message).SeqNum, ((JSONMessage) message).SubId);
        }

        // implemtation if WebSocketListener.
        public void OnOpen() 
        {
            try 
            {
                String user =(String) props[EFTL.PROPERTY_USERNAME];
                String password =(String) props[EFTL.PROPERTY_PASSWORD];
                String identifier =(String) props[EFTL.PROPERTY_CLIENT_ID];

                String userInfo = getURL().UserInfo;

                if (userInfo != null && userInfo.Length > 0) 
                {
                    String[] tokens = userInfo.Split(':');

                    user = tokens[0];

                    if (tokens.Length > 1)
                        password = tokens[1];
                }

                String query = getURL().Query;

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
                message[ProtocolConstants.PROTOCOL_FIELD]        = ProtocolConstants.PROTOCOL_VERSION;
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

                int maxPendingAcks = GetMaxPendingAcks();

                if (maxPendingAcks > 0) 
                {
                    message[ProtocolConstants.MAX_PENDING_ACKS_FIELD] = maxPendingAcks;
                }

                JsonObject loginOptions = new JsonObject();

                // add resume when auto-reconnecting
                if (Interlocked.Read(ref reconnecting) == 1)
                    loginOptions[ProtocolConstants.RESUME_FIELD] = "true";

                // add user properties
                foreach (String key in props.Keys) 
                {
                    if (key.Equals(EFTL.PROPERTY_USERNAME) ||
                        key.Equals(EFTL.PROPERTY_PASSWORD) ||
                        key.Equals(EFTL.PROPERTY_CLIENT_ID) ||
                        key.Equals(EFTL.PROPERTY_TRUST_ALL))
                        continue;

                    try
                    {
                        String val = (String) props[key];
                        loginOptions[key] = val;
                    } 
                    catch(Exception) 
                    {
                        //Console.WriteLine(e.StackTrace);
                    }
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
            setState(ConnectionState.DISCONNECTED);

            if (Interlocked.CompareExchange(ref connecting, 0, 1) == 1) 
            {
                // Reconnect when the close code reflects a server restart.
                if (code != ConnectionListenerConstants.RESTART || !scheduleReconnect()) 
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
                else
                {
                    // stop the writer thread
                    if (writer != null)
                        writer.Interrupt();
                }
            }
        }

        public void OnError(Exception cause) 
        {
            setState(ConnectionState.DISCONNECTED);

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

            if (obj is JsonObject) 
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
                        case ProtocolOpConstants.OP_REQUEST_REPLY:
                            handleReply(message);
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
            setState(ConnectionState.CONNECTED);

            // a non-null reconnect token indicates a prior connection
            bool invokeOnReconnect = (reconnectId != null);
            bool resume = false;

            clientId = (String) message[ProtocolConstants.CLIENT_ID_FIELD];
            reconnectId = (String) message[ProtocolConstants.ID_TOKEN_FIELD];
            maxMessageSize = (long) message[ProtocolConstants.MAX_SIZE_FIELD];
try {
            if (message.ContainsKey(ProtocolConstants.PROTOCOL_FIELD))
                protocol = (long) message[ProtocolConstants.PROTOCOL_FIELD];
            else
                protocol = 0;
} catch (Exception e) {
Console.WriteLine(e);
}
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

            // reset URL list to start auto-reconnect attempts from the beginning
            resetURLList();

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
                    if (!resume)
                        subscription.LastSeqNum = 0;

                    Subscribe(subscription);
                }

                // re-send unacknowledged messages
                foreach (PublishContext ctx in requests.Values())
                    Queue(ctx);
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
            if (subscription != null && subscription.Pending)
            {
                subscription.Pending = false;
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

            BasicSubscription subscription = subscriptions[subscriptionId];

            // remove the subscription if not retryable
            if (code == SubscriptionConstants.SUBSCRIPTION_INVALID)
                subscriptions.TryRemove(subscriptionId, out subscription);

            if (subscription != null) 
            {
                subscription.Pending = true;
                subscription.getListener().OnError(subscriptionId, Convert.ToInt32(code), reason);
            }
        }

        private void handleMessage(JsonObject envelope) 
        {
            lock(processLock) 
            {
                long seqNum = 0;
                long reqId = 0;
                long msgId = 0;
                long deliveryCount = 0;
                String replyTo = null;
                
                String to = (String) envelope[ProtocolConstants.TO_FIELD];
                JsonObject body = (JsonObject) envelope[ProtocolConstants.BODY_FIELD];

                object fieldObj;

                if (envelope.TryGetValue(ProtocolConstants.SEQ_NUM_FIELD, out fieldObj))
                    seqNum = Convert.ToInt64(fieldObj);
                if (envelope.TryGetValue(ProtocolConstants.REQ_ID_FIELD, out fieldObj))
                    reqId = Convert.ToInt64(fieldObj);
                if (envelope.TryGetValue(ProtocolConstants.STORE_MSG_ID_FIELD, out fieldObj))
                    msgId = Convert.ToInt64(fieldObj);
                if (envelope.TryGetValue(ProtocolConstants.DELIVERY_COUNT_FIELD, out fieldObj))
                    deliveryCount = Convert.ToInt64(fieldObj);
                if (envelope.TryGetValue(ProtocolConstants.REPLY_TO_FIELD, out fieldObj))
                    replyTo = Convert.ToString(fieldObj);

                // The message will be processed if there is no sequence number or 
                // if the sequence number is greater than the last received sequence number.

                BasicSubscription subscription = null;

                bool exists = subscriptions.TryGetValue(to, out subscription);
                if (exists && subscription != null) 
                {
                    if (seqNum == 0 || seqNum > subscription.LastSeqNum)
                    {
                        IMessage message = new JSONMessage(body);

                        ((JSONMessage) message).SeqNum = seqNum;
                        ((JSONMessage) message).SubId = to;
                        ((JSONMessage) message).ReplyTo = replyTo;
                        ((JSONMessage) message).ReqId = reqId;
                        ((JSONMessage) message).StoreMessageId = msgId;
                        ((JSONMessage) message).DeliveryCount = deliveryCount;

                        try 
                        {
                            subscription.getListener().OnMessages(new IMessage[] {message});
                        } 
                        catch(Exception) 
                        {
                            // catch and discard exceptions thrown by the listener
                        }

                        // track the last received sequence number 
                        if (subscription.AutoAck && seqNum != 0)
                            subscription.LastSeqNum = seqNum;
                    }

                    // auto-acknowledge the message
                    if (subscription.AutoAck && seqNum != 0)
                        acknowledge(seqNum, subscription.Id);
                }
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

            if (seqNum > 0) 
            {
                if (code > 0)
                    requestError(seqNum, code, reason);
                else
                    requestSuccess(seqNum, null);
            }
        }

        private void handleReply(JsonObject message) 
        {
            long seqNum = 0;
            JsonObject body = null;
            int code = 0;
            string reason = null;

            object obj;

            if (message.TryGetValue(ProtocolConstants.SEQ_NUM_FIELD, out obj))
                seqNum = Convert.ToInt64(obj);
            if (message.TryGetValue(ProtocolConstants.BODY_FIELD, out obj))
                body =(JsonObject) obj;
            if (message.TryGetValue(ProtocolConstants.ERR_CODE_FIELD, out obj))
                code = Convert.ToInt32(obj);
            if (message.TryGetValue(ProtocolConstants.REASON_FIELD, out obj))
                reason = Convert.ToString(obj);

            if (seqNum > 0) 
            {
                if (code > 0)
                    requestError(seqNum, code, reason);
                else
                    requestSuccess(seqNum, (body != null ? new JSONMessage(body) : null));
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

        private void acknowledge(Int64 seqNum, String subId) 
        {
            if (seqNum == 0)
                return;

            JsonObject message = new JsonObject();

            message[ProtocolConstants.OP_FIELD] = ProtocolOpConstants.OP_ACK;
            message[ProtocolConstants.SEQ_NUM_FIELD] = seqNum;

            if (subId != null)
                message[ProtocolConstants.ID_FIELD] = subId;

            // queue message for writer thread
            Queue(message.ToString());
        }

        private void requestTimeout(Object stateInfo)
        {
            Int64 seqNum = (Int64)stateInfo;

            requestError(seqNum, RequestListenerConstants.REQUEST_TIMEOUT, "request timeout");
        }

        private void requestSuccess(Int64 seqNum, IMessage response) 
        {
            RequestContext ctx = null;
            if (requests.Remove(seqNum, out ctx)) 
            {
                ctx.OnSuccess(response);
            }
        }

        private void requestError(Int64 seqNum, int code, String reason) 
        {
            RequestContext ctx = null;
            if (requests.Remove(seqNum, out ctx)) 
            {
                if (!ctx.HasListener()) 
                    listener.OnError(this, code, reason);

                ctx.OnError(code, reason);
            }
        }

        private void clearRequests(int code, String reason) 
        {
            foreach (RequestContext ctx in requests.Values()) 
            {
                requests.Remove(ctx.GetSeqNum());

                ctx.OnError(code, reason);
            }
        }

        private bool scheduleReconnect() 
        {
            bool scheduled = false;

            setState(ConnectionState.RECONNECTING);

            // schedule a connect only if not previously connected
            // and there are additional URLs to try in the URL list
            //
            // schedule a reconnect only if previously connected and 
            // the number of reconnect attempts has not been exceeded
            if (Interlocked.Read(ref connected) == 0 && nextURL())
            {
                try 
                {
                    timer = new System.Timers.Timer();

                    // Hook up the Elapsed event for the timer.
                    timer.Elapsed += new ElapsedEventHandler(reconnectTask);

                    timer.AutoReset = false;
                    timer.Enabled = true;

                    scheduled = true;
                } 
                catch(Exception) 
                {
                    // failed to schedule connect
                }
            }
            else if (Interlocked.Read(ref connected) == 1 && reconnectAttempts < GetReconnectAttempts()) 
            {
                try 
                {
                    timer = new System.Timers.Timer();

                    // Hook up the Elapsed event for the timer.
                    timer.Elapsed += new ElapsedEventHandler(reconnectTask);

                    timer.AutoReset = false;
                    timer.Enabled = true;

                    // backoff once all URLs have been tried
                    if (!nextURL())
                    {
                        // add jitter by applying a randomness factor of 0.5
                        double jitter = rand.NextDouble() + 0.5;
                        // exponential backoff truncated to 30 seconds
                        long backoff = (long)(Math.Pow(2.0, reconnectAttempts++) * 1000 * jitter);
                        if (backoff > GetReconnectMaxDelay() || backoff <= 0)
                            backoff = GetReconnectMaxDelay();
                        timer.Interval = backoff;
                    }

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

        private double GetConnectTimeout() 
        {
            // defaults to 15 seconds
            double value = defaultConnectTimeout;

            if (props.ContainsKey(EFTL.PROPERTY_TIMEOUT))
            {
                string propValue = (String) props[EFTL.PROPERTY_TIMEOUT];
                double doubleValue = 0.0;
                if (Double.TryParse(propValue, out doubleValue))
                    value = doubleValue;
            }

            return value;
        }

        private int GetReconnectAttempts() 
        {
            // defaults to 256 attempts
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

        private int GetMaxPendingAcks() 
        {
            int value = 0;

            if (props.ContainsKey(EFTL.PROPERTY_MAX_PENDING_ACKS)) 
            {
                string propValue = (String) props[EFTL.PROPERTY_MAX_PENDING_ACKS];
                int intValue = 0;
                if (Int32.TryParse(propValue, out intValue))
                    value = intValue;
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

        private void setURLList(String list)
        {
            foreach (String url in list.Split('|')) {
                urlList.Add(new Uri(url));
            }

            urlIndex = 0;

            // randomize the URL list
            urlList = urlList.OrderBy(x => rand.Next()).ToList();
            //urlList = urlList.OrderBy(x => Random.value).ToList();
        }

        private void resetURLList()
        {
            urlIndex = 0;
        }

        private bool nextURL()
        {
            if (++urlIndex < urlList.Count) {
                return true;
            } else {
                urlIndex = 0;
                return false;
            }
        }

        private Uri getURL()
        {
            return urlList[urlIndex];
        }

        private void setState(ConnectionState state)
        {
            // Check of the object implements the OnStateChange callback
            Type typeInstance = this.listener.GetType();
            if (typeInstance != null)
            {
               MethodInfo methodInfo = typeInstance.GetMethod("OnStateChange");
               if (methodInfo != null)
               {
                   ParameterInfo[] parameterInfo = methodInfo.GetParameters();
                   object classInstance = Activator.CreateInstance(typeInstance, null);
                   if (parameterInfo.Length != 0)
                   {
                       var result = methodInfo.Invoke(classInstance, new object[] {this, state});
                   }
               }
            }
        }
    }
}
