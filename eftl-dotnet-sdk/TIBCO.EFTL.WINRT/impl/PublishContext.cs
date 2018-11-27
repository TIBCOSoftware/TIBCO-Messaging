/*
 * Copyright (c) 2001-$Date: 2016-03-28 12:11:38 -0500 (Mon, 28 Mar 2016) $ TIBCO Software Inc.
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
    public class PublishContext
    {
        Int64     seqNum;
        protected ICompletionListener listener;
        protected IMessage message;
        protected String json;
        
        public PublishContext(String json)
        {
            this.json = json;
        }

        public PublishContext(Int64 seqNum, String json, IMessage message, ICompletionListener listener)
        {
            this.seqNum   = seqNum;
            this.json     = json;
            this.message  = message;
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
        
        internal IMessage GetMessage()
        {
            return message;
        }
        
        internal ICompletionListener GetListener()
        {
            return listener;
        }
    }
}
