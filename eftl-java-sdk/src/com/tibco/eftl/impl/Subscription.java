/*
 * Copyright (c) 2001-$Date: 2020-06-16 09:06:49 -0700 (Tue, 16 Jun 2020) $ TIBCO Software Inc.
 * All rights reserved.
 * For more information, please contact:
 * TIBCO Software Inc., Palo Alto, California, USA
 *
 * $Id: Subscription.java 126238 2020-06-16 16:06:49Z bpeterse $
 *
 */
package com.tibco.eftl.impl;

import java.util.Properties;

import com.tibco.eftl.*;

public class Subscription
{
    private SubscriptionListener listener;
    private AcknowledgeMode ackMode;
    private Properties props;
    private String durable;
    private String matcher;
    private String id;
    private boolean pending;
    private long lastSeqNum;
 
    enum AcknowledgeMode 
    {
        UNKNOWN,
        AUTO,
        CLIENT,
        NONE,
    }
    
    Subscription(String id, String matcher, String durable, Properties props, SubscriptionListener listener)
    {
        this.id = id;
        this.matcher = matcher;
        this.durable = durable;
        this.props = props;
        this.listener = listener;
        this.ackMode = getAckMode(props);
        this.pending = true;
    }
    
    String getSubscriptionId()
    {
        return id;
    }
    
    void setListener(SubscriptionListener listener)
    {
        this.listener = listener;
    }

    SubscriptionListener getListener()
    {
        return listener;
    }

    String getMatcher()
    {
        return matcher;
    }
    
    String getDurable()
    {
        return durable;
    }
    
    void setPending(boolean pending)
    {
        this.pending = pending;
    }
    
    boolean isPending()
    {
        return pending;
    }
    
    Properties getProperties()
    {
        return props;
    }
    
    AcknowledgeMode getAckMode() 
    {
        return ackMode;
    }
    
    boolean isAutoAck()
    {
        return ackMode == AcknowledgeMode.AUTO;
    }
    
    long getLastSeqNum()
    {
        return lastSeqNum;
    }
    
    void setLastSeqNum(long seqNum)
    {
        this.lastSeqNum = seqNum;
    }
    
    private static AcknowledgeMode getAckMode(Properties props)
    {
        if (props == null)
            return AcknowledgeMode.AUTO;
        
        String ackMode = props.getProperty(EFTL.PROPERTY_ACKNOWLEDGE_MODE, EFTL.ACKNOWLEDGE_MODE_AUTO);
        
        switch (ackMode)
        {
        case EFTL.ACKNOWLEDGE_MODE_AUTO:   
            return AcknowledgeMode.AUTO;
        case EFTL.ACKNOWLEDGE_MODE_CLIENT: 
            return AcknowledgeMode.CLIENT;
        case EFTL.ACKNOWLEDGE_MODE_NONE:   
            return AcknowledgeMode.NONE;
        default:                           
            return AcknowledgeMode.UNKNOWN;
        }
    }
}
