/*
 * Copyright (c) 2001-$Date: 2020-09-17 09:04:34 -0700 (Thu, 17 Sep 2020) $ TIBCO Software Inc.
 * Licensed under a BSD-style license. Refer to [LICENSE]
 * For more information, please contact:
 * TIBCO Software Inc., Palo Alto, California, USA
 *
 * $Id: IConnection.cs 128659 2020-09-17 16:04:34Z $
 *
 */
using System;
using System.Collections;
using System.Linq;
using System.Text;

namespace TIBCO.EFTL 
{
    /// <summary>
    /// A connection object represents a program's connection to an eFTL
    /// server.
    /// </summary>
    ///
    /// <description>
    /// Programs use connection objects to create messages, send messages,
    /// and subscribe to messages.
    /// <p>
    /// Programs receive connection objects through
    /// <see cref="IConnectionListener"/> callbacks.
    /// </p>
    /// </description>
    ///
    public interface IConnection : IDisposable 
    {
        /// <summary>
        /// Gets the client identifier for this connection.
        /// </summary>
        /// 
        /// <returns>
        /// The client's identifier.
        /// </returns>
        /// 
        /// <seealso cref="EFTL.Connect"/>.
        ///
        String GetClientId();

        /// <summary>
        /// Reopen a closed connection. 
        /// </summary>
        ///
        /// <description>
        /// <p>
        /// You may call this method within your
        /// <see cref="IConnectionListener.OnDisconnect"/> method.
        /// </p>
        /// <p>
        /// This call returns immediately; connecting continues asynchronously.
        /// When the connection is ready to use, the eFTL library calls your
        /// <see cref="IConnectionListener.OnReconnect"/> callback.
        /// </p>
        /// <p>
        /// Reconnecting automatically re-activates all
        /// subscriptions on the connection.
        /// The eFTL library invokes your
        /// <see cref="ISubscriptionListener.OnSubscribe"/> callback
        /// for each successful resubscription.
        /// </p>
        /// </description>
        /// 
        /// <param name="props"> These properties affect the connection attempt.
        /// You must supply username and password credentials each time.  All 
        /// other properties remain stored from earlier connect and reconnect
        /// calls.  New values overwrite stored values.
        /// </param>
        ///
        /// <seealso cref="EFTL.PROPERTY_CLIENT_ID"/>
        /// <seealso cref="EFTL.PROPERTY_USERNAME"/>
        /// <seealso cref="EFTL.PROPERTY_PASSWORD"/>
        /// <seealso cref="EFTL.PROPERTY_TIMEOUT"/>
        /// <seealso cref="EFTL.PROPERTY_TRUST_ALL"/>
        ///
        void Reconnect(System.Collections.Hashtable props);

        /// <summary>
        /// Disconnect from the eFTL server.
        /// </summary>
        ///
        /// <description>
        /// <p>
        /// Programs may disconnect to free server resources.
        /// </p>
        /// This call returns immediately; disconnecting continues
        /// asynchronously.
        /// When the connection has closed, the eFTL library calls your
        /// <see cref="IConnectionListener.OnDisconnect"/>} callback.
        /// </description>
        ///
        void Disconnect();

        /// <summary>
        /// Determine whether this connection to the eFTL server is open or
        /// closed.
        /// </summary>
        ///
        /// <returns> <c> true </c> if this connection is open;
        /// <c> false </c> otherwise.
        /// </returns>
        ///
        bool IsConnected();

        /// <summary>
        /// Create a <see cref="IMessage"/>.
        /// </summary>
        ///
        /// <returns> A new message object.
        /// </returns>
        ///
        IMessage CreateMessage();

        /// <summary>
        /// Create a <see cref="IKVMap"/>.
        /// </summary>
        /// 
        /// <param name="name"> Key-value map name.
        /// </param>
        ///
        /// <returns> A new key-value map object.
        /// </returns>
        ///
        IKVMap CreateKVMap(String name);

