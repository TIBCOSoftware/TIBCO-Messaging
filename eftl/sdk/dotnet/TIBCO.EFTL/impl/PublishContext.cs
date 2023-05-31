/*
 * Copyright (c) 2001-$Date$ Cloud Software Group, Inc.
 * Licensed under a BSD-style license. Refer to [LICENSE]
 *
 * $Id$
 *
 */

using System;

namespace TIBCO.EFTL 
{
    internal class PublishContext : RequestContext 
    {
        private ICompletionListener listener;
        private IMessage message;

        internal PublishContext(Int64 seqNum, String json, IMessage message, ICompletionListener listener) : base (seqNum, json)
        { 
            this.message = message;
            this.listener = listener;
        }

        internal override bool HasListener()
        {
            return (listener != null);
        }

        internal override void OnSuccess(IMessage response) 
        {
            if (listener != null)
                listener.OnCompletion(message);
        }

        internal override void OnError(String reason) 
        {
            if (listener != null)
                listener.OnError(message, CompletionListenerConstants.PUBLISH_FAILED, reason);
        }

        internal override void OnError(int code, String reason) 
        {
            if (listener != null)
                listener.OnError(message, code, reason);
        }
    }
}
