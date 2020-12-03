/*
 * Copyright (c) 2001-$Date: 2020-09-29 13:29:25 -0700 (Tue, 29 Sep 2020) $ TIBCO Software Inc.
 * Licensed under a BSD-style license. Refer to [LICENSE]
 * For more information, please contact:
 * TIBCO Software Inc., Palo Alto, California, USA
 *
 * $Id: WebSocketConnection.java 128906 2020-09-29 20:29:25Z bpeterse $
 */
package com.tibco.eftl.impl;

import java.io.IOException;
import java.net.URI;
import java.net.URISyntaxException;
import java.security.KeyStore;
import java.util.ArrayList;
import java.util.Collections;
import java.util.Enumeration;
import java.util.Iterator;
import java.util.Map;
import java.util.Properties;
import java.util.Timer;
import java.util.TimerTask;
import java.util.concurrent.BlockingQueue;
import java.util.concurrent.ConcurrentHashMap;
import java.util.concurrent.ConcurrentSkipListMap;
import java.util.concurrent.LinkedBlockingQueue;
import java.util.concurrent.TimeUnit;
import java.util.concurrent.atomic.AtomicBoolean;
import java.util.concurrent.atomic.AtomicLong;

import javax.net.ssl.TrustManager;
import javax.net.ssl.TrustManagerFactory;

import com.tibco.eftl.CompletionListener;
import com.tibco.eftl.Connection;
import com.tibco.eftl.ConnectionListener;
import com.tibco.eftl.RequestListener;
import com.tibco.eftl.EFTL;
import com.tibco.eftl.KVMap;
import com.tibco.eftl.KVMapListener;
import com.tibco.eftl.Message;
import com.tibco.eftl.SubscriptionListener;
import com.tibco.eftl.Version;
import com.tibco.eftl.json.JsonArray;
import com.tibco.eftl.json.JsonObject;
import com.tibco.eftl.json.JsonValue;
import com.tibco.eftl.websocket.WebSocket;
import com.tibco.eftl.websocket.WebSocketListener;

public class WebSocketConnection implements Connection, WebSocketListener, Runnable {

    protected ArrayList<URI> urlList = new ArrayList<URI>();
    protected int urlIndex;
    protected Properties props = new Properties();
    protected ConnectionListener listener;
    protected WebSocket webSocket;
    protected TrustManager[] trustManagers;
    protected boolean trustAll;
    protected String clientId;
    protected String reconnectId;
    protected int protocol;
    protected int maxMessageSize;
    protected int reconnectAttempts;
    protected boolean qos;
    protected Thread writer;
    protected Timer reconnectTimer;
    protected AtomicBoolean connected = new AtomicBoolean();
    protected AtomicBoolean connecting = new AtomicBoolean();
    protected AtomicBoolean reconnecting = new AtomicBoolean();
    protected AtomicLong messageIdGenerator = new AtomicLong();
    protected AtomicLong requestIdGenerator = new AtomicLong();
    protected AtomicLong subscriptionIdGenerator = new AtomicLong();
    protected ConcurrentHashMap<String, Subscription> subscriptions = 
            new ConcurrentHashMap<String, Subscription>();
    protected ConcurrentSkipListMap<Long, Request> requests =
            new ConcurrentSkipListMap<Long, Request>();
    protected BlockingQueue<Request> writeQueue = 
            new LinkedBlockingQueue<Request>();
    protected Object writeLock = new Object();
    protected Object processLock = new Object();
    
    private static final Request DISCONNECT = new Request(new String());
    
    public WebSocketConnection(String uri, ConnectionListener listener) 
    {
        try 
        {
            setURLList(uri);

            this.listener = listener;
            
            // default quality of service
            props.setProperty(ProtocolConstants.QOS_FIELD, Boolean.toString(true));
        } 
        catch (URISyntaxException e) 
        {
            throw new IllegalArgumentException(uri);
        }
    }
    
    public void setTrustStore(KeyStore keyStore)
    {
        if (keyStore != null)
        {
            try
            {
                TrustManagerFactory factory = TrustManagerFactory.getInstance(TrustManagerFactory.getDefaultAlgorithm());
                factory.init(keyStore);
                trustManagers = factory.getTrustManagers();
            }
            catch (Exception e)
            {
                throw new RuntimeException(e);
            }
        }
        else
        {
            trustManagers = null;
        }
    }
    
    public void setTrustAll(boolean trustAll)
    {
        this.trustAll = trustAll;
    }

