/*
 * Copyright (c) 2001-$Date$ Cloud Software Group, Inc.
 * Licensed under a BSD-style license. Refer to [LICENSE]
 *
 * $Id$
 *
 */
package com.tibco.eftl;

import java.security.KeyStore;
import java.util.Properties;

import com.tibco.eftl.impl.WebSocketConnection;

/**
 * Programs use class EFTL to connect to an eFTL server.
 */
public class EFTL
{
    /**
     * Connect as this user; property name.
     * <p>
     * Programs use this property to supply a user name credential
     * to the {@link #connect} call if the username is not specified
     * with the URL.
     * The server authenticates the
     * user name and password.
     * 
     * @see #connect
     */
    public static final String PROPERTY_USERNAME = "user";
    
    /**
     * Connect using this password; property name.
     * <p>
     * Programs use this property to supply a password credential
     * to the {@link #connect} call if the password is not specified
     * with the URL.
     * The server authenticates the
     * user name and password.
     * 
     * @see #connect
     */
    public static final String PROPERTY_PASSWORD = "password";
    
    /**
     * Connection attempt timeout; property name.
     * <p>
     * Programs use this property to supply a timeout
     * to the {@link #connect} call.
     * If eFTL cannot connect to the server within this time limit (in
     * seconds), it stops trying to connect and invokes
     * your {@link ConnectionListener#onDisconnect} method with a
     * {@link ConnectionListener#CONNECTION_ERROR} code.
     * <p>
     * If you omit this property, the default timeout is 15.0 seconds.
     * 
     * @see #connect
     */
    public static final String PROPERTY_TIMEOUT = "timeout";

    /**
     * Connect using this client identifier; property name.
     * <p>
     * Programs use this property to supply a client identifier
     * to the {@link #connect} call if the client identifier is not
     * specified with the URL.
     * The server uses the client
     * identifier to associate a particular client with a durable
     * subscription.
     * <p>
     * If you omit this property, the server assigns a unique
     * client identifier.
     * 
     * @see #connect
     */
    public static final String PROPERTY_CLIENT_ID = "client_id";
    
    /**
     * Firebase Cloud Messaging registration token for push 
     * notifications; property name.
     * <p>
     * Programs use this property to supply a FCM registration token
     * to the {@link #connect} call. 
     * The server uses the token to push notifications 
     * to a disconnected client when messages are available.
     * 
     * @see #connect
     */
    public static final String PROPERTY_NOTIFICATION_TOKEN = "notification_token";

    /**
     * Maximum Auto-reconnect attempts; property name.
     * <p>
     * Programs use this property to define the maximum number of times
     * an attempt to auto-reconnect to the server is made.
     * <p>
     * If you omit this property, the default value of 256 is used.
     * 
     * @see #connect
     */
    public static final String PROPERTY_AUTO_RECONNECT_ATTEMPTS = "auto_reconnect_attempts";

    /**
     * Maximum auto-reconnect delay; property name.
     * <p>
     * Programs use this property to define the maximum delay between auto-reconnect attempts.
     * Following the loss of connection, the auto-reconnect process delays for 1 second
     * before attempting to auto-reconnect. Subsequent attempts double this delay time, up to the maximum
     * defined by this property.
     * <p>
     * If you omit this property, the default value of 30 seconds is used.
     * 
     * @see #connect
     */
    public static final String PROPERTY_AUTO_RECONNECT_MAX_DELAY = "auto_reconnect_max_delay";
 
    /**
     * Maximum number of unacknowledged messages allowed for the client; property name.
     * <p>
     * Programs use this property to specify the maximum number of unacknowledged messages
     * allowed for the client. Once the maximum number of unacknowledged messages is reached
     * the client will stop receiving additional messages until previously received messages
     * are acknowledged.
     * <p>
     * If you omit this property, the server's configured value will be used.
     *
     * @see #connect
     */
    public static final String PROPERTY_MAX_PENDING_ACKS = "max_pending_acks";

