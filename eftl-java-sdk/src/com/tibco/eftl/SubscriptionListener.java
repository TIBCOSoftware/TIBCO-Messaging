/*
 * Copyright (c) 2001-$Date: 2016-04-12 11:27:33 -0700 (Tue, 12 Apr 2016) $ TIBCO Software Inc.
 * Licensed under a BSD-style license. Refer to [LICENSE]
 * For more information, please contact:
 * TIBCO Software Inc., Palo Alto, California, USA
 *
 * $Id: SubscriptionListener.java 85385 2016-04-12 18:27:33Z dinitz $
 *
 */
package com.tibco.eftl;

/**
 * Subscription event handler.
 * <p>
 * Implement this interface to process inbound messages and
 * other subscription events.
 * <p>
 * Supply an instance when you call
 * {@link Connection#subscribe(String, String, SubscriptionListener)} or 
 * {@link Connection#subscribe(String, SubscriptionListener)}.
 */
public interface SubscriptionListener
{
    /**
     * The administrator has disallowed the user from subscribing.
     */
    public static final int SUBSCRIPTIONS_DISALLOWED = 13;
    
    /** 
     * The eFTL server could not establish the subscription.
     * <p>
     * You may attempt to subscribe again.  It is good practice for
     * administrators to check the server log to determine the root
     * cause.
     */
    public static final int SUBSCRIPTION_FAILED = 21;
 
    /**
     * The client supplied an invalid matcher or durable name in
     * the subscribe call.
     */
    public static final int SUBSCRIPTION_INVALID = 22;

    /**
     * Process inbound messages.
     * <p>
     * The eFTL library presents inbound messages to this method for
     * processing.  You must implement this method to process the
     * messages.
     * <p>
     * The messages are not thread-safe.  You may access a
     * message in any thread, but in only one thread at a time.
     * 
     * @param messages Inbound messages.
     */
    public void onMessages(Message[] messages);
    
    /**
     * A new subscription is ready to receive messages.
     * <p>
     * The eFTL library may invoke this method after the first
     * message arrives.
     * <p>
     * To close the subscription, call {@link Connection#unsubscribe}
     * with this subscription identifier.
     * 
     * @param subscriptionId This subscription is ready.
     */
    public void onSubscribe(String subscriptionId);
    
    /**
     * Process subscription errors.
     * <p>
     * Possible returned codes include:
     * <ul>
     * <li>{@link #SUBSCRIPTIONS_DISALLOWED}
     * <li>{@link #SUBSCRIPTION_FAILED}
     * <li>{@link #SUBSCRIPTION_INVALID}
     * </ul>
     * 
     * @param subscriptionId eFTL could not establish this subscription.
     * @param code This code categorizes the error.
     *             Application programs may use this value
     *             in response logic.
     * @param reason This string provides more detail.
     *               Application programs may use this value
     *               in error reporting and logging.
     */
    public void onError(String subscriptionId, int code, String reason);
}
