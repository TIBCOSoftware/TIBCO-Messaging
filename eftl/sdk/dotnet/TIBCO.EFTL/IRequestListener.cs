/*
 * Copyright (c) 2001-$Date: 2015-10-26 22:07:01 -0700 (Mon, 26 Oct 2015) $ TIBCO Software Inc.
 * Licensed under a BSD-style license. Refer to [LICENSE]
 * For more information, please contact:
 * TIBCO Software Inc., Palo Alto, California, USA
 *
 * $Id: ICompletionListener.cs 82627 2015-10-27 05:07:01Z $
 *
 */
using System;
using System.Collections;
using System.Linq;
using System.Text;

namespace TIBCO.EFTL
{
    /// <summary>
    /// Request event handler.
    /// <p>
    /// Implement this interface to process request events.
    /// </p>
    /// </summary>
    /// Supply an instance when you call
    /// <see cref="IConnection.SendRequest(IMessage, double, IRequestListener)"/>
    ///
    public interface IRequestListener
    {
        ///
        /// A request has received a reply.
        /// 
        /// <param name="reply"> The reply message.
        /// </param>        
        ///
        void OnReply(IMessage reply);
        
        ///
        /// A request resulted in an error.
        /// <p>
        /// The message was not forwarded by the eFTL server, or no reply
        /// was received within the specified timeout.
        /// </p>
        /// Possible codes include:
        /// <list type="bullet">
        ///    <item> <description> <see cref="RequestListenerConstants.REQUEST_FAILED" /> </description> </item>
        ///    <item> <description> <see cref="RequestListenerConstants.REQUEST_DISALLOWED" /> </description> </item>
        ///    <item> <description> <see cref="RequestListenerConstants.REQUEST_TIMEOUT" /> </description> </item>
        /// </list>
        /// 
        /// <param name="request"> The original request message.
        /// </param>       
        /// <param name="code"> This code categorizes the error.  
        ///                     Application programs may use this value
        ///                     in response logic.
        /// </param>
        /// <param name="reason"> This string provides more detail about the 
        ///                       error. Application programs may use this 
        ///                       value for error reporting and logging.
        /// </param>
        ///
        void OnError(IMessage request, int code, String reason);
    }
}
