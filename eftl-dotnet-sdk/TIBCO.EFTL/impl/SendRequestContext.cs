/*
 * Copyright (c) 2001-$Date: 2018-09-04 15:57:51 -0700 (Tue, 04 Sep 2018) $ TIBCO Software Inc.
 * Licensed under a BSD-style license. Refer to [LICENSE]
 * For more information, please contact:
 * TIBCO Software Inc., Palo Alto, California, USA
 *
 * $Id: PublishContext.cs 103512 2018-09-04 22:57:51Z $
 *
 */

using System;
using System.Threading;

namespace TIBCO.EFTL 
{
    internal class SendRequestContext : RequestContext 
    {
        private IRequestListener listener;
        private IMessage message;
        private Timer timer;

        internal SendRequestContext(Int64 seqNum, String json, IMessage message, IRequestListener listener) : base (seqNum, json)
        { 
            this.message = message;
            this.listener = listener;
        }

        internal void SetTimeout(double timeout, TimerCallback callback)
        {
            timer = new System.Threading.Timer(callback, seqNum, TimeSpan.FromSeconds(timeout), Timeout.InfiniteTimeSpan);
        }

        internal override bool HasListener()
        {
            return (listener != null);
        }

        internal override void OnSuccess(IMessage response) 
        {
            if (listener != null)
                listener.OnReply(response);

            timer?.Dispose();
        }

        internal override void OnError(String reason) 
        {
            if (listener != null)
                listener.OnError(message, RequestListenerConstants.REQUEST_FAILED, reason);

            timer?.Dispose();
        }

        internal override void OnError(int code, String reason) 
        {
            if (listener != null)
                listener.OnError(message, code, reason);

            timer?.Dispose();
        }
    }
}