    /**
     * Create a subscription with a specific acknowledgment mode.
     * <p>
     * Programs use this property to specify how messages consumed by the
     * subscription are acknowledged. The following acknowledgment modes are
     * supported:
     *            <ul>
     *             <li> {@link #ACKNOWLEDGE_MODE_AUTO}
     *             <li> {@link #ACKNOWLEDGE_MODE_CLIENT}
     *             <li> {@link #ACKNOWLEDGE_MODE_NONE}
     *            </ul>
     * <p>
     * If you omit this property, the subscription is created with an
     * acknowledgment mode of {@link #ACKNOWLEDGE_MODE_AUTO}.
     *
     * @see Connection#subscribe(String, String, Properties, SubscriptionListener)
     * @see #ACKNOWLEDGE_MODE_AUTO
     * @see #ACKNOWLEDGE_MODE_CLIENT
     * @see #ACKNOWLEDGE_MODE_NONE
     */
    public static final String PROPERTY_ACKNOWLEDGE_MODE = "ack";

    /**
     * Auto acknowledgment mode.
     * <p>
     * Messages consumed from a subscription with this acknowledgment mode are
     * automatically acknowledged.
     * 
     * @see #PROPERTY_ACKNOWLEDGE_MODE
     */
    public static final String ACKNOWLEDGE_MODE_AUTO = "auto";

    /**
     * Client acknowledgment mode.
     * <p>
     * Messages consumed from a subscription with this acknowledgment mode require
     * explicit acknowledgment by calling either the {@link Connection#acknowledge} 
     * or {@link Connection#acknowledgeAll} method.
     * <p>
     * The eFTL server will stop delivering messages to the client once the 
     * server's configured maximum unacknowledged messages is reached.
     *
     * @see #PROPERTY_ACKNOWLEDGE_MODE
     */
    public static final String ACKNOWLEDGE_MODE_CLIENT = "client";

    /**
     * None acknowledgment mode.
     * <p>
     * Messages consumed from a subscription with this acknowledgment mode do
     * not require acknowledgment.
     * 
     * @see #PROPERTY_ACKNOWLEDGE_MODE
     */
    public static final String ACKNOWLEDGE_MODE_NONE = "none";

    /**
     * Create a durable subscription of this type; property name.
     * <p>
     * Programs use this optional property to supply a durable type to the
     * {@link Connection#subscribe(String, String, Properties, SubscriptionListener)}
     * call. If not specified the default durable type will be used.
     * <p>
     * The available durable types are {@link #DURABLE_TYPE_SHARED} and
     * {@link #DURABLE_TYPE_LAST_VALUE}.
     * 
     * @see Connection#subscribe(String, String, Properties, SubscriptionListener)
     * @see #DURABLE_TYPE_SHARED
     * @see #DURABLE_TYPE_LAST_VALUE
     */
    public static final String PROPERTY_DURABLE_TYPE = "type";
    
    /**
     * Specify the key field of a {@link #DURABLE_TYPE_LAST_VALUE last-value}
     * durable subscription; property name.
     * <p>
     * Programs use this property to supply a key field to the
     * {@link Connection#subscribe(String, String, Properties, SubscriptionListener)}
     * call when the {@link #PROPERTY_DURABLE_TYPE} is of type
     * {@link #DURABLE_TYPE_LAST_VALUE}.
     * <p>
     * Note that the supplied key field must be a part of the durable
     * subscription's matcher.
     * 
     * @see Connection#subscribe(String, String, Properties, SubscriptionListener)
     */
    public static final String PROPERTY_DURABLE_KEY = "key";
    
    /**
     * Shared durable type.
     * <p>
     * Multiple cooperating subscribers can use the same shared durable
     * to each receive a portion of the subscription's messages.
     * 
     * @see Connection#subscribe(String, String, Properties, SubscriptionListener)
     * @see #PROPERTY_DURABLE_TYPE
     */
    public static final String DURABLE_TYPE_SHARED = "shared";
    
