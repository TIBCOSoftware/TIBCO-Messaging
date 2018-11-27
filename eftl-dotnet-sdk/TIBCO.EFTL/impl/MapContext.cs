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
        internal MapContext(Int64 seqNum, String json, String key, IMessage value, IKVMapListener listener) : base (seqNum, json, (listener != null ? new MapListener (key, value, listener) : null)) { }

        internal MapContext(Int64 seqNum, String json, String key, IKVMapListener listener) : base (seqNum, json, (listener != null ? new MapListener (key, null, listener) : null)) { }

        internal class MapListener : IRequestListener 
        {
            private String key;
            private IMessage value;
            private IKVMapListener listener;

            internal MapListener(String key, IMessage value, IKVMapListener listener) 
            {
                this.key = key;
                this.value = value;
                this.listener = listener;
            }

            public void OnSuccess(IMessage response) 
            {
                if (value != null)
                    listener.OnSuccess(key, value);
                else
                    listener.OnSuccess(key, response);
            }

            public void OnError(String reason) 
            {
                if (value != null)
                    listener.OnError(key, value, KVMapListenerConstants.REQUEST_FAILED, reason);
                else
                    listener.OnError(key, null, KVMapListenerConstants.REQUEST_FAILED, reason);
            }

            public void OnError(int code, String reason) 
            {
                if (value != null)
                    listener.OnError(key, value, code, reason);
                else
                    listener.OnError(key, null, code, reason);
            }
        }
    }
}
