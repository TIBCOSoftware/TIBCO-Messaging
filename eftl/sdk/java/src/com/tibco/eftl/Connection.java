/*
 * Copyright (c) 2001-$Date$ TIBCO Software Inc.
 * Licensed under a BSD-style license. Refer to [LICENSE]
 * For more information, please contact:
 * TIBCO Software Inc., Palo Alto, California, USA
 *
 * $Id$
 *
 */
package com.tibco.eftl;

import java.util.Properties;

/**
 * A connection object represents a program's connection to an eFTL
 * server.
 * <p>
 * Programs use connection objects to create messages, send messages,
 * and subscribe to messages.
 * <p>
 * Programs receive connection objects through
 * {@link ConnectionListener} callbacks.
 */
public interface Connection
{
    /**
     * Gets the client identifier for this connection.
     * <p>
     * See
     * {@link EFTL#connect(String, Properties, ConnectionListener)}.
     * 
     * @return The client's identifier.
     */
    public String getClientId();
    
    /**
     * Reopen a closed connection.
     * <p>
     * You may call this method within your
     * {@link ConnectionListener#onDisconnect} method.
     * <p>
     * This call returns immediately; connecting continues asynchronously.
     * When the connection is ready to use, the eFTL library calls your
     * {@link ConnectionListener#onReconnect} callback.
     * <p>
     * Reconnecting automatically re-activates all
     * subscriptions on the connection.
     * The eFTL library invokes your
     * {@link SubscriptionListener#onSubscribe} callback
     * for each successful resubscription.
     * 
     * @param props These properties affect the connection attempt.
     *                You must supply username and password
     *                credentials each time.  All other properties
     *                remain stored from earlier connect and reconnect
     *                calls.  New values overwrite stored values.
     *
     * @see EFTL#PROPERTY_CLIENT_ID
     * @see EFTL#PROPERTY_USERNAME
     * @see EFTL#PROPERTY_PASSWORD
     * @see EFTL#PROPERTY_TIMEOUT
     * @see EFTL#PROPERTY_NOTIFICATION_TOKEN
     */
   public void reconnect(Properties props);
    
    /**
     * Disconnect from the eFTL server.
     * <p>
     * Programs may disconnect to free server resources.
     * <p>
     * This call returns immediately; disconnecting continues
     * asynchronously.
     * When the connection has closed, the eFTL library calls your
     * {@link ConnectionListener#onDisconnect} callback.
     */
    public void disconnect();
    
    /**
     * Determine whether this connection to the eFTL server is open or
     * closed.
     * 
     * @return {@code true} if this connection is open;
     * {@code false} otherwise.
     */
    public boolean isConnected();
    
    /**
     * Create a {@link Message}.
     * 
     * @return A new message object.
     */
    public Message createMessage();

    /**
     * Create a {@link KVMap}.
     * 
     * @param name Key-value map name.
     * 
     * @return A new key-value map object.
     */
    public KVMap createKVMap(String name);
    
    /**
     * Remove a key-value map.
     * 
     * @param name Key-value map name.
     */
    public void removeKVMap(String name);
    
    /**
     * Publish a request message.
     * <p>
     * This call returns immediately. When the reply is received
     * the eFTL library calls your {@link RequestListener#onReply} 
     * callback.
     * 
     * @param request The request message to publish.
     * @param timeout Seconds to wait for a reply.
     * @param listener This listener defines callback methods for
     *                 successful completion and for errors.
     *
     * @throws IllegalStateException The connection is not open.
     * @throws IllegalArgumentException The message would exceed the
     *         eFTL server's maximum message size.
     *
     * @see Message#FIELD_NAME_DESTINATION
     */
    public void sendRequest(Message request, double timeout, RequestListener listener);
    
    /**
     * Send a reply message in response to a request message.
     * <p>
     * This call returns immediately. When the send completes successfully
     * the eFTL library calls your {@link CompletionListener#onCompletion} 
     * callback.
     * 
     * @param reply The reply messasge to send.
     * @param request The request messasge.
     * @param listener This listener defines callback methods for
     *                 successful completion and for errors.
     *
     * @throws IllegalStateException The connection is not open.
     * @throws IllegalArgumentException The message would exceed the
     *         eFTL server's maximum message size.
     */
    public void sendReply(Message reply, Message request, CompletionListener listener);
    
    /**
     * Publish a one-to-many message to all subscribing clients.
     *
     * @param message Publish this message.
     *
     * @throws IllegalStateException The connection is not open.
     * @throws IllegalArgumentException The message would exceed the
     *         eFTL server's maximum message size.
     *
     * @see Message#FIELD_NAME_DESTINATION
     */
    public void publish(Message message);
    
    /**
     * Publish a one-to-many message to all subscribing clients.
     * <p>
     * This call returns immediately; publishing continues
     * asynchronously.  When the publish completes successfully, 
     * the eFTL library calls your {@link CompletionListener#onCompletion} 
     * callback.
     * 
     * @param message Publish this message.
     * @param listener This listener defines callback methods for
     *                 successful completion and for errors.
     *
     * @throws IllegalStateException The connection is not open.
     * @throws IllegalArgumentException The message would exceed the
     *         eFTL server's maximum message size.
     *
     * @see Message#FIELD_NAME_DESTINATION
     */
    public void publish(Message message, CompletionListener listener);
    