    /**
     * Last-value durable type.
     * <p>
     * A last-value durable subscription stores only the most recent message
     * for each unique value of the {@link #PROPERTY_DURABLE_KEY}.
     * 
     * @see Connection#subscribe(String, String, Properties, SubscriptionListener)
     * @see #PROPERTY_DURABLE_TYPE
     * @see #PROPERTY_DURABLE_KEY
     */
    public static final String DURABLE_TYPE_LAST_VALUE = "last-value";
    
    private static KeyStore trustStore; 
    private static boolean trustAll; 

    protected EFTL()
    {
    }
    
    /**
     * Get the version of the eFTL Java client library.
     * 
     * @return The version of the eFTL Java client library.
     */
    public static String getVersion()
    {
        return Version.EFTL_VERSION_STRING_LONG;
    }
    
    /**
     * Set the SSL trust store that the client library uses to verify
     * certificates from the eFTL server.
     * <p>
     * You cannot set the trust store after the first connect call.
     * 
     * @param trustStore Set this trust store.
     */
    public static void setSSLTrustStore(KeyStore trustStore)
    {
        EFTL.trustStore = trustStore;
    }
    
    /**
     * Set whether or not to skip server certificate authentication.
     * <p>
     * Specify {@code true} to accept any server certificate. This 
     * option should only be used during development and testing.
     * 
     * @param trustAll Set whether or not to skip server certificate 
     * authenticaiton.
     */
    public static void setSSLTrustAll(boolean trustAll)
    {
        EFTL.trustAll = trustAll;
    }
    
    /**
     * Connect to an eFTL server.
     * <p>
     * This call returns immediately; connecting continues asynchronously.
     * When the connection is ready to use, the eFTL library calls your
     * {@link ConnectionListener#onConnect} method, passing a
     * {@link Connection} object that you can use to publish and subscribe.
     * <p>
     * When a pipe-separated list of URLs is specified this call will attempt
     * a connection to each in turn, in a random order, until one is connected.
     * <p>
     * A program that uses more than one server channel must connect
     * separately to each channel.
     *
     * @param url The call connects to the eFTL server at this URL. This can be 
     *            a single URL, or a pipe ('|') separated list of URLs. URLs can
     *            be in either of these forms:
     *            <ul>
     *             <li> {@code ws://host:port/channel}
     *             <li> {@code wss://host:port/channel}
     *            </ul>
     *            Optionally, the URLs can contain the username, password, 
     *            and/or client identifier: 
     *            <ul>
     *             <li> {@code ws://username:password@host:port/channel?clientId=<identifier>}
     *             <li> {@code wss://username:password@host:port/channel?clientId=<identifier>}
     *            </ul>
     * @param props These properties affect the connection attempt:
     *            <ul>
     *             <li> {@link #PROPERTY_CLIENT_ID}
     *             <li> {@link #PROPERTY_USERNAME}
     *             <li> {@link #PROPERTY_PASSWORD}
     *             <li> {@link #PROPERTY_TIMEOUT}
     *             <li> {@link #PROPERTY_NOTIFICATION_TOKEN}
     *             <li> {@link #PROPERTY_AUTO_RECONNECT_ATTEMPTS}
     *             <li> {@link #PROPERTY_AUTO_RECONNECT_MAX_DELAY}
     *             <li> {@link #PROPERTY_MAX_PENDING_ACKS}
     *            </ul>
     * @param listener Connection events invoke methods of this listener.
     * @throws IllegalArgumentException The URL is invalid.
     * @see ConnectionListener
     */
    public static void connect(String url, Properties props, ConnectionListener listener)
    {
        WebSocketConnection connection = new WebSocketConnection(url, listener);
        connection.setTrustStore(trustStore);
        connection.setTrustAll(trustAll);
        connection.connect(props);
    }
}
