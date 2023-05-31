/*
 * Copyright (c) 2013-$Date$ Cloud Software Group, Inc.
 * Licensed under a BSD-style license. Refer to [LICENSE]
 *
 * $Id$
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
