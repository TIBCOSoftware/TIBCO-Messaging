/*
 * Copyright (c) 2001-$Date: 2018-05-14 16:33:28 -0500 (Mon, 14 May 2018) $ TIBCO Software Inc.
 * Licensed under a BSD-style license. Refer to [LICENSE]
 * For more information, please contact:
 * TIBCO Software Inc., Palo Alto, California, USA
 *
 * $Id: MapRequest.java 101286 2018-05-14 21:33:28Z bpeterse $
 *
 */
package com.tibco.eftl.impl;

import com.tibco.eftl.*;

public class MapRequest extends Request {
    protected MapRequest(final long seqNum, final String json, final String key, final Message value,
            final KVMapListener listener) {
        super(seqNum, json, (listener == null ? null : new RequestListener() {
            @Override
            public void onSuccess(Message response) {
                listener.onSuccess(key, value);
            }

            @Override
            public void onError(String reason) {
                listener.onError(key, value, KVMapListener.MAP_REQUEST_FAILED, reason);
            }

            @Override
            public void onError(int code, String reason) {
                listener.onError(key, value, code, reason);
            }
        }));
    }

    protected MapRequest(final long seqNum, final String json, final String key, final KVMapListener listener) {
        super(seqNum, json, (listener == null ? null : new RequestListener() {
            @Override
            public void onSuccess(Message response) {
                listener.onSuccess(key, response);
            }

            @Override
            public void onError(String reason) {
                listener.onError(key, null, KVMapListener.MAP_REQUEST_FAILED, reason);
            }

            @Override
            public void onError(int code, String reason) {
                listener.onError(key, null, code, reason);
            }
        }));
    }
}
