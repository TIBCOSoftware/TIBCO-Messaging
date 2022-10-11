/*
 * Copyright (c) 2001-$Date: 2020-05-26 09:25:10 -0700 (Tue, 26 May 2020) $ TIBCO Software Inc.
 * Licensed under a BSD-style license. Refer to [LICENSE]
 * For more information, please contact:
 * TIBCO Software Inc., Palo Alto, California, USA
 *
 * $Id: PublishContext.cs 125227 2020-05-26 16:25:10Z $
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
