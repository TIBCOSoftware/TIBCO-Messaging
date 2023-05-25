/*
 * Copyright (c) 2001-$Date$ TIBCO Software Inc.
 * Licensed under a BSD-style license. Refer to [LICENSE]
 * For more information, please contact:
 * TIBCO Software Inc., Palo Alto, California, USA
 *
 * $Id$
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
    private State state;
 
    enum AcknowledgeMode 
    {
        UNKNOWN,
        AUTO,
        CLIENT,
        NONE,
    }

    enum State
    {
        STOPPED,
        STARTING,
        STARTED,
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
        this.state = State.STARTED;
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

    void setState(State state)
    {
        this.state = state;
    }

    State getState()
    {
        return state;
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