    public void connect(Properties props) 
    {
        if (connecting.compareAndSet(false, true))
        {
            if (props != null)
                this.props.putAll(props);
        
            int connectTimeout = getConnectTimeout();

            webSocket = new WebSocket(getURL(), this);
            webSocket.setProtocol(ProtocolConstants.EFTL_WS_PROTOCOL);
            webSocket.setTrustManagers(trustManagers);
            webSocket.setTrustAll(trustAll);
            webSocket.setConnectTimeout(connectTimeout, TimeUnit.MILLISECONDS);
            webSocket.setSocketTimeout(connectTimeout, TimeUnit.MILLISECONDS);
            webSocket.open();
        }
    }
    
    /**
     * Force the client to disconnect bypassing all eFTL and WebSocket 
     * protocols. This method primarily exists for testing various 
     * reconnect scenarios.
     */
    public void forceDisconnect()
    {
        if (webSocket != null)
            webSocket.forceClose();
    }
    
    @Override
    public String getClientId()
    {
        return clientId;
    }
    
    @Override
    public void reconnect(Properties props) 
    {
        if (isConnected())
            return;
        
        // cancel auto-reconnects
        cancelReconnect();
        
        // set only when auto-reconnecting
        reconnecting.set(false);
        
        connect(props);
    }

    @Override
    public void disconnect() 
    {
        if (connected.compareAndSet(true, false))
        {
            if (cancelReconnect())
            {
                // clear the unacknowledged messages
                clearRequests(CompletionListener.PUBLISH_FAILED, "Disconnected");

                // a pending reconnect was cancelled, 
                // tell the user we've disconnected
                listener.onDisconnect(this, ConnectionListener.NORMAL, null);
            }
            else
            {
                // invoked in a thread to prevent potential
                // deadlocks with message callbacks
                new Thread() {
                    public void run() {
                        // synchronize to prevent a disconnect 
                        // from occurring between the message 
                        // callback and message acknowledgment
                        synchronized (processLock) {
                            queue(DISCONNECT);
                        }
                    }
                }.start();
            }
        }
    }

    @Override
    public boolean isConnected() 
    {
        return (connected.get());
    }

    @Override
    public Message createMessage() 
    {
        return new JSONMessage();
    }

    @Override
    public KVMap createKVMap(final String name) 
    {
        return new KVMap() 
        {
            @Override
            public void set(String key, Message value, KVMapListener listener) 
            {
                if (!isConnected())
                    throw new IllegalStateException("not connected");

                JsonObject envelope = new JsonObject();

                // synchronized to ensure message sequence numbers are ordered
                synchronized (writeLock)
                {
                    long seqNum = messageIdGenerator.incrementAndGet();

                    envelope.put(ProtocolConstants.OP_FIELD, ProtocolConstants.OP_MAP_SET);
                    if (qos)
                        envelope.put(ProtocolConstants.SEQ_NUM_FIELD, seqNum);
                    if (name != null)
                        envelope.put(ProtocolConstants.MAP_FIELD, name);
                    if (key != null)
                        envelope.put(ProtocolConstants.KEY_FIELD, key);
                    if (value != null)
                        envelope.put(ProtocolConstants.VALUE_FIELD, ((JSONMessage) value).toJsonObject());

                    MapRequest request = new MapRequest(seqNum, envelope.toString(), key, value, listener);

                    if (maxMessageSize > 0 && request.getJson().length() > maxMessageSize)
                        throw new IllegalArgumentException("maximum message size exceeded");

                    requests.put(seqNum, request);

                    queue(request);
                }
            }

            @Override
            public void get(final String key, final KVMapListener listener) 
            {
                if (!isConnected())
                    throw new IllegalStateException("not connected");

                JsonObject envelope = new JsonObject();

                // synchronized to ensure message sequence numbers are ordered
                synchronized (writeLock)
                {
                    long seqNum = messageIdGenerator.incrementAndGet();

                    envelope.put(ProtocolConstants.OP_FIELD, ProtocolConstants.OP_MAP_GET);
                    if (qos)
                        envelope.put(ProtocolConstants.SEQ_NUM_FIELD, seqNum);
                    if (name != null)
                        envelope.put(ProtocolConstants.MAP_FIELD, name);
                    if (key != null)
                        envelope.put(ProtocolConstants.KEY_FIELD, key);

                    MapRequest request = new MapRequest(seqNum, envelope.toString(), key, listener);

                    requests.put(seqNum, request);

                    queue(request);
                }
            }

            @Override
            public void remove(final String key, final KVMapListener listener) 
            {
                if (!isConnected())
                    throw new IllegalStateException("not connected");

                JsonObject envelope = new JsonObject();

                // synchronized to ensure message sequence numbers are ordered
                synchronized (writeLock)
                {
                    long seqNum = messageIdGenerator.incrementAndGet();

                    envelope.put(ProtocolConstants.OP_FIELD, ProtocolConstants.OP_MAP_REMOVE);
                    if (qos)
                        envelope.put(ProtocolConstants.SEQ_NUM_FIELD, seqNum);
                    if (name != null)
                        envelope.put(ProtocolConstants.MAP_FIELD, name);
                    if (key != null)
                        envelope.put(ProtocolConstants.KEY_FIELD, key);

                    MapRequest request = new MapRequest(seqNum, envelope.toString(), key, listener);

                    requests.put(seqNum, request);

                    queue(request);
                }
            }
        };
    }
    
