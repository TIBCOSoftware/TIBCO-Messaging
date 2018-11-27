/*
 * Copyright (c) 2001-$Date: 2016-01-25 19:33:46 -0600 (Mon, 25 Jan 2016) $ TIBCO Software Inc.
 * Licensed under a BSD-style license. Refer to [LICENSE]
 * For more information, please contact:
 * TIBCO Software Inc., Palo Alto, California, USA
 *
 * $Id: BasicSubscription.cs 83756 2016-01-26 01:33:46Z bmahurka $
 *
 */

using System;
using System.Collections;

namespace TIBCO.EFTL
{
    internal class BasicSubscription
    {
        protected ISubscriptionListener listener;
        protected String contentMatcher;
        protected String id;
        protected String durable;
        protected bool pending;
        
        internal BasicSubscription(String id, String contentMatcher, String durable, ISubscriptionListener listener)
        {
            this.id              = id;
            this.contentMatcher  = contentMatcher;
            this.listener        = listener;
            this.durable         = durable;
        }
        
        internal String getSubscriptionId()
        {
            return id;
        }
        
        internal void setListener(ISubscriptionListener listener)
        {
            this.listener = listener;
        }
        
        internal ISubscriptionListener getListener()
        {
            return listener;
        }
        
        internal String getContentMatcher()
        {
            return contentMatcher;
        }
        
        internal String getDurable()
        {
            return durable;
        }

        internal void setPending(bool pending)
        {
            this.pending = pending;
        }
        
        internal bool isPending()
        {
            return pending;
        }
    }
}
