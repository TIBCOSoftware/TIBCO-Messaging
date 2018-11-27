/*
 * Copyright (c) 2001-$Date: 2015-10-27 00:07:01 -0500 (Tue, 27 Oct 2015) $ TIBCO Software Inc.
 * Licensed under a BSD-style license. Refer to [LICENSE]
 * For more information, please contact:
 * TIBCO Software Inc., Palo Alto, California, USA
 *
 * $Id: ICompletionListener.cs 82627 2015-10-27 05:07:01Z bmahurka $
 *
 */
using System;
using System.Collections;
using System.Linq;
using System.Text;

namespace TIBCO.EFTL
{
    /// <summary>
    /// Completion event handler.
    /// <p>
    /// Implement this interface to process completion events.
    /// </p>
    /// </summary>
    /// Supply an instance when you call
    /// <see cref="IConnection.Publish(IMessage, ICompletionListener)"/>
    ///
    public interface ICompletionListener
    {
        ///
        /// A publish operation has completed successfully.
        /// 
        /// <param name="message"> This message has been published.
        /// </param>        
        ///
        void OnCompletion(IMessage message);
        
        ///
        /// A publish operation resulted in an error.
        /// <p>
        /// The message was not forwarded by the eFTL server.
        /// </p>
        /// <p>
        /// When developing a client application, consider alerting the
        /// user to the error.
        /// </p>
        /// Possible codes include:
        /// <list type="bullet">
        ///    <item> <description> <see cref="CompletionListenerConstants.PUBLISH_FAILED" /> </description> </item>
        ///    <item> <description> <see cref="CompletionListenerConstants.PUBLISH_DISALLOWED" /> </description> </item>
        /// </list>
        /// 
        /// <param name="message"> This message was <i> not </i> published.
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
        void OnError(IMessage message, int code, String reason);
    }
}
