/*
 * Copyright (c) 2001-$Date: 2016-03-28 10:11:38 -0700 (Mon, 28 Mar 2016) $ TIBCO Software Inc.
 * Licensed under a BSD-style license. Refer to [LICENSE]
 * For more information, please contact:
 * TIBCO Software Inc., Palo Alto, California, USA
 *
 * $Id: PublishContext.cs 85026 2016-03-28 17:11:38Z bpeterse $
 *
 */

using System;

namespace TIBCO.EFTL 
{
    internal class MapContext : RequestContext 
    {
        private String key;
        private IMessage value;
        private IKVMapListener listener;

        internal MapContext(Int64 seqNum, String json, String key, IMessage value, IKVMapListener listener) : base (seqNum, json)
        { 
            this.key = key;
            this.value = value;
            this.listener = listener;
        }

        internal MapContext(Int64 seqNum, String json, String key, IKVMapListener listener) : base (seqNum, json)
        { 
            this.key = key;
            this.listener = listener;
        }

        internal override bool HasListener()
        {
            return (listener != null);
        }

        internal override void OnSuccess(IMessage response) 
        {
            if (listener != null) 
                listener.OnSuccess(key, (value != null ? value : response));
        }

        internal override void OnError(String reason) 
        {
            if (listener != null) 
                listener.OnError(key, value, KVMapListenerConstants.REQUEST_FAILED, reason);
        }

        internal override void OnError(int code, String reason) 
        {
            if (listener != null) 
                listener.OnError(key, value, code, reason);
        }
    }
}
