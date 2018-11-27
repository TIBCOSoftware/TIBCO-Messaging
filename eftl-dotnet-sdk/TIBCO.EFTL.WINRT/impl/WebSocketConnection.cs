/*
 * Copyright (c) 2009-$Date: 2017-07-12 12:07:25 -0500 (Wed, 12 Jul 2017) $ TIBCO Software Inc.
 * Licensed under a BSD-style license. Refer to [LICENSE]
 * For more information, please contact:
 * TIBCO Software Inc., Palo Alto, California, USA
 *
 * $Id: WebSocketConnection.cs 94582 2017-07-12 17:07:25Z bpeterse $
 */
using System;
using System.Collections.Generic;
using System.Linq;
using System.Threading;
using Windows.System.Threading;

namespace TIBCO.EFTL
{
    public class WebSocketConnection: TIBCO.EFTL.IConnection, TIBCO.EFTL.WebSocketListener,  IDisposable
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
        protected int       reconnectAttempts;
        protected long      lastSequenceNumber;
        protected Thread    writer;
        protected           ThreadPoolTimer     timer;

        Queue<PublishContext> writeQueue = new Queue<PublishContext>();
        bool writerInterrupted = false;
        Object writeQueueLock = new Object();
        Dictionary<String, BasicSubscription> subscriptions = new Dictionary<String, BasicSubscription>();
        protected SkipList sendList = new SkipList();

        private static readonly PublishContext DISCONNECT = new PublishContext("");

        public WebSocketConnection(String uri, IConnectionListener listener)
        {
            this.uri      = new Uri(uri);
            this.listener = listener;
            this.props    = new Hashtable();
            
            props.Add(ProtocolConstants.QOS_FIELD, "true");
        }

        public void Connect(Hashtable p)
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

        public void Reconnect(Hashtable props)
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
                    publishClear(CompletionListenerConstants.PUBLISH_FAILED, "Disconnected");
                    