        /// <summary>
        /// Remove a key-value map.
        /// </summary>
        /// 
        /// <param name="name"> Key-value map name.
        /// </param>
        ///
        void RemoveKVMap(String name);

        /// <summary>
        /// Publish a request message. 
        /// </summary>
        ///
        /// <description>
        /// This call returns immediately. When the reply is received
        /// the eFTL library calls your 
        /// <see cref="IRequestListener.OnReply"/> callback.
        /// </description>
        ///
        /// <param name="request"> The request message to publish.
        /// </param>
        /// <param name="timeout"> Seconds to wait for a reply.
        /// </param>
        /// <param name="listener"> This listener defines callback methods for
        ///                successful completion and for errors.
        /// </param>
        ///
        /// <exception cref="Exception"> The connection is not open.
        /// </exception>
        /// <exception cref="Exception"> The message would exceed the 
        /// eFTL server's maximum message size.
        /// </exception>
        ///
        /// <seealso cref="MessageConstants.FIELD_NAME_DESTINATION"/>
        ///
        void SendRequest(IMessage request, double timeout, IRequestListener listener);

        /// <summary>
        /// Send a reply message in response to a request message.
        /// </summary>
        ///
        /// <description>
        /// This call returns immediately. When the send completes 
        /// successfully, the eFTL library calls your 
        /// <see cref="ICompletionListener.OnCompletion"/> callback.
        /// </description>
        ///
        /// <param name="reply"> The reply message to send.
        /// </param>
        /// <param name="request"> The request message.
        /// </param>
        /// <param name="listener"> This listener defines callback methods for
        ///                successful completion and for errors.
        /// </param>
        ///
        /// <exception cref="Exception"> The connection is not open.
        /// </exception>
        /// <exception cref="Exception"> The message would exceed the 
        /// eFTL server's maximum message size.
        /// </exception>
        ///
        void SendReply(IMessage reply, IMessage request, ICompletionListener listener);

        /// <summary>
        /// Publish a one-to-many message to all subscribing clients.
        /// </summary>
        ///
        /// <param name="message"> Publish this message.
        /// </param>
        ///
        /// <exception cref="Exception"> The connection is not open.
        /// </exception>
        /// <exception cref="Exception"> The message would exceed the 
        /// eFTL server's maximum message size.
        /// </exception>
        ///
        /// <seealso cref="MessageConstants.FIELD_NAME_DESTINATION"/>
        ///
        void Publish(IMessage message);

        /// <summary>
        /// Publish a one-to-many message to all subscribing clients.
        /// </summary>
        ///
        /// <description>
        /// This call returns immediately; publishing continues
        /// asynchronously.  When the publish completes successfully,
        /// the eFTL library calls your 
        /// <see cref="ICompletionListener.OnCompletion"/> callback.
        /// </description>
        ///
        /// <param name="message"> Publish this message.
        /// </param>
        /// <param name="listener"> This listener defines callback methods for
        ///                successful completion and for errors.
        /// </param>
        ///
        /// <exception cref="Exception"> The connection is not open.
        /// </exception>
        /// <exception cref="Exception"> The message would exceed the 
        /// eFTL server's maximum message size.
        /// </exception>
        ///
        /// <seealso cref="MessageConstants.FIELD_NAME_DESTINATION"/>
        ///
        void Publish(IMessage message, ICompletionListener listener);

        /// <summary>
        /// Subscribe to messages.
        /// </summary>
        ///
        /// <description>
        /// Register a subscription for one-to-many messages.
        /// <p>
        /// This call returns immediately; subscribing continues
        /// asynchronously.  When the subscription is
        /// ready to receive messages, the eFTL library calls your 
        /// <see cref="ISubscriptionListener.OnSubscribe"/> callback.
        /// </p>
        /// <p>
        /// A matcher can narrow subscription interest in the inbound
        /// message stream.
        /// </p>
        /// </description>
        ///
        /// <param name="matcher">
        /// The subscription uses this content matcher to
        /// narrow the message stream. 
        /// </param>
        /// <param name="listener">
        /// This listener defines callback methods for
        /// successful subscription, message arrival, and errors.
        /// </param>        
        /// 
        /// <returns> An identifier that represents the new subscription.
        /// </returns>
        ///
        /// <exception cref="Exception"> The connection is not open.
        /// </exception>
        /// 
        /// <seealso cref="ISubscriptionListener"/>
        /// <seealso cref="IConnection.Unsubscribe"/>
        /// <seealso cref="IConnection.CloseSubscription"/>
        ///
        /// <seealso cref="MessageConstants.FIELD_NAME_DESTINATION"/>
        ///
        String Subscribe(String matcher, ISubscriptionListener listener);

