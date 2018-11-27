/*
 * Copyright (c) 2001-$Date: 2018-09-04 17:57:51 -0500 (Tue, 04 Sep 2018) $ TIBCO Software Inc.
 * Licensed under a BSD-style license. Refer to [LICENSE]
 * For more information, please contact:
 * TIBCO Software Inc., Palo Alto, California, USA
 *
 * $Id: PublishContext.cs 103512 2018-09-04 22:57:51Z bpeterse $
 *
 */

using System;

namespace TIBCO.EFTL 
{
    internal class PublishContext : RequestContext 
    {
        internal PublishContext(Int64 seqNum, String json, IMessage message, ICompletionListener listener) : base (seqNum, json, (listener != null ? new PublishListener (message, listener) : null)) 
        { 
        }

        internal class PublishListener : IRequestListener 
        {
            private ICompletionListener listener;
            private IMessage message;

            internal PublishListener(IMessage message, ICompletionListener listener) 
            {
                this.message = message;
                this.listener = listener;
            }

            public void OnSuccess(IMessage response) 
            {
                listener.OnCompletion(message);
            }

            public void OnError(String reason) 
            {
                listener.OnError(message, CompletionListenerConstants.PUBLISH_FAILED, reason);
            }

            public void OnError(int code, String reason) 
            {
                listener.OnError(message, code, reason);
            }
        }
    }
}