                    // a pending reconnect was cancelled, 
                    // tell the user we've disconnected
                    listener.OnDisconnect(this, ConnectionListenerConstants.NORMAL, null);
                }
                else
                {
                    Queue(DISCONNECT);
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

        public void Publish(IMessage message)
        {
            Publish(message, null);
        }

        public void Publish(IMessage message, ICompletionListener listener)
        {
            if (!IsConnected())
                throw new System.InvalidOperationException("not connected");

            JsonObject envelope = new JsonObject();

            // synchronized to ensure message sequence numbers are ordered
            lock (writeLock)
            {
                long seqNum = Interlocked.Increment(ref messageIdGenerator);

                envelope.Add(ProtocolConstants.OP_FIELD, ProtocolOpConstants.OP_MESSAGE);
                envelope.Add(ProtocolConstants.BODY_FIELD, ((JSONMessage)message)._rawData());

                if (qos)
                {
                    envelope.Add(ProtocolConstants.SEQ_NUM_FIELD, seqNum);
                }

                PublishContext ctx = new PublishContext(seqNum, envelope.ToString(), message, listener);

                if (maxMessageSize > 0 && ctx.GetJson().Length > maxMessageSize)
                    throw new Exception("message exceeds maximum size of " + maxMessageSize);

                sendList.Put(seqNum, ctx);

                Queue(ctx);
            }
        }
        public String Subscribe(String matcher, ISubscriptionListener listener)
        {
            return Subscribe(matcher, null, listener);
        }

        public String Subscribe(String matcher, String durable, ISubscriptionListener listener)
        {
            if (!IsConnected())
                throw new Exception("not connected");

            String subscriptionId = clientId + ".s." + Interlocked.Increment(ref subscriptionIdGenerator);
            Subscribe(subscriptionId, matcher, durable, listener);
            return subscriptionId;
        }

        private void Subscribe(String subscriptionId, String matcher, String durable, ISubscriptionListener listener)
        {
            BasicSubscription subscription = new BasicSubscription(subscriptionId, matcher, durable, listener);
            subscription.setPending(true);
            subscriptions.Add(subscription.getSubscriptionId(), subscription);

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

            Queue(message.ToString());
        }

        public void Unsubscribe(String subscriptionId)
        {
            BasicSubscription subscription = null;
            if (!IsConnected())
                throw new Exception("not connected");

            if (subscriptions.ContainsKey(subscriptionId))
                subscription = subscriptions[subscriptionId];

            bool removed = subscriptions.Remove(subscriptionId);
            if (removed && subscription != null)
            {
                JsonObject message = new JsonObject();

                message[ProtocolConstants.OP_FIELD]  = ProtocolOpConstants.OP_UNSUBSCRIBE;
                message[ProtocolConstants.ID_FIELD]  = subscription.getSubscriptionId();

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
                Object value = null;
                String user = null;
                try
                {
                    if (props.TryGetValue(EFTL.PROPERTY_USERNAME, out value))
                        user = (String)props[EFTL.PROPERTY_USERNAME];
                }
                catch (Exception) { user = null; }

                String password = null;
                try
                {
                    if (props.TryGetValue(EFTL.PROPERTY_PASSWORD, out value))
                        password = (String)props[EFTL.PROPERTY_PASSWORD];
                }
                catch (Exception) { password = null; }

                String identifier = null;

                try
                {
                    if (props.TryGetValue(EFTL.PROPERTY_CLIENT_ID, out value))
                        identifier = (String)props[EFTL.PROPERTY_CLIENT_ID];
                }
                catch (Exception) { identifier = null; }

                String userInfo = uri.UserInfo;

                if (userInfo != null)
                {
                    String[] tokens = userInfo.Split(':');

                    user = tokens[0];

                    if (tokens.Length > 1)
                        password = tokens[1];
                }

                String query = uri.Query;

                if (query != null)
                {
                    // remove leading '?'
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
                    message[ProtocolConstants.USER_FIELD]            = user;
                if (password != null)
                    message[ProtocolConstants.PASSWORD_FIELD]        = password;
    
                if (clientId != null && reconnectId != null)
                {
                    message[ProtocolConstants.CLIENT_ID_FIELD]  = clientId;
                    message[ProtocolConstants.ID_TOKEN_FIELD]   = reconnectId;
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

                        String val = (String)props[key];

                        if (String.Compare("true", val) == 0)
                            loginOptions[key] = "true";
                        else
                            loginOptions[key] = val;
                    }
                }
                catch (Exception e)
                {
                    Console.WriteLine(e.StackTrace);
                }

                message[ProtocolConstants.LOGIN_OPTIONS_FIELD] = loginOptions;

                try 
                {
                    webSocket.send(message.ToString());
                }
                catch (Exception)
                {

                } 
            }
            catch(Exception e)
            {
                Console.WriteLine(e.StackTrace);
            }
        }

        private void writerThreadInterrupt()
        {
            try {
                lock (writeQueueLock)
                {
                    writerInterrupted = true;
                    Monitor.PulseAll(writeQueueLock);
                }
            } 
            catch(System.Threading.ThreadAbortException) { }
           catch(Exception) { }
        }

        public void OnClose(int code, String reason)
        {
            if (Interlocked.CompareExchange(ref connecting, 0, 1) == 1)
            {
                // no longer connected
                Interlocked.Exchange(ref connected, 0);

                // stop the writer thread
                writerThreadInterrupt();

                // clear the unacknowledged messages
                publishClear(CompletionListenerConstants.PUBLISH_FAILED, "Closed");

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
                    writerThreadInterrupt();

                    // clear the unacknowledged messages
                    publishClear(CompletionListenerConstants.PUBLISH_FAILED, "Error");

                    listener.OnDisconnect(this, ConnectionListenerConstants.CONNECTION_ERROR, cause.Message);
                }
                else
                {
                    // stop the writer thread
                    writerThreadInterrupt();
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
                    switch(Convert.ToInt32(op))
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

            clientId        = (String) message[ProtocolConstants.CLIENT_ID_FIELD];
            reconnectId     = (String) message[ProtocolConstants.ID_TOKEN_FIELD];
            maxMessageSize  = (long)message[ProtocolConstants.MAX_SIZE_FIELD];

            if (message.ContainsKey(ProtocolConstants.QOS_FIELD))
                qos = System.Boolean.Parse((String) message[ProtocolConstants.QOS_FIELD]);
            else
                qos = false;

            if (message.ContainsKey(ProtocolConstants.RESUME_FIELD))
                resume = System.Boolean.Parse((String)message[ProtocolConstants.RESUME_FIELD]);

            // connected
            Interlocked.Exchange(ref connected, 1);

            // reset reconnect attempts
            reconnectAttempts = 0;

            // synchronized to ensure message sequence numbers are ordered
            lock(writeLock)
            {
                lock (writeQueueLock)
                {
                    // clear the queue
                    while (writeQueue.Count > 0)
                        writeQueue.Dequeue();
                }
  
                // start writer thread
                writer  = new Thread(new ThreadStart(Run));
                writer.Name = "EFTL Writer";
                writer.IsBackground = true;
                writerInterrupted = false;
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
                    foreach (PublishContext ctx in sendList.Values())
                        Queue(ctx);
                }
                else
                {
                    // clear unacknowledged messages
                    publishClear(CompletionListenerConstants.PUBLISH_FAILED, "Reconnect");

                    // reset the last sequence number if not a reconnect
                    lastSequenceNumber = 0;
                }
            }

            // invoke callback if not auto-reconnecting
            if (!invokeOnReconnect)
                listener.OnConnect(this);
            else if (Interlocked.Read(ref reconnecting) == 1)
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
            subscription = subscriptions[subscriptionId]; 
            bool removed = subscriptions.Remove(subscriptionId);
            if (removed && subscription != null)
            {
                subscription.getListener().OnError(subscriptionId, Convert.ToInt32(code), reason);
            }
        }

        private void handleMessages(JsonArray array)
        {
            lock (processLock)
            {
                ArrayList messages = new ArrayList(array.Count);

                BasicSubscription currentSubscription = null;
                long seqNum = 0;
                long lastSeqNum = 0;
                int max = array.Count;

                for (int i = 0; i < max; i++)
                {

                    JsonObject envelope = (JsonObject) array[i];

                    String to = (string)envelope[ProtocolConstants.TO_FIELD];
                    JsonObject message = (JsonObject)envelope[ProtocolConstants.BODY_FIELD];

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

                        if (exists && subscription != null && !subscription.isPending())
                        {
                            if (currentSubscription != null && currentSubscription != subscription)
                            {
                                try
                                {
                                    IMessage[] arr = new IMessage[messages.Count];

                                    for (int j=0; j < messages.Count; j++)
                                        arr[j] = (JSONMessage)messages[j];

                                    currentSubscription.getListener().OnMessages(arr);
                                }
                                catch (Exception)
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

                        for (int i=0; i < messages.Count; i++)
                            arr[i] = (JSONMessage)messages[i];

                        currentSubscription.getListener().OnMessages(arr);
                    }
                    catch (Exception e)
                    {
                        // catch and discard exceptions thrown by the listener
                        Console.WriteLine(e.Message);
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
            lock (processLock)
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
                    if (exists && subscription != null && !subscription.isPending())
                    {
                        IMessage[] messages = {new JSONMessage(message)};
                        try
                        {
                            subscription.getListener().OnMessages(messages);
                        }
                        catch (Exception)
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
                Interlocked.Exchange(ref connecting, 0);

                // stop the writer thread
                writerThreadInterrupt();

                // clear the unacknowledged messages
                publishClear(CompletionListenerConstants.PUBLISH_FAILED, "Goodbye");

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
                    publishError(seqNum, code, reason);
                else
                    publishComplete(seqNum);
            }
        }

        private void ack(Int64 seqNum)
        {
            if (!qos)
                return;

            JsonObject message = new JsonObject();

            message[ProtocolConstants.OP_FIELD]      = ProtocolOpConstants.OP_ACK;
            message[ProtocolConstants.SEQ_NUM_FIELD] =  seqNum;

            // queue message for writer thread
            Queue(message.ToString());
        }

        private void publishComplete(Int64 seqNum)
        {
            foreach (PublishContext ctx in sendList.Values())
            {
                if (ctx.GetListener() != null)
                    ctx.GetListener().OnCompletion(ctx.GetMessage());
                if (ctx.GetSeqNum() <= seqNum)
                    sendList.Remove(ctx.GetSeqNum());
            }
        }

        private void publishError(Int64 seqNum, int code, String reason)
        {
            foreach (PublishContext ctx in sendList.Values())
            {
                if (ctx.GetListener() != null)
                    ctx.GetListener().OnError(ctx.GetMessage(), code, reason);
                else
                    listener.OnError(this, code, reason);

                if (ctx.GetSeqNum() <= seqNum)
                    sendList.Remove(ctx.GetSeqNum());
            }
        }

        private void publishClear(int code, String reason)
        {
            foreach (PublishContext ctx in sendList.Values())
            {
                if (ctx.GetListener() != null)
                    ctx.GetListener().OnError(ctx.GetMessage(), code, reason);

                sendList.Remove(ctx.GetSeqNum());
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
                    long backoff = Math.Min((long) Math.Pow(2.0, reconnectAttempts++) * 1000, 30000);

                    TimeSpan ts = new TimeSpan(backoff * 10000);
                   
                    timer = Windows.System.Threading.ThreadPoolTimer.CreateTimer(reconnectTask, ts);
                    
                    scheduled = true;
                }
                catch (Exception)
                {
                    Console.WriteLine("Schedule reconnect failed");
                    // failed to schedule reconnect
                }
            }

            return scheduled;
        }

        private void reconnectTask(ThreadPoolTimer timer)
        {
            // set only when auto-reconnecting
            Interlocked.Exchange(ref reconnecting, 1);
                
            Connect(null);
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
                ctimeout = (String)props[EFTL.PROPERTY_TIMEOUT];

            return (int)(Double.Parse(ctimeout) * 1000.0);
        }

        private int GetReconnectAttempts()
        {
            // defaults to 5 attempts
            String attempts = "5";
            if (props.ContainsKey("reconnect_attempts"))
                attempts = (String)props["reconnect_attempts"];
            
            return Int32.Parse(attempts);
        }

        private void Queue(String text)
        {
            Queue(new PublishContext(text));
        }

        private void Queue(PublishContext ctx)
        {
            bool notifyWriterThread = false;

            try {
                lock (writeQueueLock)
                {
                    if (writeQueue.Count == 0)
                        notifyWriterThread = true;

                    writeQueue.Enqueue(ctx);

                    if (notifyWriterThread)
                        Monitor.Pulse(writeQueueLock);
                }
            }
            catch (System.Threading.ThreadAbortException) { } 
            catch (Exception) { } 
        }

        public void Run() 
        {
            try
            {
                while (webSocket.isConnected())
                {
                    PublishContext ctx = null;

                    lock (writeQueueLock)
                    {
                        if (writeQueue.Count == 0)
                            Monitor.Wait(writeQueueLock);

                        if (writerInterrupted)
                            break;

                        ctx = writeQueue.Dequeue();
                    }

                    if (ctx == DISCONNECT)
                    {
                        // don't send the disconnect until the message(s) currently^M
                        // being processed have finished and acknowledged^M
                        lock (processLock)
                        {
                            // send a disconnect message and close
                            JsonObject message = new JsonObject();
                            message[ProtocolConstants.OP_FIELD] = ProtocolOpConstants.OP_DISCONNECT;
                            webSocket.send(message.ToString());
                            webSocket.close();
                        }
                    }
                    else
                    {
                        webSocket.send(ctx.GetJson());

                        if (!qos && ctx.GetSeqNum() > 0)
                        {
                            publishComplete(ctx.GetSeqNum());
                        }
                    }
                }
            }
            catch (System.Threading.ThreadAbortException) { }
            catch (Exception)
            {
                // expected
            }
        }
    }
}
