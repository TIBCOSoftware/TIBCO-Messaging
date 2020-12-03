/*
 * Copyright (c) 2001-$Date: 2020-05-26 09:25:10 -0700 (Tue, 26 May 2020) $ TIBCO Software Inc.
 * Licensed under a BSD-style license. Refer to [LICENSE]
 * For more information, please contact:
 * TIBCO Software Inc., Palo Alto, California, USA
 *
 * $Id: SendRequest.java 125227 2020-05-26 16:25:10Z bpeterse $
 *
 */
package com.tibco.eftl.impl;

import java.util.Timer;
import java.util.TimerTask;

import com.tibco.eftl.*;

class SendRequest extends Request {
    private Message message;
    private RequestListener listener;
    private Timer timer;

    SendRequest(final long seqNum, final String json, final Message message, final RequestListener listener) {
        super(seqNum, json);
        this.message = message;
        this.listener = listener;
        this.timer = new Timer("request timer " + seqNum);
    }

    void setTimeout(long timeout, TimerTask task) {
        timer.schedule(task, timeout);
    }

    @Override
    boolean hasListener() {
        return (listener != null);
    }

    @Override
    void onSuccess(Message response) {
        if (listener != null) {
            listener.onReply(response);
        }

        timer.cancel();
    }

    @Override
    void onError(String reason) {
        if (listener != null) {
            listener.onError(message, RequestListener.REQUEST_FAILED, reason);
        }

        timer.cancel();
    }

    @Override
    void onError(int code, String reason) {
        if (listener != null) {
            listener.onError(message, code, reason);
        }

        timer.cancel();
    }
}
