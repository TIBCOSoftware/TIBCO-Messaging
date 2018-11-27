/*
 * Copyright (c) 2001-$Date: 2018-05-14 16:33:28 -0500 (Mon, 14 May 2018) $ TIBCO Software Inc.
 * Licensed under a BSD-style license. Refer to [LICENSE]
 * For more information, please contact:
 * TIBCO Software Inc., Palo Alto, California, USA
 *
 * $Id: Request.java 101286 2018-05-14 21:33:28Z bpeterse $
 *
 */
package com.tibco.eftl.impl;

public class Request {
    private RequestListener listener;
    private String json;
    private long seqNum;

    protected Request(String json) {
        this.json = json;
    }

    protected Request(long seqNum, String json, RequestListener listener) {
        this.seqNum = seqNum;
        this.json = json;
        this.listener = listener;
    }

    protected long getSeqNum() {
        return seqNum;
    }

    protected String getJson() {
        return json;
    }

    protected RequestListener getListener() {
        return listener;
    }
}
