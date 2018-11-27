/*
 * Copyright (c) 2001-$Date: 2018-09-04 17:57:51 -0500 (Tue, 04 Sep 2018) $ TIBCO Software Inc.
 * Licensed under a BSD-style license. Refer to [LICENSE]
 * For more information, please contact:
 * TIBCO Software Inc., Palo Alto, California, USA
 *
 * $Id: Constants.cs 103512 2018-09-04 22:57:51Z bpeterse $
 *
 */
using System;
using System.Collections;
using System.Linq;
using System.Text;

namespace TIBCO.EFTL 
{
    /// <summary>
    /// Durable subscription types.
    /// </summary>
    public static class DurableType 
    {
        /// <summary>
        /// Shared durable type.
        /// </summary>
        /// <description>
        /// Multiple cooperating subscribers can use the same shared durable
        /// to each receive a portion of the subscription's messages.
        /// </description>
        /// <seealso cref="EFTL.PROPERTY_DURABLE_TYPE"/>
        public static readonly String SHARED = "shared";

        /// <summary>
        /// Last-value durable type.
        /// </summary>
        /// <description>
        /// A last-value durable subscription stores only the most recent message
        /// for each unique value of the <see cref="EFTL.PROPERTY_DURABLE_KEY"/>.
        /// </description>
        public static readonly String LAST_VALUE = "last-value";
    }

    /// <summary>
    /// Subscription errors.
    /// </summary>
    /// <description>
    /// <see cref="ISubscriptionListener.OnError"/>
    /// returns these constants.
    /// </description>
    public static class SubscriptionConstants 
    {
        ///
        /// The administrator has disallowed the user from subscribing for
        /// one-to-many messages.
        ////
        public static readonly int SUBSCRIPTIONS_DISALLOWED = 13;

        /// 
        /// The eFTL server could not establish the subscription.
        /// <p>
        /// To determine the root cause, examine the server log.
        /// </p>
        /// <p>
        /// You may attempt to subscribe again.
        /// </p>
        public static readonly int SUBSCRIPTION_FAILED = 21;

        ///
        /// The client supplied an invalid matcher or durable name in
        /// the subscribe call.
        ///
        public static readonly int SUBSCRIPTION_INVALID = 22;
    }

    /// <summary>
    /// Message constants.
    /// </summary>
    /// <description>
    /// Defines specific constants that can be used with eFTL messages.
    /// </description>
    public static class MessageConstants 
    {
        /// Message field name identifying the destination
        /// of a message.
        /// <p>
        /// To publish a message on a specific destination include this
        /// message field using <see cref="IMessage.SetString"/>.
        /// </p>
        /// <p>
        /// To subscribe to messages published on a specific destination,
        /// use a matcher that includes this message field name.
        /// </p>
        public static readonly String FIELD_NAME_DESTINATION = "_dest";
    }

    /// <summary>
    /// Message field types.
    /// </summary>
    /// <description>
    /// Enumerates the legal types for eFTL message fields.
    /// </description>
    public enum FieldType 
    {
        /// Unknown field 
        UNKNOWN,

        /// String field.
        STRING,

        /// Long field.
        LONG,

        /// Double field.
        DOUBLE,

        /// Date field.
        DATE,

        /// Sub-message field.
        MESSAGE,

        /// Opaque field.
        OPAQUE,

        /// String array field.
        STRING_ARRAY,

        /// Long array field.
        LONG_ARRAY,

        /// Double array field.
        DOUBLE_ARRAY,

        /// Date array field
        DATE_ARRAY,

        /// Message array field.
        MESSAGE_ARRAY
    };

    /// <summary>
    /// Connection errors.
    /// </summary>
    /// <description>
    /// <see cref="IConnectionListener.OnError"/>
    /// returns these constants.
    /// </description>
    public static class ConnectionListenerConstants 
    {
        /// Normal close.
        /// <p>
        /// Your program closed the connection by calling 
        /// <see cref="IConnection.Disconnect"/>.
        /// You may now proceed with clean-up.
        /// </p>
        public static readonly int NORMAL = 1000; /* RFC6455.CLOSE_NORMAL */

        /// The server shut down the connection.
        /// <p>
        /// Programs may wait, then attempt to reconnect.
        /// </p>
        public static readonly int SHUTDOWN = 1001; /* RFC6455.CLOSE_SHUTDOWN */

