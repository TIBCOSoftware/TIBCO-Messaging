/*                              
 * Copyright (c) 2001-$Date: 2016-01-25 19:33:46 -0600 (Mon, 25 Jan 2016) $ TIBCO Software Inc.
 * Licensed under a BSD-style license. Refer to [LICENSE]
 * For more information, please contact:
 * TIBCO Software Inc., Palo Alto, California, USA
 *
 * $Id: ISubscriptionListener.cs 83756 2016-01-26 01:33:46Z bmahurka $
 *
 */
using System;
using System.Collections;
using System.Linq;
using System.Text;

namespace TIBCO.EFTL
{
    /// <summary>
    /// Subscription event handler.
    /// </summary>
    /// <p>
    /// You must implement this interface to process inbound messages and
    /// other subscription events.
    /// </p>
    /// <p>
    /// Supply an instance when you call
    /// <see cref="IConnection.Subscribe(String, String, ISubscriptionListener)"/>
    /// </p>
    ///
    public interface ISubscriptionListener
    {
        /// <summary>
        /// Process inbound messages.
        /// </summary>
        /// <p>
        /// The eFTL library presents inbound messages to this method for
        /// processing.  You must implement this method to process the
        /// messages.
        /// </p>
        /// <p>
        /// The messages are not thread-safe.  You may access a
        /// message in any thread, but in only one thread at a time.
        /// </p>      
        ///
        /// <param name="messages"> Inbound messages. 
        /// </param>
        ///
        void OnMessages(IMessage[] messages);
        
        /// <summary>
        /// A new subscription is ready to receive messages.
        /// </summary>
        /// <p>
        /// The eFTL library may invoke this method after the first
        /// message arrives. 
        /// </p>
        /// 
        /// <p>
        /// To close the subscription,
        /// call <see cref="IConnection.Unsubscribe"/>
        /// with this subscription identifier.
        /// </p>
        ///
        /// <param name="subscriptionId"> This subscription is ready.
        /// </param>
        ///
        void OnSubscribe(String subscriptionId);
        
        /// <summary>
        /// Process subscription errors.
        /// </summary>
        /// <p>
        /// Possible returned codes include:
        /// <list type="bullet">
        /// <item><see cref="SubscriptionConstants.SUBSCRIPTIONS_DISALLOWED"/> </item>
        /// <item><see cref="SubscriptionConstants.SUBSCRIPTION_FAILED"/> </item>
        /// <item><see cref="SubscriptionConstants.SUBSCRIPTION_INVALID"/> </item>
        /// </list>
        /// </p>
        /// 
        /// <param name="subscriptionId"> eFTL could not establish this subscription.
        /// </param>
        /// <param name="code"> This code categorizes the error.
        ///             Your program can use this value in its response logic.
        /// </param>
        /// <param name="reason"> This string provides more detail.  Your program
        ///               can use this value in error reporting and logging. 
        /// </param>
        ///
        void OnError(String subscriptionId, int code, String reason);
    }
}
