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
    internal class RequestContext 
    {
        protected String json;
        protected Int64 seqNum;

        internal RequestContext(String json) 
        {
            this.json = json;
        }

        internal RequestContext(Int64 seqNum, String json) 
        {
            this.seqNum = seqNum;
            this.json = json;
        }

        internal Int64 GetSeqNum() 
        {
            return seqNum;
        }

        internal String GetJson() 
        {
            return json;
        }

        internal virtual void OnSuccess(IMessage response) {}

        internal virtual void OnError(String reason) {}

        internal virtual void OnError(int code, String reason) {}

        internal virtual bool HasListener()
        {
            return false;
        }
    }
}
