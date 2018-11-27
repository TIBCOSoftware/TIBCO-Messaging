/*
 * Copyright (c) 2001-$Date: 2018-05-14 16:33:28 -0500 (Mon, 14 May 2018) $ TIBCO Software Inc.
 * Licensed under a BSD-style license. Refer to [LICENSE]
 * For more information, please contact:
 * TIBCO Software Inc., Palo Alto, California, USA
 *
 * $Id: Publish.java 101286 2018-05-14 21:33:28Z bpeterse $
 *
 */
package com.tibco.eftl.impl;

import com.tibco.eftl.*;

public class Publish extends Request {
    protected Publish(final long seqNum, final String json, final Message message, final CompletionListener listener) {
        super(seqNum, json, (listener == null ? null : new RequestListener() {
            @Override
            public void onSuccess(Message response) {
                listener.onCompletion(message);
            }

            @Override
            public void onError(String reason) {
                listener.onError(message, CompletionListener.PUBLISH_FAILED, reason);
            }

            @Override
            public void onError(int code, String reason) {
                listener.onError(message, code, reason);
            }
        }));
    }
}
