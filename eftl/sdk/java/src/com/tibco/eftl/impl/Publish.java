/*
 * Copyright (c) 2001-$Date: 2020-05-26 09:25:10 -0700 (Tue, 26 May 2020) $ TIBCO Software Inc.
 * Licensed under a BSD-style license. Refer to [LICENSE]
 * For more information, please contact:
 * TIBCO Software Inc., Palo Alto, California, USA
 *
 * $Id: Publish.java 125227 2020-05-26 16:25:10Z $
 *
 */
package com.tibco.eftl.impl;

import com.tibco.eftl.*;

class Publish extends Request {
    private Message message;
    private CompletionListener listener;

    Publish(final long seqNum, final String json, final Message message, final CompletionListener listener) {
        super(seqNum, json);
        this.message = message;
        this.listener = listener;
    }

    @Override
    boolean hasListener() {
        return (listener != null);
    }

    @Override
    void onSuccess(Message response) {
        if (listener != null) {
            listener.onCompletion(message);
        }
    }

    @Override
    void onError(String reason) {
        if (listener != null) {
            listener.onError(message, CompletionListener.PUBLISH_FAILED, reason);
        }
    }

    @Override
    void onError(int code, String reason) {
        if (listener != null) {
            listener.onError(message, code, reason);
        }
    }
}
