/*
 * Copyright (c) 2001-$Date: 2020-01-13 09:13:35 -0800 (Mon, 13 Jan 2020) $ TIBCO Software Inc.
 * Licensed under a BSD-style license. Refer to [LICENSE]
 * For more information, please contact:
 * TIBCO Software Inc., Palo Alto, California, USA
 *
 * $Id: ConnectionListener.java 120856 2020-01-13 17:13:35Z bpeterse $
 *
 */
package com.tibco.eftl;

/**
 * Connection event handler.
 * <p>
 * Implement this interface to process connection events.
 * <p>
 * Supply an instance when you call
 * {@link EFTL#connect(String, java.util.Properties, ConnectionListener)}.
 */
public interface ConnectionListener
{
    /** Normal close.
     * <p>
     * Your program closed the connection by calling 
     * {@link Connection#disconnect}.
     * Programs can continue clean-up procedures.
     */
    public static final int NORMAL = 1000; /* RFC6455.CLOSE_NORMAL */
 
    /** The server closed the connection because the server is exiting.
     */
    public static final int SHUTDOWN = 1001; /* RFC6455.CLOSE_SHUTDOWN */
    
    /** Protocol error.  Please report this error to TIBCO support staff.
     */
    public static final int PROTOCOL = 1002; /* RFC6455.CLOSE_PROTOCOL */
    
    /** Invalid message from server.  Please report this error to TIBCO support staff.
     */
    public static final int BAD_DATA = 1003; /* RFC6455.CLOSE_BAD_DATA */
    
    /** Connection error.
     * <p>
     * Programs may attempt to reconnect.
     */
    public static final int CONNECTION_ERROR = 1006; /* RFC6455.CLOSE_NO_CLOSE */
    
    /** Invalid network data.  Please report this error to TIBCO support staff.
     */
    public static final int BAD_PAYLOAD = 1007; /* RFC6455.CLOSE_BAD_PAYLOAD */
    
    /** Policy violation. Please report this error to TIBCO support staff.
     */
    public static final int POLICY_VIOLATION = 1008; /* RFC6455.CLOSE_POLICY_VIOLATION */
    
    /** Message too large.
     * <p>
     * The eFTL server closed the connection because the program sent
     * a message that exceeds the channel's maximum message size.
     * (Administrators can configure this limit.)
     * <p>
     * Programs may attempt to reconnect.
     */
    public static final int MESSAGE_TOO_LARGE = 1009; /* RFC6455.CLOSE_MESSAGE_TOO_LARGE */
    
    /** eFTL server error.  Please report this error to TIBCO support staff.
     */
    public static final int SERVER_ERROR = 1011; /* RFC6455.CLOSE_SERVER_ERROR */
    
    /** The server closed the connection because the server is restarting.
     */
    public static final int RESTART = 1012; 
    
    /** The client could not establish a secure connection to the eFTL server.
     * <p>
     * Possible diagnoses:
     * <ul>
     *   <li> The client rejected the server certificate.
     *   <li> Encryption protocols do not match.
     * </ul>
     */
    public static final int FAILED_TLS_HANDSHAKE = 1015; /* RFC6455.CLOSE_FAILED_TLS_HANDSHAKE */
    
    /** Force close.
     * <p>
     * The eFTL server closed the connection because another client with
     * the same client identifier has connected.
     */ 
    public static final int FORCE_CLOSE = 4000;

    /** Not authenticated.
     * <p>
     * The eFTL server could not authenticate the client's username and 
     * passsword.
     */ 
    public static final int NOT_AUTHENTICATED = 4002;

    /** Bad subscription identifier.
     * <p>
     * The server detected an invalid subscription identifier.
     * Please report this error to TIBCO support staff.
     */
    public static final int BAD_SUBSCRIPTION_ID = 20;
    
    /** This user is not allowed to publish messages.
     * <p>
     * Administrators configure permission to publish.
     */
    public static final int PUBLISH_DISALLOWED = 12;
    
    /**
     * A new connection to the eFTL server is ready to use.
     * <p>
     * The eFTL library invokes this method only after your program calls 
     * {@link EFTL#connect} (and not after {@link Connection#reconnect}).
     * 
     * @param connection This connection is ready to use.
     */
    public void onConnect(Connection connection);
    
    /**
     * A connection to the eFTL server has re-opened and is ready to use.
     * <p>
     * The eFTL library invokes this method only after your program calls 
     * {@link Connection#reconnect} (and not after {@link EFTL#connect}).
     * 
     * @param connection This connection is ready to use.
     */
    public void onReconnect(Connection connection);
    
    /**
     * A connection to the eFTL server has closed.
     * <p>
     * Possible codes include:
     * <ul>
     * <li>{@link #NORMAL}
     * <li>{@link #SHUTDOWN}
     * <li>{@link #CONNECTION_ERROR}
     * <li>{@link #POLICY_VIOLATION}
     * <li>{@link #MESSAGE_TOO_LARGE}
     * <li>{@link #SERVER_ERROR}
     * <li>{@link #FORCE_CLOSE}
     * <li>{@link #NOT_AUTHENTICATED}
     * </ul>
     * 
     * @param connection This connection has closed.
     * @param code This code categorizes the condition.
     *             Application programs may use this value
     *             in response logic.
     * @param reason This string provides more detail.
     *               Application programs may use this value
     *               in error reporting and logging.
     */
    public void onDisconnect(Connection connection, int code, String reason);
    
    /**
     * An error prevented an operation.  The connection remains open.
     * <p>
     * Your implementation could alert the user.
     * <p>
     * Possible codes include:
     * <ul>
     * <li>{@link #BAD_SUBSCRIPTION_ID}
     * <li>{@link #PUBLISH_DISALLOWED}
     * </ul>
     * 
     * @param connection This connection reported an error.
     * @param code This code categorizes the error.
     *             Application programs may use this value
     *             in response logic.
     * @param reason This string provides more detail.
     *               Application programs may use this value
     *               in error reporting and logging.
     */
    public void onError(Connection connection, int code, String reason);
}
