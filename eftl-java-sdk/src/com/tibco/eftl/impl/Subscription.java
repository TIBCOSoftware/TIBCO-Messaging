/*
 * Copyright (c) 2001-$Date: 2018-02-05 18:15:48 -0600 (Mon, 05 Feb 2018) $ TIBCO Software Inc.
 * Licensed under a BSD-style license. Refer to [LICENSE]
 * For more information, please contact:
 * TIBCO Software Inc., Palo Alto, California, USA
 *
 * $Id: Subscription.java 99237 2018-02-06 00:15:48Z bpeterse $
 *
 */
package com.tibco.eftl.impl;

import java.util.Properties;

import com.tibco.eftl.*;

public class Subscription
{
    protected SubscriptionListener listener;
    protected Properties props;
    protected String durable;
    protected String matcher;
    protected String id;
    protected boolean pending;
    
    protected Subscription(String id, String matcher, String durable, Properties props, SubscriptionListener listener)
    {
        this.id = id;
        this.matcher = matcher;
        this.durable = durable;
        this.props = props;
        this.listener = listener;
    }
    
    protected String getSubscriptionId()
    {
        return id;
    }
    
    protected void setListener(SubscriptionListener listener)
    {
        this.listener = listener;
    }

    protected SubscriptionListener getListener()
    {
        return listener;
    }

    protected String getMatcher()
    {
        return matcher;
    }
    
    protected String getDurable()
    {
        return durable;
    }
    
    protected void setPending(boolean pending)
    {
        this.pending = pending;
    }
    
    protected boolean isPending()
    {
        return pending;
    }
    
    protected Properties getProperties()
    {
        return props;
    }
}
