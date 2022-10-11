/*
 * Copyright (c) 2013-$Date: 2013-12-04 09:51:34 -0800 (Wed, 04 Dec 2013) $ TIBCO Software Inc.
 * Licensed under a BSD-style license. Refer to [LICENSE]
 * For more information, please contact:
 * TIBCO Software Inc., Palo Alto, California, USA
 *
 * $Id: UpgradeException.java 70851 2013-12-04 17:51:34Z $
 *
 */
package com.tibco.eftl.websocket;

@SuppressWarnings("serial")
public class UpgradeException extends Exception {

    private final int statusCode;

    public UpgradeException(int statusCode, String message) {
        super(message);
        this.statusCode = statusCode;
    }

    public int getStatusCode() {
        return statusCode;
    }
}