        /// <summary>
        /// Create a durable subscriber to messages.
        /// </summary>
        ///
        /// <description>
        /// Register a durable subscription for one-to-many messages.
        /// <p>
        /// This call returns immediately; subscribing continues
        /// asynchronously.  When the subscription is
        /// ready to receive messages, the eFTL library calls your 
        /// <see cref="ISubscriptionListener.OnSubscribe"/> callback.
        /// </p>
        /// <p>
        /// A matcher can narrow subscription interest in the inbound
        /// message stream.
        /// </p>
        /// </description>
        ///
        /// <param name="matcher"> The subscription uses this matcher to
        ///                    narrow the message stream.
        /// </param>
        /// <param name="durable"> The subscription uses this durable name.
        /// </param>
        /// <param name="listener"> This listener defines callback methods for
        ///                 successful subscription, message arrival and
        ///                 errors.
        /// </param>
        ///
        /// <returns> An identifier that represents the new subscription.
        /// </returns>
        ///
        /// <exception cref="Exception"> The connection is not open.
        /// </exception>
        ///
        /// <seealso cref="ISubscriptionListener"/>
        /// <seealso cref="IConnection.Unsubscribe"/>
        /// <seealso cref="IConnection.CloseSubscription"/>
        ///
        /// <seealso cref="MessageConstants.FIELD_NAME_DESTINATION"/>
        ///
        String Subscribe(String matcher, String durable, ISubscriptionListener listener);

        /// <summary>
        /// Create a durable subscriber to messages.
        /// </summary>
        ///
        /// <description>
        /// Register a durable subscription for one-to-many messages.
        /// <p>
        /// This call returns immediately; subscribing continues
        /// asynchronously.  When the subscription is
        /// ready to receive messages, the eFTL library calls your 
        /// <see cref="ISubscriptionListener.OnSubscribe"/> callback.
        /// </p>
        /// <p>
        /// A matcher can narrow subscription interest in the inbound
        /// message stream.
        /// </p>
        /// </description>
        ///
        /// <param name="matcher"> The subscription uses this matcher to
        ///                    narrow the message stream.
        /// </param>
        /// <param name="durable"> The subscription uses this durable name.
        /// </param>
        /// <param name="props">
        /// These properties can be used to affect the subscription:
        ///   <list type="bullet">
        ///     <item> <see cref="EFTL.PROPERTY_DURABLE_TYPE"/> </item>
        ///     <item> <see cref="EFTL.PROPERTY_DURABLE_KEY"/> </item>
        ///   </list>
        /// </param>
        /// <param name="listener"> This listener defines callback methods for
        ///                 successful subscription, message arrival and
        ///                 errors.
        /// </param>
        ///
        /// <returns> An identifier that represents the new subscription.
        /// </returns>
        ///
        /// <exception cref="Exception"> The connection is not open.
        /// </exception>
        ///
        /// <seealso cref="ISubscriptionListener"/>
        /// <seealso cref="IConnection.Unsubscribe"/>
        /// <seealso cref="IConnection.CloseSubscription"/>
        ///
        /// <seealso cref="MessageConstants.FIELD_NAME_DESTINATION"/>
        ///
        String Subscribe(String matcher, String durable, Hashtable props, ISubscriptionListener listener);

