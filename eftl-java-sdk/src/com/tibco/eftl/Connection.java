/*
 * Copyright (c) 2001-$Date: 2018-04-23 16:00:56 -0500 (Mon, 23 Apr 2018) $ TIBCO Software Inc.
 * Licensed under a BSD-style license. Refer to [LICENSE]
 * For more information, please contact:
 * TIBCO Software Inc., Palo Alto, California, USA
 *
 * $Id: Connection.java 100979 2018-04-23 21:00:56Z bpeterse $
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
     * Publish a one-to-many message to all subscribing clients.
     * <p>
     * It is good practice to publish each message to a specific
     * destination by using the message field name
     * {@link Message#FIELD_NAME_DESTINATION}.
     * <p>
     * To direct a message to a specific destination,
     * add a string field to the message; for example:
     * <pre>
     * {@code
     * message.setString(Message.FIELD_NAME_DESTINATION, "myTopic");
     * }
     * </pre>
     *
     * @param message Publish this message.
     *
     * @throws IllegalStateException The connection is not open.
     * @throws IllegalArgumentException The message would exceed the
     *         eFTL server's maximum message size.
     */
    public void publish(Message message);
    
    /**
     * Publish a one-to-many message to all subscribing clients.
     * <p>
     * This call returns immediately; publishing continues
     * asynchronously.  When the publish completes successfully, 
     * the eFTL library calls your {@link CompletionListener#onCompletion} 
     * callback.
     * <p>
     * It is good practice to publish each message to a specific
     * destination by using the message field name
     * {@link Message#FIELD_NAME_DESTINATION}.
     * <p>
     * To direct a message to a specific destination,
     * add a string field to the message; for example:
     * <pre>
     * {@code
     * message.setString(Message.FIELD_NAME_DESTINATION, "myTopic");
     * }
     * </pre>
     * 
     * @param message Publish this message.
     * @param listener This listener defines callback methods for
     *                 successful completion and for errors.
     *
     * @throws IllegalStateException The connection is not open.
     * @throws IllegalArgumentException The message would exceed the
     *         eFTL server's maximum message size.
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
     * <p>
     * It is good practice to subscribe to
     * messages published to a specific destination
     * by using the message field name
     * {@link Message#FIELD_NAME_DESTINATION}.
     * <p>
     * To subscribe to messages published to a specific destination,
     * create a subscription matcher for that destination; for example:
     * <code> {"_dest":"myTopic"} </code>
     * </p>
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
     * <p>
     * It is good practice to subscribe to
     * messages published to a specific destination
     * by using the message field name
     * {@link Message#FIELD_NAME_DESTINATION}.
     * <p>
     * To subscribe to messages published to a specific destination,
     * create a subscription matcher for that destination; for example:
     * <code> {"_dest":"myTopic"} </code>
     * </p>
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
     */
    public String subscribe(String matcher, String durable, Properties props, SubscriptionListener listener);
    
    /**
     * Close a subscription.
     * <p>
     * Programs receive subscription identifiers through their
     * {@link SubscriptionListener#onSubscribe} methods.
     * 
     * @param subscriptionId Close this subscription.
     *
     * @see #subscribe
     */
    public void unsubscribe(String subscriptionId);
    
    /**
     * Close all subscriptions.
     *
     * @see #subscribe
     */
    public void unsubscribeAll();
}
