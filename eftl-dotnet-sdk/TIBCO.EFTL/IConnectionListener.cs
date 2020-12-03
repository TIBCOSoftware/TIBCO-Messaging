/*
 * Copyright (c) 2001-$Date: 2015-10-26 21:47:25 -0700 (Mon, 26 Oct 2015) $ TIBCO Software Inc.
 * Licensed under a BSD-style license. Refer to [LICENSE]
 * For more information, please contact:
 * TIBCO Software Inc., Palo Alto, California, USA
 *
 * $Id: IConnectionListener.cs 82604 2015-10-27 04:47:25Z bmahurka $
 *
 */
using System;
using System.Collections;
using System.Linq;
using System.Text;

namespace TIBCO.EFTL
{
    /// <summary>
    /// Connection event handler.
    /// 
    /// <p>
    /// You must implement this interface to process connection events.
    /// </p>
    /// </summary>
    /// <p>
    /// Supply an instance when you call
    /// <see cref="EFTL.Connect(String, Hashtable, IConnectionListener)"/>.
    ///  </p>
    ///
    public interface IConnectionListener
    {
        /// <summary>
        /// A new connection to the eFTL server is ready to use.
        /// </summary>
        /// <p>
        /// The eFTL library invokes this method only after your program calls 
        /// <see cref="EFTL.Connect"/>
        /// (and not after <see cref="IConnection.Reconnect"/>).
        ///  </p>
        /// 
        /// <param name="connection"> This connection is ready to use. </param>
        ///
        void OnConnect(IConnection connection);
        
        /// <summary>
        /// A connection to the eFTL server has re-opened and is ready to use.
        /// </summary>
        /// <p>
        /// The eFTL library invokes this method only after your program calls 
        /// <see cref="IConnection.Reconnect"/>
        /// (and not after <see cref="EFTL.Connect"/>).
        ///  </p>
        /// 
        /// <param name="connection"> This connection is ready to use. </param>
        ///
        void OnReconnect(IConnection connection);
        
        /// <summary>
        /// A connection to the eFTL server has closed.
        /// </summary>
        /// <p>
        /// Possible codes include:
        /// <list type="bullet">
        ///    <item><see cref="ConnectionListenerConstants.NORMAL"/> </item>
        ///    <item><see cref="ConnectionListenerConstants.SHUTDOWN"/> </item>
        ///    <item><see cref="ConnectionListenerConstants.CONNECTION_ERROR"/> </item>
        ///    <item><see cref="ConnectionListenerConstants.POLICY_VIOLATION"/> </item>
        ///    <item><see cref="ConnectionListenerConstants.MESSAGE_TOO_LARGE"/> </item>
        ///    <item><see cref="ConnectionListenerConstants.SERVER_ERROR"/> </item>
        ///    <item><see cref="ConnectionListenerConstants.FORCE_CLOSE"/> </item>
        ///    <item><see cref="ConnectionListenerConstants.NOT_AUTHENTICATED"/> </item>
        /// </list>
        /// </p>
        /// 
        /// <param name="connection"> This connection has closed. 
        /// </param>
        /// <param name="code"> This code categorizes the condition.  Your program can
        ///             use this value in its response logic.
        /// </param>
        /// <param name="reason"> This string provides more detail.  Your program
        ///               can use this value in error reporting and
        ///               logging. 
        /// </param>
        ///
        void OnDisconnect(IConnection connection, int code, String reason);
        
        /// <summary>
        /// An error prevented an operation.  The connection remains open.
        /// </summary>
        /// <p>
        /// Your implementation could alert the user.
        /// </p>
        /// Possible codes include:
        /// <list type="bullet">
        ///    <item><see cref="ConnectionListenerConstants.BAD_SUBSCRIPTION_ID"/> </item>
        ///    <item><see cref="ConnectionListenerConstants.SEND_DISALLOWED"/> </item>
        /// </list>
        /// <param name="connection"> This connection reported an error. 
        /// </param>
        /// <param name="code"> This code categorizes the error.  Your program can
        ///             use this value in its response logic. 
        /// </param>
        /// <param name="reason"> This string provides more detail.  Your program
        ///               can use this value in error reporting and
        ///               logging. 
        /// </param>
        ////
        void OnError(IConnection connection, int code, String reason);
    }
}
