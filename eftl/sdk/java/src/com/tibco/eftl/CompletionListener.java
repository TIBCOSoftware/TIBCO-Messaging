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

/**
 * Completion event handler.
 * <p>
 * Implement this interface to process completion events.
 * <p>
 * Supply an instance when you call
 * {@link Connection#publish(Message, CompletionListener)}.
 */
public interface CompletionListener
{
    /** The server could not forward a message published by an eFTL
     * client.
     * <p>
     * To determine the root cause, examine the server log.
     * <p> 
     * The client may attempt to publish the message again.
     */
    public static final int PUBLISH_FAILED = 11;
    
    /** This user is not allowed to publish messages.
     * <p>
     * Administrators configure permission to publish.
     */
    public static final int PUBLISH_DISALLOWED = 12;
    
    /**
     * A publish operation has completed successfully.
     * 
     * @param message This message has been published.
     */
    public void onCompletion(Message message);
    
    /**
     * A publish operation resulted in an error.
     * <p>
     * The message was not forwarded by the eFTL server.
     * <p>
     * When developing a client application, consider alerting the
     * user to the error.
     * <p>
     * Possible error codes include:
     * <ul>
     * <li>{@link #PUBLISH_FAILED}
     * <li>{@link #PUBLISH_DISALLOWED}
     * </ul>
     * 
     * @param message This message was <i> not </i> published.
     * @param code This code categorizes the error.
     *             Application programs may use this value
     *             in response logic.
     * @param reason This string provides more detail about the error.
     *               Application programs may use this value
     *               for error reporting and logging.
     */
    public void onError(Message message, int code, String reason);
}