    @Override
    public void removeKVMap(final String name)
    {
        if (!isConnected())
            throw new IllegalStateException("not connected");

        JsonObject envelope = new JsonObject();
        
        envelope.put(ProtocolConstants.OP_FIELD, ProtocolConstants.OP_MAP_DESTROY);
        if (name != null)
            envelope.put(ProtocolConstants.MAP_FIELD, name);
        
        queue(envelope.toString());
    }
    
    @Override
    public void sendRequest(Message request, double timeout, RequestListener listener)
    {
        if (!isConnected())
            throw new IllegalStateException("not connected");

        JsonObject envelope = new JsonObject();
        
        // synchronized to ensure message sequence numbers are ordered
        synchronized (writeLock)
        {
            final long seqNum = messageIdGenerator.incrementAndGet();
                
            envelope.put(ProtocolConstants.OP_FIELD, ProtocolConstants.OP_REQUEST);
            envelope.put(ProtocolConstants.SEQ_NUM_FIELD, seqNum);
            envelope.put(ProtocolConstants.BODY_FIELD, ((JSONMessage) request).toJsonObject());
                
            SendRequest sendRequest = new SendRequest(seqNum, envelope.toString(), request, listener);
            
            if (protocol < 1)
                throw new UnsupportedOperationException("send request is not supported with this server");

            if (maxMessageSize > 0 && sendRequest.getJson().length() > maxMessageSize)
                throw new IllegalArgumentException("maximum message size exceeded");

            sendRequest.setTimeout((long)(timeout * 1000), new TimerTask() {
                    @Override
                    public void run() {
                        requestTimeout(seqNum);
                    }
            });

            requests.put(seqNum, sendRequest);
                
            queue(sendRequest);
        }
    }

    @Override
    public void sendReply(Message reply, Message request, CompletionListener listener)
    {
        if (!isConnected())
            throw new IllegalStateException("not connected");

        if (((JSONMessage) request).replyTo == null)
            throw new IllegalArgumentException("not a request message");
            
        JsonObject envelope = new JsonObject();
        
        // synchronized to ensure message sequence numbers are ordered
        synchronized (writeLock)
        {
            long seqNum = messageIdGenerator.incrementAndGet();
                
            envelope.put(ProtocolConstants.OP_FIELD, ProtocolConstants.OP_REPLY);
            envelope.put(ProtocolConstants.SEQ_NUM_FIELD, seqNum);
            envelope.put(ProtocolConstants.TO_FIELD, ((JSONMessage) request).replyTo);
            envelope.put(ProtocolConstants.REQ_ID_FIELD, ((JSONMessage) request).reqId);
            envelope.put(ProtocolConstants.BODY_FIELD, ((JSONMessage) reply).toJsonObject());
                
            Publish publish = new Publish(seqNum, envelope.toString(), reply, listener);
            
            if (protocol < 1)
                throw new UnsupportedOperationException("send reply is not supported with this server");

            if (maxMessageSize > 0 && publish.getJson().length() > maxMessageSize)
                throw new IllegalArgumentException("maximum message size exceeded");

            requests.put(seqNum, publish);
                
            queue(publish);
        }
    }

    @Override
    public void publish(Message message) 
    {
        publish(message, null);
    }

    @Override
    public void publish(Message message, CompletionListener listener) 
    {
        if (!isConnected())
            throw new IllegalStateException("not connected");

        JsonObject envelope = new JsonObject();
        
        // synchronized to ensure message sequence numbers are ordered
        synchronized (writeLock)
        {
            long seqNum = messageIdGenerator.incrementAndGet();
                
            envelope.put(ProtocolConstants.OP_FIELD, ProtocolConstants.OP_MESSAGE);
            envelope.put(ProtocolConstants.BODY_FIELD, ((JSONMessage) message).toJsonObject());
                
            if (qos)
            {
                envelope.put(ProtocolConstants.SEQ_NUM_FIELD, seqNum);
            }
                
            Publish publish = new Publish(seqNum, envelope.toString(), message, listener);
            
            if (maxMessageSize > 0 && publish.getJson().length() > maxMessageSize)
                throw new IllegalArgumentException("maximum message size exceeded");

            requests.put(seqNum, publish);
                
            queue(publish);
        }
    }
    
