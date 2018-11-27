/*
 * Copyright (c) 2001-$Date: 2018-09-04 17:57:51 -0500 (Tue, 04 Sep 2018) $ TIBCO Software Inc.
 * Licensed under a BSD-style license. Refer to [LICENSE]
 * For more information, please contact:
 * TIBCO Software Inc., Palo Alto, California, USA
 *
 * $Id: IConnection.cs 103512 2018-09-04 22:57:51Z bpeterse $
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
        /// <description>
        /// See <see cref="EFTL.Connect"/>.
        /// </description>
        /// <returns>
        /// The client's identifier.
        /// </returns>
        String GetClientId();

        /// <summary>
        /// Reopen a closed connection. </summary>
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
        bool IsConnected();

        /// <summary>
        /// Create a <see cref="IMessage"/>.
        /// </summary>
        /// 
        /// <returns> A new message object.
        /// </returns>
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
        IKVMap CreateKVMap(String name);

        /// <summary>
        /// Publish a one-to-many message to all subscribing clients.
        /// </summary>
        /// <p>
        /// It is good practice to publish each message to a specific
        /// destination by using the message field name
        /// <see cref="MessageConstants.FIELD_NAME_DESTINATION"/>.
        /// </p>
        /// <p>
        /// To direct a message to a specific destination,
        /// add a string field to the message; for example:
        /// </p>
        /// <pre>
        /// message.SetString(MessageConstants.FIELD_NAME_DESTINATION, "myTopic");
        /// </pre>
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
        void Publish(IMessage message);

        /// <summary>
        /// Publish a one-to-many message to all subscribing clients.
        /// </summary>
        /// <p>
        /// This call returns immediately; publishing continues
        /// asynchronously.  When the publish completes successfully,
        /// the eFTL library calls your 
        /// <see cref="ICompletionListener.OnCompletion"/> callback.
        /// </p>
        /// <p>
        /// It is good practice to publish each message to a specific
        /// destination by using the message field name
        /// <see cref="MessageConstants.FIELD_NAME_DESTINATION"/>.
        /// </p>
        /// <p>
        /// To direct a message to a specific destination,
        /// add a string field to the message; for example:
        /// </p>
        /// <pre>
        /// message.SetString(MessageConstants.FIELD_NAME_DESTINATION, "myTopic");
        /// </pre>
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
        void Publish(IMessage message, ICompletionListener listener);

        /// <summary>
        /// Subscribe to messages.
        /// </summary>
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
        /// <p>
        /// It is good practice to subscribe to
        /// messages published to a specific destination
        /// using the message field name
        /// <see cref="MessageConstants.FIELD_NAME_DESTINATION"/>.
        /// </p>
        /// <p>
        /// To subscribe for messages published to a specific destination,
        /// create a subscription matcher for that destination; for example:
        /// <code> {"_dest":"myTopic"} </code>
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
        ///
        String Subscribe(String matcher, ISubscriptionListener listener);

        ///
        /// Create a durable subscriber to messages.
        /// <p>
        /// Register a durable subscription for one-to-many messages.
        /// </p>
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
        /// <p>
        /// It is good practice to subscribe to
        /// messages published to a specific destination
        /// using the message field name
        /// <see cref="MessageConstants.FIELD_NAME_DESTINATION"/>.
        /// </p>
        /// <p>
        /// To subscribe for messages published to a specific destination,
        /// create a subscription matcher for that destination; for example:
        /// <code> {"_dest":"myTopic"} </code>
        /// </p>
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
        ///
        String Subscribe(String matcher, String durable, ISubscriptionListener listener);

        /// Create a durable subscriber to messages.
        /// <p>
        /// Register a durable subscription for one-to-many messages.
        /// </p>
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
        ///
        String Subscribe(String matcher, String durable, Hashtable props, ISubscriptionListener listener);

        /// <summary>
        /// Close a subscription.
        /// </summary>
        /// <description>
        /// Programs receive subscription identifiers through their
        /// <see cref="ISubscriptionListener.OnSubscribe"/> methods.
        /// </description>
        /// 
        /// <param name="subscriptionId"> Close this subscription.
        /// </param>
        ///
        void Unsubscribe(String subscriptionId);

        /// <summary>
        /// Close all subscriptions.
        /// </summary>
        /// <description>
        /// <see cref="IConnection.Subscribe(string, ISubscriptionListener)"/>
        /// <see cref="IConnection.Subscribe(string, string, ISubscriptionListener)"/>
        /// </description>
        ///
        void UnsubscribeAll();
    }
}
