/*
 * Copyright (c) 2001-$Date: 2015-10-26 22:07:01 -0700 (Mon, 26 Oct 2015) $ TIBCO Software Inc.
 * Licensed under a BSD-style license. Refer to [LICENSE]
 * For more information, please contact:
 * TIBCO Software Inc., Palo Alto, California, USA
 *
 * $Id: ICompletionListener.cs 82627 2015-10-27 05:07:01Z bmahurka $
 *
 */
using System;

namespace TIBCO.EFTL 
{
    /// <summary>
    /// Key-value map event handler.
    /// <p>
    /// Implement this interface to process key-value map events.
    /// </p>
    /// </summary>
    /// Supply an instance when you call
    /// <see cref="IKVMap.Set(String, IMessage, IKVMapListener)"/>
    /// <see cref="IKVMap.Get(String, IKVMapListener)"/>
    /// <see cref="IKVMap.Remove(String, IKVMapListener)"/>
    ///
    public interface IKVMapListener 
    {
        ///
        /// A key-value map operation has completed successfully.
        /// 
        /// <param name="key"> The key for the operation.
        /// </param>        
        /// <param name="value"> The value of the key.
        /// </param>        
        ///
        void OnSuccess(String key, IMessage value);

        ///
        /// A key-value map operation resulted in an error.
        ///
        /// <p>
        /// When developing a client application, consider alerting the
        /// user to the error.
        /// </p>
        /// Possible codes include:
        /// <list type="bullet">
        ///    <item> <description> <see cref="KVMapListenerConstants.REQUEST_FAILED" /> </description> </item>
        ///    <item> <description> <see cref="KVMapListenerConstants.REQUEST_DISALLOWED" /> </description> </item>
        /// </list>
        /// 
        /// <param name="key"> The key for the operation.
        /// </param>       
        /// <param name="value"> The value of the key.
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
        void OnError(String key, IMessage value, int code, String reason);
    }
}