    @Override
    public String subscribe(String matcher, SubscriptionListener listener) 
    {
        return subscribe(matcher, null, null, listener);
    }

    @Override
    public String subscribe(String matcher, String durable, SubscriptionListener listener)
    {
        return subscribe(matcher, durable, null, listener);
    }
    
    @Override
    public String subscribe(String matcher, String durable, Properties props, SubscriptionListener listener) 
    {
        if (!isConnected())
            throw new IllegalStateException("not connected");

        String subscriptionId = String.valueOf(subscriptionIdGenerator.incrementAndGet());
        subscribe(subscriptionId, matcher, durable, props, listener);
        return subscriptionId;
    }

    private void subscribe(String subscriptionId, String matcher, String durable, Properties props, SubscriptionListener listener)
    {
        Subscription subscription = new Subscription(subscriptionId, matcher, durable, props, listener);
        
        subscriptions.put(subscription.getSubscriptionId(), subscription);
        subscribe(subscription);
    }
    
    private void subscribe(Subscription subscription) 
    {
        JsonObject message = new JsonObject();
        
        message.put(ProtocolConstants.OP_FIELD, ProtocolConstants.OP_SUBSCRIBE);
        message.put(ProtocolConstants.ID_FIELD, subscription.getSubscriptionId());
        if (subscription.getMatcher() != null)
            message.put(ProtocolConstants.MATCHER_FIELD, subscription.getMatcher());
        if (subscription.getDurable() != null)
            message.put(ProtocolConstants.DURABLE_FIELD, subscription.getDurable());
        if (subscription.getProperties() != null)
        {
            for (Map.Entry<Object, Object> entry : subscription.getProperties().entrySet()) {
                message.put((String) entry.getKey(), entry.getValue());
            }
        }
        
        queue(message.toString());
    } 
    
    @Override
    public void closeSubscription(String subscriptionId) 
    {
        if (!isConnected())
            throw new IllegalStateException("not connected");

        if (protocol < 1)
            throw new UnsupportedOperationException("close subscription is not supported with this server");

        Subscription subscription = subscriptions.remove(subscriptionId);
        if (subscription != null)
        {
            JsonObject message = new JsonObject();
            
            message.put(ProtocolConstants.OP_FIELD, ProtocolConstants.OP_UNSUBSCRIBE);
            message.put(ProtocolConstants.ID_FIELD, subscription.getSubscriptionId());
            message.put(ProtocolConstants.DEL_FIELD, false);

            queue(message.toString());
        }
    }

    @Override
    public void closeAllSubscriptions()
    {
        for (Enumeration<String> e = subscriptions.keys(); e.hasMoreElements();)
        {
            closeSubscription(e.nextElement());
        }
    }
    
    @Override
    public void unsubscribe(String subscriptionId) 
    {
        if (!isConnected())
            throw new IllegalStateException("not connected");

        Subscription subscription = subscriptions.remove(subscriptionId);
        if (subscription != null)
        {
            JsonObject message = new JsonObject();
            
            message.put(ProtocolConstants.OP_FIELD, ProtocolConstants.OP_UNSUBSCRIBE);
            message.put(ProtocolConstants.ID_FIELD, subscription.getSubscriptionId());

            queue(message.toString());
        }
    }

    @Override
    public void unsubscribeAll()
    {
        for (Enumeration<String> e = subscriptions.keys(); e.hasMoreElements();)
        {
            unsubscribe(e.nextElement());
        }
    }
    
    @Override
    public void acknowledge(Message message)
    {
        if (!isConnected())
            throw new IllegalStateException("not connected");
        
        acknowledge(((JSONMessage) message).seqNum, null);
    }
    
    @Override
    public void acknowledgeAll(Message message)
    {
        if (!isConnected())
            throw new IllegalStateException("not connected");
        
        acknowledge(((JSONMessage) message).seqNum, ((JSONMessage) message).subId);
    }
    
