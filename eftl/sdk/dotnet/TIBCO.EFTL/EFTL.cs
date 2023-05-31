/*
 * Copyright (c) 2001-$Date$ Cloud Software Group, Inc.
 * Licensed under a BSD-style license. Refer to [LICENSE]
 *
 * $Id$
 *
 */
using System;
using System.Collections;
using System.Linq;
using System.Text;

namespace TIBCO.EFTL
{
    /// <summary>
    /// Programs use class EFTL to connect to an eFTL server.
    /// </summary>
    public class EFTL
    {
        protected EFTL()
        {
        }
        
        /// <summary>
        /// Connect as this user; property name.
        /// </summary>
        /// <description>
        /// Programs use this property to supply a username credential to the
        /// <see cref="EFTL.Connect"/> call if the username is not specified
        /// with the URL.
        /// The server authenticates the user and password.
        /// </description>
        public static readonly String PROPERTY_USERNAME = "user";

        /// <summary>
        /// Connect using this password; property name.
        /// </summary>
        /// <description>
        /// Programs use this property to supply a password credential to the
        /// <see cref="EFTL.Connect"/> call if the password is not specified
        /// with the URL.
        /// The server authenticates the user and password.
        /// </description>
        public static readonly String PROPERTY_PASSWORD = "password";

        /// <summary>
        /// Connection attempt timeout; property name.
        /// </summary>
        /// <description>
        /// Programs use this property to supply a timeout to the
        /// <see cref="EFTL.Connect"/> call.
        /// If the client cannot connect to the server
        /// within this time limit (in seconds),
        /// it stops trying to connect and invokes your
        /// <see cref="IConnectionListener.OnDisconnect"/> method
        /// with a
        /// <see cref="ConnectionListenerConstants.CONNECTION_ERROR"/>
        /// code.
        /// <p>
        /// If you omit this property, the default timeout is 15.0 seconds.
        /// </p>
        /// </description>
        ///
        /// <seealso cref="EFTL.Connect"/>
        ///
        public static readonly String PROPERTY_TIMEOUT = "timeout";

        /// <summary>
        /// Connect using this client identifier; property name.
        /// </summary>
        /// <description>
        /// Programs use this property to supply a client identifier to the
        /// <see cref="EFTL.Connect"/> call if the client identifier is not 
        /// specified with the URL.
        /// The server uses the client
        /// identifier to associate a particular client with a durable
        /// subscription.
        /// <p>
        /// If you omit this property, the server assigns a unique client
        /// identifier.
        /// </p>
        /// </description>
        ///
        public static readonly String PROPERTY_CLIENT_ID = "client_id";

        /// <summary>
        /// Trust any eFTL server certificate; property name.
        /// </summary>
        /// <description>
        /// When present with value <c>true</c>, the client accepts any
        /// eFTL server certificate. Use this setting only in development 
        /// environments. It is not secure, so it is unsuitable for production 
        /// environments.
        /// <p>
        /// When absent or <c>false</c>, the client trusts only certificates 
        /// installed on the client host computer.
        /// </p>
        /// </description>
        public static readonly String PROPERTY_TRUST_ALL = "trust_all";

        /// <summary>
        /// Auto-reconnect maximum attempts; property name.
        /// </summary>
        /// <description>
        /// Programs use this property to supply the maximum number of auto-reconnect attempts to the
        /// <see cref="EFTL.Connect"/> call.
        /// If the connection is lost, the client will attempt to automatically reconnect to the server.
        /// If the number of auto-reconnect attempts exceeds this value,
        /// it stops trying to auto-reconnect and invokes your
        /// <see cref="IConnectionListener.OnDisconnect"/> method
        /// with a
        /// <see cref="ConnectionListenerConstants.CONNECTION_ERROR"/>
        /// code.
        /// <p>
        /// If you omit this property, the default number of attempts is 256.
        /// </p>
        /// </description>
        ///
        /// <seealso cref="EFTL.Connect"/>
        ///
        public static readonly String PROPERTY_AUTO_RECONNECT_ATTEMPTS = "auto_reconnect_attempts";

        /// <summary>
        /// Auto-reconnect maximum delay; property name.
        /// </summary>
        /// <description>
        /// Programs use this property to supply the maximum delay (in seconds) between auto-reconnect attempts to the
        /// <see cref="EFTL.Connect"/> call.
        /// If the connection is lost, the client will attempt to automatically reconnect to the server
        /// after a delay of 1 second. For each subsequent attempt, the delay is doubled, to a maximum
        /// determined by this property.
        /// <p>
        /// If you omit this property, the default maximum delay is 30 seconds.
        /// </p>
        /// </description>
        ///
        /// <seealso cref="EFTL.Connect"/>
        ///
        public static readonly String PROPERTY_AUTO_RECONNECT_MAX_DELAY = "auto_reconnect_max_delay";

        /// <summary>
        /// Maximum number of unacknowledged messages allowed for the client; property name.
        /// </summary>
        /// <description>
        /// Programs use this property to specify the maximum number of unacknowledged messages
        /// allowed for the client. Once the maximum number of unacknowledged messages is reached
        /// the client will stop receiving additional messages until previously received messages
        /// are acknowledged.
        /// <p>
        /// If you omit this property, the server's configured value will be used.
        /// </p>
        /// </description>
        ///
        /// <seealso cref="EFTL.Connect"/>
        ///
        public static readonly String PROPERTY_MAX_PENDING_ACKS = "max_pending_acks";

