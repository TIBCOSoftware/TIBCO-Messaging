/*
 * Copyright (c) 2001-$Date: 2020-05-26 09:25:10 -0700 (Tue, 26 May 2020) $ TIBCO Software Inc.
 * Licensed under a BSD-style license. Refer to [LICENSE]
 * For more information, please contact:
 * TIBCO Software Inc., Palo Alto, California, USA
 *
 * $Id: Request.java 125227 2020-05-26 16:25:10Z $
 *
 */
package com.tibco.eftl.impl;

import com.tibco.eftl.Message;

class Request {
    private String json;
    private long seqNum;

    Request(String json) {
        this.json = json;
    }

    Request(long seqNum, String json) {
        this.seqNum = seqNum;
        this.json = json;
    }

    long getSeqNum() {
        return seqNum;
    }

    String getJson() {
        return json;
    }

    boolean hasListener() {
        return false;
    }

    void onSuccess(Message response) {}

    void onError(String reason) {}

    void onError(int code, String reason) {}
}
