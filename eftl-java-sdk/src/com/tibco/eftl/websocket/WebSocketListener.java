/*
 * Copyright (c) 2013-$Date: 2013-12-04 09:51:34 -0800 (Wed, 04 Dec 2013) $ TIBCO Software Inc.
 * Licensed under a BSD-style license. Refer to [LICENSE]
 * For more information, please contact:
 * TIBCO Software Inc., Palo Alto, California, USA
 *
 * $Id: WebSocketListener.java 70851 2013-12-04 17:51:34Z $
 *
 */
package com.tibco.eftl.websocket;

public interface WebSocketListener {

    public void onOpen();
    
    public void onClose(int code, String reason);
    
    public void onError(Throwable cause);

    public void onMessage(String text);
    
    public void onMessage(byte[] data, int offset, int length);
    
    public void onPong(byte[] data, int offset, int length);
}
