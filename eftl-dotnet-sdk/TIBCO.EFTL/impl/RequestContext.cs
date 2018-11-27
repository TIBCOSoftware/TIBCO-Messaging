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
        private IRequestListener listener;
        private String json;
        private Int64 seqNum;

        internal RequestContext(String json) 
        {
            this.json = json;
        }

        internal RequestContext(Int64 seqNum, String json, IRequestListener listener) 
        {
            this.seqNum = seqNum;
            this.json = json;
            this.listener = listener;
        }

        internal Int64 GetSeqNum() 
        {
            return seqNum;
        }

        internal String GetJson() 
        {
            return json;
        }

        internal IRequestListener GetListener() 
        {
            return listener;
        }
    }
}
