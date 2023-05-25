/*
 * Copyright (c) 2013-$Date$ TIBCO Software Inc.
 * Licensed under a BSD-style license. Refer to [LICENSE]
 * For more information, please contact:
 * TIBCO Software Inc., Palo Alto, California, USA
 *
 * $Id$
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