        /// <summary>
        /// Create a subscription with a specific acknowledgment mode.
        /// </summary>
        /// <description>
        /// Programs use this property to specify how messages received by the
        /// subscription are acknowledged. The following acknowledgment modes are
        /// supported:
        ///   <list type="bullet">
        ///     <item> <see cref="AcknowledgeMode.AUTO"/> </item>
        ///     <item> <see cref="AcknowledgeMode.CLIENT"/> </item>
        ///     <item> <see cref="AcknowledgeMode.NONE"/> </item>
        ///   </list>
        /// <p>
        /// If you omit this property, the subscription is created with an
        /// acknowledgment mode of <see cref="AcknowledgeMode.AUTO"/>.
        /// </p>
        /// </description>
        /// <seealso cref="IConnection.Subscribe(String, String, Hashtable, ISubscriptionListener)"/>
        /// <seealso cref="AcknowledgeMode.AUTO"/>
        /// <seealso cref="AcknowledgeMode.CLIENT"/>
        /// <seealso cref="AcknowledgeMode.NONE"/>
        public static readonly String PROPERTY_ACKNOWLEDGE_MODE = "ack";

        /// <summary>
        /// Create a durable subscription of this type; property name.
        /// </summary>
        /// <description>
        /// Programs use this optional property to supply a durable type to the
        /// <see cref="IConnection.Subscribe(String, String, Hashtable, ISubscriptionListener)"/>
        /// call. If not specified the default durable type will be used.
        /// </description>
        /// <seealso cref="DurableType.SHARED"/>
        /// <seealso cref="DurableType.LAST_VALUE"/>
        public static readonly String PROPERTY_DURABLE_TYPE = "type";

        /// <summary>
        /// Specify the key field of a last-value durable subscription; property name.
        /// </summary>
        /// <description>
        /// Programs use this property to supply a key field to the
        /// <see cref="IConnection.Subscribe(String, String, Hashtable, ISubscriptionListener)"/>
        /// call when the <see cref="EFTL.PROPERTY_DURABLE_TYPE"/> property
        /// is of type <see cref="DurableType.LAST_VALUE"/>.
        /// Note that the supplied key field must be a part of the durable
        /// subscription's matcher.
        /// </description>
        public static readonly String PROPERTY_DURABLE_KEY = "key";

        /// <summary>
        /// Get the version of the eFTL .NET client library.
        /// </summary>
        /// 
        /// <returns> The version of the eFTL .NET client library.
        /// </returns>
        ///
        public static String GetVersion()
        {
            return Version.EFTL_VERSION_STRING_LONG;
        }
        
        /// <summary>
        /// Connect to an eFTL server.
        /// </summary>
        /// <description>
        /// This call returns immediately; connecting continues asynchronously.
        /// When the connection is ready to use, the eFTL library calls your
        /// <see cref="IConnectionListener.OnConnect"/> method, passing an
        /// <see cref="IConnection"/> object that you can use to publish and subscribe.
        /// <p>
        /// When a pipe-separated list of URLs is specified this call will attempt
        /// a connection to each in turn, in a random order, until one is connected.
        /// </p>
        /// <p>
        /// A program that uses more than one server channel must connect
        /// separately to each channel.
        /// </p>
        ///
        /// <param name="url">
        /// The call connects to the eFTL server at this URL. This can be a single
        ///   URL, or a pipe ('|') separated list of URLs. URLs can be in either of
        ///   these forms:
        ///   <list type="bullet">
        ///     <item> <c> ws://host:port/channel </c> </item>
        ///     <item> <c> wss://host:port/channel </c> </item>
        ///   </list>
        /// Optionally, the URL can contain the username, password, and/or
        /// client identifier:
        ///   <list type="bullet">
        ///     <item> <c> ws://username:password@host:port/channel?clientId=&lt;identifier&gt; </c> </item>
        ///     <item> <c> wss://username:password@host:port/channel?clientId=&lt;identifier&gt; </c> </item>
        ///   </list>
        /// </param>
        /// <param name="props">
        /// These properties affect the connection attempt:
        ///   <list type="bullet">
        ///     <item> <see cref="EFTL.PROPERTY_CLIENT_ID"/> </item>
        ///     <item> <see cref="EFTL.PROPERTY_USERNAME"/> </item>
        ///     <item> <see cref="EFTL.PROPERTY_PASSWORD"/> </item>
        ///     <item> <see cref="EFTL.PROPERTY_TIMEOUT"/> </item>
        ///     <item> <see cref="EFTL.PROPERTY_TRUST_ALL"/> </item>
        ///     <item> <see cref="EFTL.PROPERTY_AUTO_RECONNECT_ATTEMPTS"/> </item>
        ///     <item> <see cref="EFTL.PROPERTY_AUTO_RECONNECT_MAX_DELAY"/> </item>
        ///     <item> <see cref="EFTL.PROPERTY_MAX_PENDING_ACKS"/> </item>
        ///   </list>
        /// </param>
        /// <param name="listener">
        /// Connection events invoke methods of this listener. 
        /// </param>
        /// </description>
        ///
        /// <seealso cref="IConnectionListener"/>
        ///
        public static void Connect(String url, Hashtable props, IConnectionListener listener)
        {
            WebSocketConnection ws = new WebSocketConnection(url, listener);
            ws.Connect(props);
        }
    }
}
