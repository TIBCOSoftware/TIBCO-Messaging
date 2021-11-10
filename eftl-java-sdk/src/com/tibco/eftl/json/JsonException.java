/*
 * Copyright (c) 2001-$Date: 2016-03-11 16:29:10 -0800 (Fri, 11 Mar 2016) $ TIBCO Software Inc.
 * All rights reserved.
 * For more information, please contact:
 * TIBCO Software Inc., Palo Alto, California, USA
 *
 * $Id: JsonException.java 84680 2016-03-12 00:29:10Z bpeterse $
 */
package com.tibco.eftl.json;

public class JsonException extends RuntimeException {

    private static final long serialVersionUID = 1L;

    public JsonException(String message) {
        super(message);
    }

    public JsonException(Throwable cause) {
        super(cause);
    }
}
