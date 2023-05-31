/*
 * Copyright (c) 2001-$Date$ Cloud Software Group, Inc.
 * Licensed under a BSD-style license. Refer to [LICENSE]
 *
 * $Id$
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