    @Override
    public void onOpen() 
    {
        String user = props.getProperty(EFTL.PROPERTY_USERNAME, null);
        String password = props.getProperty(EFTL.PROPERTY_PASSWORD, null);
        String identifier = props.getProperty(EFTL.PROPERTY_CLIENT_ID, null);
        
        String userInfo = getURL().getUserInfo();
        
        if (userInfo != null)
        {
            String[] tokens = userInfo.split(":");
            
            user = tokens[0];
            
            if (tokens.length > 1)
                password = tokens[1];
        }
        
        String query = getURL().getQuery();
        
        if (query != null)
        {
            String[] tokens = query.split("&");

            for (int i = 0; i < tokens.length; i++)
            {
                if (tokens[i].startsWith("clientId=")) 
                {
                    identifier = tokens[i].substring("clientId=".length());
                }
            }
        }
        
        JsonObject message = new JsonObject();
        
        message.put(ProtocolConstants.OP_FIELD, ProtocolConstants.OP_LOGIN);
        message.put(ProtocolConstants.PROTOCOL, ProtocolConstants.PROTOCOL_VERSION);
        message.put(ProtocolConstants.CLIENT_TYPE_FIELD, "java");
        message.put(ProtocolConstants.CLIENT_VERSION_FIELD, Version.EFTL_VERSION_STRING_SHORT);
        
        if (user != null)
            message.put(ProtocolConstants.USER_FIELD, user);
        
        if (password != null)
            message.put(ProtocolConstants.PASSWORD_FIELD, password);
        
        if (clientId != null && reconnectId != null)
        {
            message.put(ProtocolConstants.CLIENT_ID_FIELD, clientId);
            message.put(ProtocolConstants.ID_TOKEN_FIELD, reconnectId);
        }
        else if (identifier != null)
        {
            message.put(ProtocolConstants.CLIENT_ID_FIELD, identifier);
        }
        
        int maxPendingAcks = getMaxPendingAcks();

        if (maxPendingAcks > 0) 
        {
            message.put(ProtocolConstants.MAX_PENDING_ACKS, maxPendingAcks);
        }

        JsonObject loginOptions = new JsonObject();

        // add resume when auto-reconnecting
        if (reconnecting.get())
            loginOptions.put(ProtocolConstants.RESUME_FIELD, "true");
        
        // add user properties
        for (String name : props.stringPropertyNames())
        {
            if (name.equals(EFTL.PROPERTY_USERNAME) ||
                name.equals(EFTL.PROPERTY_PASSWORD) ||
                name.equals(EFTL.PROPERTY_CLIENT_ID))
                continue;
            
            loginOptions.put(name,  props.getProperty(name));
        }
        
        message.put(ProtocolConstants.LOGIN_OPTIONS_FIELD, loginOptions);

        try 
        {
            webSocket.send(message.toString());
        }
        catch (IOException e)
        {
            // TODO
        }
        
        // cancel auto-reconnects
        cancelReconnect();
    }

    @Override
    public void onClose(int code, String reason) 
    {
        if (connecting.compareAndSet(true, false))
        {
            // Reconnect when the close code reflects a server restart.
            if (code != ConnectionListener.RESTART || !scheduleReconnect())
            {
                // no longer connected
                connected.set(false);
            
                // stop the writer thread
                if (writer != null)
                    writer.interrupt();

                // clear the unacknowledged messages
                clearRequests(CompletionListener.PUBLISH_FAILED, "Closed");
            
                listener.onDisconnect(this, code, reason);
            }
            else
            { 
                // stop the writer thread
                if (writer != null)
                    writer.interrupt();
            }
        }
    }

    @Override
    public void onError(Throwable cause) 
    {
        if (connecting.compareAndSet(true, false))
        {
            if (!scheduleReconnect())
            {
                // no longer connected
                connected.set(false);
                
                // stop the writer thread
                if (writer != null)
                    writer.interrupt();

                // cancel auto-reconnects
                cancelReconnect();
                
                // clear the unacknowledged messages
                clearRequests(CompletionListener.PUBLISH_FAILED, "Error");
                
                listener.onDisconnect(this, ConnectionListener.CONNECTION_ERROR, cause.getMessage());
            }
            else
            {
                // stop the writer thread
                if (writer != null)
                    writer.interrupt();
            }
        }
    }

    @Override
    public void onMessage(String text) 
    {
        Object value = JsonValue.parse(text);
            
        if (value instanceof JsonObject)
        {
            JsonObject message = (JsonObject) value;

            if (message.containsKey(ProtocolConstants.OP_FIELD))
            {
                Number op = (Number) message.get(ProtocolConstants.OP_FIELD);
                    
                switch(op.intValue())
                {
                case ProtocolConstants.OP_HEARTBEAT:
                    handleHeartbeat(message);
                    break;
                case ProtocolConstants.OP_WELCOME:
                    handleWelcome(message);
                    break;
                case ProtocolConstants.OP_SUBSCRIBED:
                    handleSubscribed(message);
                    break;
                case ProtocolConstants.OP_UNSUBSCRIBED:
                    handleUnsubscribed(message);
                    break;
                case ProtocolConstants.OP_EVENT:
                    handleMessage(message);
                    break;
                case ProtocolConstants.OP_ERROR:
                    handleError(message);
                    break;
                case ProtocolConstants.OP_ACK:
                    handleAck(message);
                    break;
                case ProtocolConstants.OP_REQUEST_REPLY:
                    handleReply(message);
                    break;
                case ProtocolConstants.OP_MAP_RESPONSE:
                    handleMapResponse(message);
                    break;
                }
            }
        }
    }