    /**
     * Subscribe to messages.
     * <p>
     * Register a subscription for one-to-many messages.
     * <p>
     * This call returns immediately; subscribing continues
     * asynchronously.  When the subscription is
     * ready to receive messages, the eFTL library calls your 
     * {@link SubscriptionListener#onSubscribe} callback.
     * <p>
     * A matcher can narrow subscription interest in the inbound
     * message stream.
     * 
     * @param matcher The subscription uses this matcher to
     *                    narrow the message stream. 
     * @param listener This listener defines callback methods for
     *                 successful subscription, message arrival, and
     *                 errors.
     *
     * @return An identifier that represents the new subscription.
     *
     * @throws IllegalStateException The connection is not open.
     *
     * @see SubscriptionListener
     * @see #unsubscribe
     * @see #closeSubscription
     * @see Message#FIELD_NAME_DESTINATION
     */
    public String subscribe(String matcher, SubscriptionListener listener);
    
    /**
     * Create a durable subscriber to messages.
     * <p>
     * Register a durable subscription for one-to-many messages.
     * <p>
     * This call returns immediately; subscribing continues
     * asynchronously.  When the subscription is
     * ready to receive messages, the eFTL library calls your 
     * {@link SubscriptionListener#onSubscribe} callback.
     * <p>
     * A matcher can narrow subscription interest in the inbound
     * message stream.
     * 
     * @param matcher The subscription uses this matcher to
     *                    narrow the message stream.
     * @param durable The subscription uses this durable name. 
     * @param listener This listener defines callback methods for
     *                 successful subscription, message arrival, and
     *                 errors.
     *
     * @return An identifier that represents the new subscription.
     *
     * @throws IllegalStateException The connection is not open.
     *
     * @see SubscriptionListener
     * @see #unsubscribe
     * @see #closeSubscription
     * @see Message#FIELD_NAME_DESTINATION
     */
    public String subscribe(String matcher, String durable, SubscriptionListener listener);
    
    /**
     * Create a durable subscriber to messages.
     * <p>
     * Register a durable subscription for one-to-many messages.
     * <p>
     * This call returns immediately; subscribing continues
     * asynchronously.  When the subscription is
     * ready to receive messages, the eFTL library calls your 
     * {@link SubscriptionListener#onSubscribe} callback.
     * <p>
     * A matcher can narrow subscription interest in the inbound
     * message stream.
     * 
     * @param matcher The subscription uses this matcher to
     *                    narrow the message stream.
     * @param durable The subscription uses this durable name.
     * @param props These properties can be used to affect the subscription:
     *            <ul>
     *             <li> {@link EFTL#PROPERTY_ACKNOWLEDGE_MODE}
     *             <li> {@link EFTL#PROPERTY_DURABLE_TYPE}
     *             <li> {@link EFTL#PROPERTY_DURABLE_KEY}
     *            </ul>
     * @param listener This listener defines callback methods for
     *                 successful subscription, message arrival, and
     *                 errors.
     *
     * @return An identifier that represents the new subscription.
     *
     * @throws IllegalStateException The connection is not open.
     *
     * @see SubscriptionListener
     * @see #unsubscribe
     * @see #closeSubscription
     */
    public String subscribe(String matcher, String durable, Properties props, SubscriptionListener listener);
    
    /**
     * Close a subscription.
     * <p>
     * For durable subscriptions, this call will cause the persistence
     * service to stop delivering messages while leaving the durable 
     * subscription to continue accumulating messages. Any unacknowledged
     * messages will be made available for redelivery.
     * <p>
     * Programs receive subscription identifiers through their
     * {@link SubscriptionListener#onSubscribe} methods.
     * 
     * @param subscriptionId Close this subscription.
     *
     * @see #subscribe
     */
    public void closeSubscription(String subscriptionId);
    
    /**
     * Close all subscriptions.
     * <p>
     * For durable subscriptions, this call will cause the persistence
     * service to stop delivering messages while leaving the durable 
     * subscriptions to continue accumulating messages. Any unacknowledged
     * messages will be made available for redelivery.
     *
     * @see #subscribe
     */
    public void closeAllSubscriptions();

    /**
     * Stop message delivery to a subscription.
     *
     * @param subscriptionId Stop this subscription.
     */
    public void stopSubscription(String subscriptionId);

    /**
     * Resume message delivery to a subscription.
     *
     * @param subscriptionId Start this subscription.
     */
    public void startSubscription(String subscriptionId);

    /**
     * Unsubscribe from messages on a subscription.
     * <p>
     * For durable subscriptions, this call will cause the persistence
     * service to remove the durable subscription, along with any
     * persisted messages.
     * <p>
     * Programs receive subscription identifiers through their
     * {@link SubscriptionListener#onSubscribe} methods.
     * 
     * @param subscriptionId Unsubscribe this subscription.
     *
     * @see #subscribe
     */
    public void unsubscribe(String subscriptionId);
    
    /**
     * Unsubscribe from messages on all subscriptions.
     * <p>
     * For durable subscriptions, this call will cause the persistence
     * service to remove the durable subscriptions, along with any
     * persisted messages.
     *
     * @see #subscribe
     */
    public void unsubscribeAll();

    /**
     * Acknowledge this message.
     * <p>
     * Messages consumed from subscriptions with a client acknowledgment mode
     * must be explicitly acknowledged. The eFTL server will stop delivering
     * messages to the client once the server's configured maximum number of
     * unacknowledged messages is reached.
     *
     * @param message The message being acknowledged.
     *
     * @see EFTL#ACKNOWLEDGE_MODE_CLIENT
     * @see #acknowledgeAll
     */
    public void acknowledge(Message message);

    /**
     * Acknowledge all messages up to and including this message.
     * <p>
     * Messages consumed from subscriptions with a client acknowledgment mode
     * must be explicitly acknowledged. The eFTL server will stop delivering
     * messages to the client once the server's configured maximum number of
     * unacknowledged messages is reached.
     *
     * @param message The message being acknowledged.
     *
     * @see EFTL#ACKNOWLEDGE_MODE_CLIENT
     * @see #acknowledge
     */
    public void acknowledgeAll(Message message);
}
