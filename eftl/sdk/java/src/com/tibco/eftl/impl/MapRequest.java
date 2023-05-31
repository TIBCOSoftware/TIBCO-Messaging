/*
 * Copyright (c) 2001-$Date$ Cloud Software Group, Inc.
 * Licensed under a BSD-style license. Refer to [LICENSE]
 *
 * $Id$
 *
 */
package com.tibco.eftl.impl;

import com.tibco.eftl.*;

class MapRequest extends Request {
    private String key;
    private Message value;
    private KVMapListener listener;

    MapRequest(final long seqNum, final String json, final String key, final KVMapListener listener) {
        this(seqNum, json, key, null, listener);
    }

    MapRequest(final long seqNum, final String json, final String key, final Message value, final KVMapListener listener) {
        super(seqNum, json);
        this.key = key;
        this.value = value;
        this.listener = listener;
    }

    @Override
    boolean hasListener() {
        return (listener != null);
    }

    @Override
    void onSuccess(Message response) {
        if (listener != null) {
            listener.onSuccess(key, (value != null ? value : response));
        }
    }

    @Override
    void onError(String reason) {
        if (listener != null) { 
            listener.onError(key, value, KVMapListener.MAP_REQUEST_FAILED, reason);
        }
    }

    @Override
    void onError(int code, String reason) {
        if (listener != null) {
            listener.onError(key, value, code, reason);
        }
    }
}