    @Override
    public void onMessage(byte[] data, int offset, int length) 
    {
        // ignore
    }

    @Override
    public void onPong(byte[] data, int offset, int length) 
    {
        // ignore
    }
    
    private void handleHeartbeat(JsonObject message)
    {
        // queue message for writer thread
        queue(message.toString());
    }
    
    private void handleWelcome(JsonObject message)
    {
        // a non-null reconnect token indicates a prior connection
        boolean invokeOnReconnect = (reconnectId != null);

        if (message.containsKey(ProtocolConstants.PROTOCOL))
            protocol = ((Number) message.get(ProtocolConstants.PROTOCOL)).intValue();
        else
            protocol = 0;

        clientId = (String) message.get(ProtocolConstants.CLIENT_ID_FIELD);
        reconnectId = (String) message.get(ProtocolConstants.ID_TOKEN_FIELD);
        maxMessageSize = ((Number) message.get(ProtocolConstants.MAX_SIZE_FIELD)).intValue();
        qos = Boolean.parseBoolean((String) message.get(ProtocolConstants.QOS_FIELD));
        int timeout = (int) (1000*(((Number) message.get(ProtocolConstants.TIMEOUT_FIELD)).doubleValue()));
        boolean resume = Boolean.parseBoolean((String) message.get(ProtocolConstants.RESUME_FIELD));
        
        webSocket.setSocketTimeout(timeout, TimeUnit.MILLISECONDS);
        
        // connected
        connected.set(true);

        // reset reconnect attempts
        reconnectAttempts = 0;

        // reset URL list to start auto-reconnect attempts from the beginning
        resetURLList();
        
        // synchronized to ensure message sequence numbers are ordered
        synchronized(writeLock)
        {
            // purge the outbound queue
            writeQueue.clear();
        
            // start writer thread
            writer = new Thread(this, "EFTL Writer");
            writer.start();
        
            // repair subscriptions
            for (Subscription subscription : subscriptions.values())
            {
                if (!resume)
                    subscription.setLastSeqNum(0);
                
                subscribe(subscription);
            }
    
            // re-send unacknowledged messages
            for (Request request : requests.values())
                queue(request);
        }
        
        // invoke callback if not auto-reconnecting
        if (!invokeOnReconnect)
            listener.onConnect(this);
        else if (!reconnecting.get())
            listener.onReconnect(this);
    }
    
    private void handleSubscribed(JsonObject message)
    {
        String subscriptionId = (String) message.get(ProtocolConstants.ID_FIELD);

        // A subscription request has succeeded.
        
        Subscription subscription = subscriptions.get(subscriptionId);
        if (subscription != null && subscription.isPending())
        {
            subscription.setPending(false);
            subscription.getListener().onSubscribe(subscriptionId);
        }
    }
    
    private void handleUnsubscribed(JsonObject message)
    {
        String subscriptionId = (String) message.get(ProtocolConstants.ID_FIELD);
        int code = ((Number) message.get(ProtocolConstants.ERR_CODE_FIELD)).intValue();
        String reason = (String) message.get(ProtocolConstants.REASON_FIELD);

        // A subscription request has failed.
        // Possible errors include:
        //   LISTENS_DISABLED listens are disabled
        //   LISTENS_DISALLOWED listens are disabled for this user
        //   SUBSCRIPTION_FAILED an internal error occurred
        
        Subscription subscription = subscriptions.get(subscriptionId);

        // remove the subscription only if it's untryable
        if (code == SubscriptionListener.SUBSCRIPTION_INVALID)
            subscriptions.remove(subscriptionId);

        if (subscription != null)
        {
            subscription.setPending(true);
            subscription.getListener().onError(subscriptionId, code, reason);
        }
    }
    
