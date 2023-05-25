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
 * Key-value map event handler.
 * <p>
 * Implement this interface to process key-value map events.
 * <p>
 * Supply an instance when you call
 * {@link KVMap#set(String, Message, KVMapListener)},
 * {@link KVMap#get(String, KVMapListener)}, or
 * {@link KVMap#remove(String, KVMapListener)}.
 */
public interface KVMapListener
{
    /** 
     * The map operation failed.
     */
    public static final int MAP_REQUEST_FAILED = 30;
    
    /** 
     * This user is not authorized for map operations.
     */
    public static final int MAP_REQUEST_DISALLOWED = 14;
    
    /**
     * A key-value map operation has completed successfully.
     * 
     * @param key The key for the operation.
     * @param value The value of the key.
     */
    public void onSuccess(String key, Message value);

    /**
     * A key-value map operation resulted in an error.
     * <p>
     * When developing a client application, consider alerting the
     * user to the error.
     * <p>
     * Possible error codes include:
     * <ul>
     * <li>{@link #MAP_REQUEST_DISALLOWED}
     * <li>{@link #MAP_REQUEST_FAILED}
     * </ul>
     * 
     * @param key The key for the operation.
     * @param value This value message was <i> not </i> set.
     * @param code This code categorizes the error.
     *             Application programs may use this value
     *             in response logic.
     * @param reason This string provides more detail about the error.
     *               Application programs may use this value
     *               for error reporting and logging.
     */
    public void onError(String key, Message value, int code, String reason);
}
