/*
 * Copyright (c) 2001-$Date: 2017-07-12 12:07:25 -0500 (Wed, 12 Jul 2017) $ TIBCO Software Inc.
 * Licensed under a BSD-style license. Refer to [LICENSE]
 * For more information, please contact:
 * TIBCO Software Inc., Palo Alto, California, USA
 *
 * $Id: EFTL.cs 94582 2017-07-12 17:07:25Z bpeterse $
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
        /// Get the version of the eFTL Java client library.
        /// </summary>
        /// 
        /// <returns> The version of the eFTL Java client library.
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
        /// A program that uses more than one server channel must connect
        /// separately to each channel.
        /// </p>
        ///
        /// <param name="url">
        /// The call connects to the eFTL server at this URL.
        ///   The URL can be in either of these forms:
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