        /// Protocol error.  Please report this error to %TIBCO.
        /// <p>
        /// Programs may wait, then attempt to connect again.
        /// </p>
        public static readonly int PROTOCOL = 1002; /* RFC6455.CLOSE_PROTOCOL */

        /// Invalid message from server.  Please report this error to %TIBCO.
        ///
        public static readonly int BAD_DATA = 1003; /* RFC6455.CLOSE_BAD_DATA */

        /// Connection error.
        /// <p>
        /// Programs may attempt to reconnect.
        /// </p>
        ///
        public static readonly int CONNECTION_ERROR = 1006; /* RFC6455.CLOSE_NO_CLOSE */

        /// Invalid network data.  Please report this error to %TIBCO.
        public static readonly int BAD_PAYLOAD = 1007; /* RFC6455.CLOSE_BAD_PAYLOAD */

        /// Policy violation. Please report this error to %TIBCO.
        ///
        public static readonly int POLICY_VIOLATION = 1008; /* RFC6455.CLOSE_POLICY_VIOLATION */

        /// Message too large.
        /// <p>
        /// The eFTL server closed the connection because your program sent
        /// a message that exceeds the server's limit.  (Administrators
        /// configure this limit; the default limit is 32 kilobytes.)
        ///  </p>
        /// <p>
        /// You may attempt to reconnect.
        /// </p>
        public static readonly int MESSAGE_TOO_LARGE = 1009; /* RFC6455.CLOSE_MESSAGE_TOO_LARGE */

        /// eFTL server error.  Please report this error to %TIBCO.
        ///
        public static readonly int SERVER_ERROR = 1011; /* RFC6455.CLOSE_SERVER_ERROR */

        /// SSL handshake failed.
        /// <p>
        /// The client could not establish a secure connection to the eFTL
        /// server.
        /// </p>
        /// <p>
        /// Possible diagnoses:
        /// </p>
        /// <ul>
        ///   <li> Client rejected server certificate. </li>
        ///   <li> Encryption protocols do not match. </li>
        /// </ul>
        public static readonly int FAILED_TLS_HANDSHAKE = 1015; /* RFC6455.CLOSE_FAILED_TLS_HANDSHAKE */

        /// Force close.
        /// <p>
        /// The eFTL server closed the connection because another client with
        /// the same client identifier has connected.
        /// </p>
        public static readonly int FORCE_CLOSE = 4000;

        /// Not authenticated.
        /// <p>
        /// The eFTL server could not authenticate the client's username and
        /// password.
        /// </p>
        public static readonly int NOT_AUTHENTICATED = 4002;

        /// Bad subscription identifier.
        /// <p>
        /// Invalid subscription ID in an unsubscribe call.
        /// </p>
        public static readonly int BAD_SUBSCRIPTION_ID = 20;

        /// The server does not allow this user to publish messages.
        /// <p>
        /// Administrators configure permission to publish.
        /// </p>
        public static readonly int SEND_DISALLOWED = 12;
    }

    /// <summary>
    /// Completion errors.
    /// </summary>
    /// <description>
    /// <see cref="ICompletionListener.OnError"/>
    /// returns these constants.
    /// </description>
    public static class CompletionListenerConstants 
    {
        /// The server failed to forward the published message.
        /// <p>
        /// To determine the root cause, examine the server log.
        /// </p>
        /// <p>
        /// You may attempt to publish again.
        /// </p>
        ///
        public static readonly int PUBLISH_FAILED = 11;

        /// The server does not allow this user to publish messages.
        /// <p>
        /// Administrators configure permission to publish.
        /// </p>
        public static readonly int PUBLISH_DISALLOWED = 12;
    }

    /// <summary>
    /// KVMap errors.
    /// </summary>
    /// <description>
    /// <see cref="IKVMapListener.OnError"/>
    /// returns these constants.
    /// </description>
    public static class KVMapListenerConstants 
    {
        /// The map operation failed.
        /// <p>
        /// To determine the root cause, examine the server log.
        /// </p>
        /// <p>
        /// You may attempt the operation again.
        /// </p>
        ///
        public static readonly int REQUEST_FAILED = 30;

        /// The user is not authorized for map operations.
        /// <p>
        /// Administrators configure permissions. 
        /// </p>
        public static readonly int REQUEST_DISALLOWED = 14;
    }
}