    private void handleMessage(JsonObject envelope)
    {
        synchronized (processLock)
        {
            String to = (String) envelope.get(ProtocolConstants.TO_FIELD);
            String replyTo = (String) envelope.get(ProtocolConstants.REPLY_TO_FIELD);
            Long seqNum = (Long) envelope.get(ProtocolConstants.SEQ_NUM_FIELD);
            Long reqId = (Long) envelope.get(ProtocolConstants.REQ_ID_FIELD);
            Long msgId = (Long) envelope.get(ProtocolConstants.STORE_MSG_ID_FIELD);
            Long deliveryCount = (Long) envelope.get(ProtocolConstants.DELIVERY_COUNT_FIELD);
            JsonObject body = (JsonObject) envelope.get(ProtocolConstants.BODY_FIELD);

            // The message will be processed if there is no sequence number or 
            // if the sequence number is greater than the last received sequence number.

            Subscription subscription = subscriptions.get(to);
            if (subscription != null)
            {
                if (seqNum == null || seqNum.longValue() > subscription.getLastSeqNum())
                {
                    Message message = new JSONMessage(body);

                    if (seqNum != null)
                        ((JSONMessage) message).setReceipt(seqNum.longValue(), subscription.getSubscriptionId());
                    if (replyTo != null)
                        ((JSONMessage) message).setReplyTo(replyTo, (reqId != null ? reqId.longValue() : 0));
                    if (msgId != null)
                        ((JSONMessage) message).setStoreMessageId(msgId.longValue());
                    if (deliveryCount != null)
                        ((JSONMessage) message).setDeliveryCount(deliveryCount.longValue());

                    try
                    {
                        subscription.getListener().onMessages(new Message[] {message});
                    }
                    catch (Exception e)
                    {
                        // catch and discard exceptions thrown by the listener
                    }
                    
                    // track the last received sequence number
                    if (subscription.isAutoAck() && seqNum != null)
                        subscription.setLastSeqNum(seqNum.longValue());                    
                }

                // auto-acknowledge the message
                if (subscription.isAutoAck() && seqNum != null)
                    acknowledge(seqNum.longValue(), subscription.getSubscriptionId());
            }
        }
    }
    
    private void handleError(JsonObject message)
    {
        int code = ((Number) message.get(ProtocolConstants.ERR_CODE_FIELD)).intValue();
        String reason = (String) message.get(ProtocolConstants.REASON_FIELD);
        
        // The server is sending an error to the client.
        // Possible errors include:
        //   BAD_SUBSCRIBER_ID a subscription ID is null or does not start 
        //                     with the client ID
        //   SEND_DISALLOWED sending is disabled for this user
        
        listener.onError(this, code, reason);
    }
    
    private void handleAck(JsonObject message)
    {
        Long seqNum = (Long) message.get(ProtocolConstants.SEQ_NUM_FIELD);
        Number code = (Number) message.get(ProtocolConstants.ERR_CODE_FIELD);
        String reason = (String) message.get(ProtocolConstants.REASON_FIELD);
        
        if (seqNum != null)
        {
            if (code != null)
                requestError(seqNum, code.intValue(), reason);
            else
                requestSuccess(seqNum, null);
        }
    }
    
    private void handleReply(JsonObject message)
    {
        Long seqNum = (Long) message.get(ProtocolConstants.SEQ_NUM_FIELD);
        JsonObject body = (JsonObject) message.get(ProtocolConstants.BODY_FIELD);
        Number code = (Number) message.get(ProtocolConstants.ERR_CODE_FIELD);
        String reason = (String) message.get(ProtocolConstants.REASON_FIELD);
        
        if (seqNum != null)
        {
            if (code != null)
                requestError(seqNum, code.intValue(), reason);
            else
                requestSuccess(seqNum, (body != null ? new JSONMessage(body) : null));
        }
    }
    
    private void handleMapResponse(JsonObject message)
    {
        Long seqNum = (Long) message.get(ProtocolConstants.SEQ_NUM_FIELD);
        JsonObject value = (JsonObject) message.get(ProtocolConstants.VALUE_FIELD);
        Number code = (Number) message.get(ProtocolConstants.ERR_CODE_FIELD);
        String reason = (String) message.get(ProtocolConstants.REASON_FIELD);
        
        // Remove all unacknowledged messages with a sequence number equal to
        // or less than the sequence number contained within the ack message.
        
        if (seqNum != null)
        {
            if (code != null)
                requestError(seqNum, code.intValue(), reason);
            else
                requestSuccess(seqNum, (value != null ? new JSONMessage(value) : null));
        }
    }
    
    private void acknowledge(long seqNum, String subId)
    {
        if (seqNum == 0)
            return;
        
        JsonObject message = new JsonObject();
        
        message.put(ProtocolConstants.OP_FIELD, ProtocolConstants.OP_ACK);
        message.put(ProtocolConstants.SEQ_NUM_FIELD, seqNum);
        
        if (subId != null)
            message.put(ProtocolConstants.ID_FIELD, subId);
        
        // queue message for writer thread
        queue(message.toString());
    }
    
    private void requestTimeout(Long seqNum)
    {
        requestError(seqNum, RequestListener.REQUEST_TIMEOUT, "request timeout");
    }
    
    private void requestSuccess(Long seqNum, Message response)
    {
        Request request = requests.remove(seqNum);
        if (request != null)
            request.onSuccess(response);
    }
    
    private void requestError(Long seqNum, int code, String reason)
    {
        Request request = requests.remove(seqNum);
        if (request != null && request.hasListener())
            request.onError(code, reason);
        else
            listener.onError(this, code, reason);
    }
    