        /// <summary>
        /// Close a subscription.
        /// </summary>
        ///
        /// <description>
        /// For durable subscriptions, this will cause the persistence 
        /// service to stop delivering messages while leaving the durable 
        /// subscription to continue accumulating persisted messages. Any
        /// unacknowledged messages will be made available for redelivery.
        /// </description>
        ///
        /// <param name="subscriptionId"> Close this subscription.
        /// </param>
        ///
        /// <seealso cref="IConnection.Subscribe(string, ISubscriptionListener)"/>
        /// <seealso cref="IConnection.Subscribe(string, string, ISubscriptionListener)"/>
        /// <seealso cref="IConnection.Subscribe(string, string, Hashtable, ISubscriptionListener)"/>
        ///
        void CloseSubscription(String subscriptionId);

        /// <summary>
        /// Close all subscriptions.
        /// </summary>
        ///
        /// <description>
        /// For durable subscriptions, this will cause the persistence
        /// service to stop delivering messages while leaving the durable 
        /// subscriptions to continue accumulating persisted messages. Any
        /// unacknowledged messages will be made available for redelivery.
        /// </description>
        ///
        /// <seealso cref="IConnection.Subscribe(string, ISubscriptionListener)"/>
        /// <seealso cref="IConnection.Subscribe(string, string, ISubscriptionListener)"/>
        /// <seealso cref="IConnection.Subscribe(string, string, Hashtable, ISubscriptionListener)"/>
        ///
        void CloseAllSubscriptions();

        /// <summary>
        /// Unsubscribe from messages on a subscription.
        /// </summary>
        ///
        /// <description>
        /// For durable subscriptions, this will cause the persistence 
        /// service to remove the durable subscription, along with any
        /// persisted messages.
        /// </description>
        ///
        /// <param name="subscriptionId"> Unsubscribe this subscription.
        /// </param>
        ///
        /// <seealso cref="IConnection.Subscribe(string, ISubscriptionListener)"/>
        /// <seealso cref="IConnection.Subscribe(string, string, ISubscriptionListener)"/>
        /// <seealso cref="IConnection.Subscribe(string, string, Hashtable, ISubscriptionListener)"/>
        ///
        void Unsubscribe(String subscriptionId);

        /// <summary>
        /// Unsubscribe from messages on all subscriptions.
        /// </summary>
        ///
        /// <description>
        /// For durable subscriptions, this will cause the persistence
        /// service to remove the durable subscriptions, along with any
        /// persisted messages.
        /// </description>
        ///
        /// <seealso cref="IConnection.Subscribe(string, ISubscriptionListener)"/>
        /// <seealso cref="IConnection.Subscribe(string, string, ISubscriptionListener)"/>
        /// <seealso cref="IConnection.Subscribe(string, string, Hashtable, ISubscriptionListener)"/>
        ///
        void UnsubscribeAll();

        /// <summary>
        /// Acknowledge this message.
        /// </summary>
        ///
        /// <description>
        /// Messages consumed from subscriptions with a client acknowledgment mode
        /// must be explicitly acknowledged. The eFTL server will stop delivering
        /// messages to the client once the server's configured maximum number of
        /// unacknowledged messages is reached.
        /// </description>
        ///
        /// <param name="message"> The message being acknowledged.
        /// </param>
        ///
        /// <exception cref="Exception"> The connection is not open.
        /// </exception>
        ///
        /// <seealso cref="AcknowledgeMode.CLIENT"/>
        ///
        void Acknowledge(IMessage message);

        /// <summary>
        /// Acknowledge all messages up to and including this message.
        /// </summary>
        ///
        /// <description>
        /// Messages consumed from subscriptions with a client acknowledgment mode
        /// must be explicitly acknowledged. The eFTL server will stop delivering
        /// messages to the client once the server's configured maximum number of
        /// unacknowledged messages is reached.
        /// </description>
        ///
        /// <param name="message"> The message being acknowledged.
        /// </param>
        ///
        /// <exception cref="Exception"> The connection is not open.
        /// </exception>
        ///
        /// <seealso cref="AcknowledgeMode.CLIENT"/>
        ///
        void AcknowledgeAll(IMessage message);
    }
}
