/*
 * Copyright (c) 2001-$Date: 2018-02-05 18:15:48 -0600 (Mon, 05 Feb 2018) $ TIBCO Software Inc.
 * Licensed under a BSD-style license. Refer to [LICENSE]
 * For more information, please contact:
 * TIBCO Software Inc., Palo Alto, California, USA
 *
 * $Id: EFTL.java 99237 2018-02-06 00:15:48Z bpeterse $
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
     * GCM registration token for push notifications; property name.
     * <p>
     * Programs use this property to supply a GCM registration token
     * to the {@link #connect} call. 
     * The server uses the registration token to push notifications 
     * to a disconnected client when messages are available.
     * 
     * @see #connect
     */
    public static final String PROPERTY_NOTIFICATION_TOKEN = "notification_token";

    /**
     * Maximum autoreconnect attempts; property name.
     * <p>
     * Programs use this property to define the maximum number of times
     * an attempt to autoreconnect to the server is made.
     * <p>
     * If you omit this property, the default value of 5 is used.
     * 
     * @see #connect
     */
    public static final String PROPERTY_AUTO_RECONNECT_ATTEMPTS = "auto_reconnect_attempts";

    /**
     * Maximum autoreconnect delay; property name.
     * <p>
     * Programs use this property to define the maximum delay between autoreconnect attempts.
     * Following the loss of connection, the autoreconnect process delays for 1 second
     * before attempting to autoreconnect. Subsequent attempts double this delay time, up to the maximum
     * defined by this property.
     * <p>
     * If you omit this property, the default value of 30 seconds is used.
     * 
     * @see #connect
     */
    public static final String PROPERTY_AUTO_RECONNECT_MAX_DELAY = "auto_reconnect_max_delay";
    
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
     * If you do not set the SSL trust store, then the application
     * trusts any server certificate.
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
     * Connect to an eFTL server.
     * <p>
     * This call returns immediately; connecting continues asynchronously.
     * When the connection is ready to use, the eFTL library calls your
     * {@link ConnectionListener#onConnect} method, passing a
     * {@link Connection} object that you can use to publish and subscribe.
     *
     * <p>
     * A program that uses more than one server channel must connect
     * separately to each channel.
     *
     * @param url The call connects to the eFTL server at this URL.
     *            The URL can be in either of these forms:
     *            <ul>
     *             <li> {@code ws://host:port/channel}
     *             <li> {@code wss://host:port/channel}
     *            </ul>
     *            Optionally, the URL can contain the username, password, 
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
     *            </ul>
     * @param listener Connection events invoke methods of this listener.
     * @throws IllegalArgumentException The URL is invalid.
     * @see ConnectionListener
     */
    public static void connect(String url, Properties props, ConnectionListener listener)
    {
        WebSocketConnection connection = new WebSocketConnection(url, listener);
        connection.setTrustStore(trustStore);
        connection.connect(props);
    }
}
