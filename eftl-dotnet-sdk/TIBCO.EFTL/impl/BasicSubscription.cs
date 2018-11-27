/*
 * Copyright (c) 2001-$Date: 2018-02-05 18:15:48 -0600 (Mon, 05 Feb 2018) $ TIBCO Software Inc.
 * Licensed under a BSD-style license. Refer to [LICENSE]
 * For more information, please contact:
 * TIBCO Software Inc., Palo Alto, California, USA
 *
 * $Id: BasicSubscription.cs 99237 2018-02-06 00:15:48Z bpeterse $
 *
 */

using System;
using System.Collections;

namespace TIBCO.EFTL
{
    internal class BasicSubscription
    {
        protected ISubscriptionListener listener;
        protected Hashtable props;
        protected String contentMatcher;
        protected String id;
        protected String durable;
        protected bool pending;
        
        internal BasicSubscription(String id, String contentMatcher, String durable, Hashtable props, ISubscriptionListener listener)
        {
            this.id              = id;
            this.contentMatcher  = contentMatcher;
            this.listener        = listener;
            this.durable         = durable;
            this.props           = props;
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

        internal Hashtable getProperties()
        {
            return props;
        }
    }
}
