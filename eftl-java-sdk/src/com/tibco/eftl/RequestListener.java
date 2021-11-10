/*
 * Copyright (c) 2001-$Date: 2020-06-22 14:31:05 -0700 (Mon, 22 Jun 2020) $ TIBCO Software Inc.
 * All rights reserved.
 * For more information, please contact:
 * TIBCO Software Inc., Palo Alto, California, USA
 *
 * $Id: RequestListener.java 126482 2020-06-22 21:31:05Z bpeterse $
 *
 */
package com.tibco.eftl;

/**
 * Request event handler.
 * <p>
 * Implement this interface to process request events.
 * <p>
 * Supply an instance when you call
 * {@link Connection#sendRequest(Message, double, RequestListener)}.
 */
public interface RequestListener
{
    /** The server could not forward a message published by an eFTL
     * client.
     * <p> 
     * The client may attempt to publish the message again.
     */
    public static final int REQUEST_FAILED = 41;
    
    /** This user is not allowed to publish messages.
     */
    public static final int REQUEST_DISALLOWED = 40;
 
    /** The request timed out.
     */   
    public static final int REQUEST_TIMEOUT = 99;

    /**
     * A request has received a reply.
     * 
     * @param reply The reply message.
     */
    public void onReply(Message reply);
    
    /**
     * A request resulted in an error.
     * <p>
     * The message was not forwarded by the eFTL server, or no reply
     * was received within the specified timeout.
     * <p>
     * Possible error codes include:
     * <ul>
     * <li>{@link #REQUEST_FAILED}
     * <li>{@link #REQUEST_DISALLOWED}
     * <li>{@link #REQUEST_TIMEOUT}
     * </ul>
     * 
     * @param request The original request message.
     * @param code This code categorizes the error.
     *             Application programs may use this value
     *             in response logic.
     * @param reason This string provides more detail about the error.
     *               Application programs may use this value
     *               for error reporting and logging.
     */
    public void onError(Message request, int code, String reason);
}
