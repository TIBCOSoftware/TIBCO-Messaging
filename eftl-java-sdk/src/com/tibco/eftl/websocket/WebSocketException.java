/*
 * Copyright (c) 2013-$Date: 2013-12-04 09:51:34 -0800 (Wed, 04 Dec 2013) $ TIBCO Software Inc.
 * Licensed under a BSD-style license. Refer to [LICENSE]
 * For more information, please contact:
 * TIBCO Software Inc., Palo Alto, California, USA
 *
 * $Id: WebSocketException.java 70851 2013-12-04 17:51:34Z $
 *
 */
package com.tibco.eftl.websocket;

@SuppressWarnings("serial")
public class WebSocketException extends Exception {

    public WebSocketException(String message) {
        super(message);
    }
}