    private void clearRequests(int code, String reason)
    {
        for (Long key : requests.keySet())
        {
            Request request = requests.remove(key);
            request.onError(code, reason);
        }
    }
    
    private boolean scheduleReconnect()
    {
        boolean scheduled = false;

        // schedule a connect only if not previously connected
        // and there are additional URLs to try in the URL list
        //
        // schedule a reconnect only if previously connected and 
        // the number of reconnect attempts has not been exceeded
        if (!connected.get() && nextURL())
        {
            scheduleReconnect(0);
            
            scheduled = true;            
        }
        else if (connected.get() && reconnectAttempts < getReconnectAttempts())
        {
            try 
            {
                long backoff = 0;
                
                // backoff once all URLs have been tried
                if (!nextURL())
                {
                    // add jitter by applying a randomness factor of 0.5
                    double jitter = Math.random() + 0.5;
                    // exponential backoff truncated to max delay
                    backoff = Math.min((long) (Math.pow(2.0, reconnectAttempts++) * 1000 * jitter), getReconnectMaxDelay());
                }

                // set only when auto-reconnecting
                reconnecting.set(true);

                scheduleReconnect(backoff);
                
                scheduled = true;
            }
            catch (Exception e)
            {
                // failed to schedule reconnect
            }
        }
        
        return scheduled;
    }

    private synchronized void scheduleReconnect(long delay)
    {
        if (reconnectTimer == null)
            reconnectTimer = new Timer("EFTL Reconnect");
        
        reconnectTimer.schedule(new TimerTask() 
        {
            @Override
            public void run() {
                connect(null);
            }            
        }, delay);        
    }
    
    private synchronized boolean cancelReconnect()
    {
        boolean cancelled = false;
        
        if (reconnectTimer != null) 
        {
            reconnectTimer.cancel();
            reconnectTimer = null;
            
            cancelled = true;
        }
        
        return cancelled;
    }
    
    private int getConnectTimeout()
    {
        // defaults to 15 seconds
        return (int)(Double.parseDouble(props.getProperty(EFTL.PROPERTY_TIMEOUT, "15.0")) * 1000.0);
    }
    
    private int getReconnectAttempts()
    {
        // defaults to 256 attempts
        int value = 0;
        try
        {
            value = Integer.parseInt(props.getProperty(EFTL.PROPERTY_AUTO_RECONNECT_ATTEMPTS, "256"));
        }
        catch (Exception e)
        {
            value = 256;
        }
        return value;
    }

    private long getReconnectMaxDelay()
    {
        // defaults to 30 seconds (30000ms)
        long value = 0;
        try
        {
            value =  (long) (Double.parseDouble(props.getProperty(EFTL.PROPERTY_AUTO_RECONNECT_MAX_DELAY, "30.0")) * 1000.0);
        }
        catch (Exception e)
        {
            value = 30000;
        }
        return value;
    }

    private int getMaxPendingAcks()
    {
        int value = 0;
        try
        {
            value = Integer.parseInt(props.getProperty(EFTL.PROPERTY_MAX_PENDING_ACKS, "0"));
        }
        catch (Exception e)
        {
            value = 0;
        }
        return value;
    }

    private void queue(String text)
    {
        queue(new Request(text));
    }
    
    private void queue(Request request)
    {
        writeQueue.offer(request);
    }
    
    @Override
    public void run() {
        try
        {
            while (webSocket.isConnected()) 
            {
                Request request = writeQueue.take();
                
                if (request == DISCONNECT)
                {
                    // send a disconnect message and close
                    JsonObject message = new JsonObject();
                    message.put(ProtocolConstants.OP_FIELD, ProtocolConstants.OP_DISCONNECT);
                    webSocket.send(message.toString());
                    webSocket.close();
                }
                else
                {
                    webSocket.send(request.getJson());
                    
                    if (!qos && request.getSeqNum() > 0)
                    {
                        requestSuccess(request.getSeqNum(), null);
                    }
                }
            }
        }
        catch (InterruptedException e)
        {
            Thread.currentThread().interrupt();
        }
        catch (IllegalStateException e)
        {
            // connection has been closed
        }
        catch (Exception e)
        {
            // TODO
        }
    }

    private void setURLList(String list) throws URISyntaxException
    {
        for (String url : list.split("\\|")) {
            this.urlList.add(new URI(url));
        }
        
        urlIndex = 0;
        
        // randomize the URL list
        Collections.shuffle(urlList);        
    }

    private void resetURLList()
    {
        urlIndex = 0;
    }
    
    private boolean nextURL()
    {
        if (++urlIndex < urlList.size()) {
            return true;
        } else {
            urlIndex = 0;
            return false;
        }
    }
    
    private URI getURL()
    {
        return urlList.get(urlIndex);
    }
}
