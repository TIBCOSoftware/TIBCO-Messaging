/*
 * Copyright (c) 2001-$Date: 2020-06-16 09:06:49 -0700 (Tue, 16 Jun 2020) $ TIBCO Software Inc.
 * Licensed under a BSD-style license. Refer to [LICENSE]
 * For more information, please contact:
 * TIBCO Software Inc., Palo Alto, California, USA
 *
 * $Id: BasicSubscription.cs 126238 2020-06-16 16:06:49Z $
 *
 */

using System;
using System.Collections;

namespace TIBCO.EFTL
{
    internal class BasicSubscription
    {
        protected ISubscriptionListener listener;
        protected AckMode ackMode;
        protected Hashtable props;
        protected String contentMatcher;
        protected String id;
        protected String durable;
        protected bool pending;
        protected long lastSeqNum;
 
        internal enum AckMode { Unknown, Auto, Client, None };
        
        internal String Id
        {
            get { return id; }
        }
 
        internal bool AutoAck
        {
            get { return ackMode == AckMode.Auto; }
        }
      
        internal long LastSeqNum
        {
            get { return lastSeqNum; }
            set { lastSeqNum = value; }
        }

        internal bool Pending
        {
            get { return pending; }
            set { pending = value; }
        }

        internal BasicSubscription(String id, String contentMatcher, String durable, Hashtable props, ISubscriptionListener listener)
        {
            this.id              = id;
            this.contentMatcher  = contentMatcher;
            this.listener        = listener;
            this.durable         = durable;
            this.props           = props;
            this.ackMode         = getAckMode(props);
            this.pending         = true;
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

        internal Hashtable getProperties()
        {
            return props;
        }

        internal static AckMode getAckMode(Hashtable props)
        {
            if (props == null)
                return AckMode.Auto;

            String ackMode = (String) props[EFTL.PROPERTY_ACKNOWLEDGE_MODE];

            if (ackMode == null)
                return AckMode.Auto;

            if (ackMode == AcknowledgeMode.AUTO)
                return AckMode.Auto;

            if (ackMode == AcknowledgeMode.CLIENT)
                return AckMode.Client;

            if (ackMode == AcknowledgeMode.NONE)
                return AckMode.None;

            return AckMode.Unknown;
        }
    }
}
