/*
 * Copyright (c) 2001-$Date: 2018-02-05 16:15:48 -0800 (Mon, 05 Feb 2018) $ TIBCO Software Inc.
 * Licensed under a BSD-style license. Refer to [LICENSE]
 * For more information, please contact:
 * TIBCO Software Inc., Palo Alto, California, USA
 *
 * $Id: IConnection.cs 99237 2018-02-06 00:15:48Z bpeterse $
 *
 */
using System;

namespace TIBCO.EFTL 
{
    /// <summary>
    /// A key-value map  object represents a program's connection to an FTL
    /// map.
    /// </summary>
    /// <description>
    /// Programs use key-value map objects to set, get, and remove key-value
    /// pairs in an FTL map.
    /// </description>
    ///
    public interface IKVMap 
    {
        /// <summary>
        /// Set a key-value pair in the map, overwriting any existing value.
        /// </summary>
        /// <p>
        /// This call returns immediately; setting continues
        /// asynchronously.  When the set completes successfully,
        /// the eFTL library calls your 
        /// <see cref="IKVListener.OnSuccess"/> callback.
        /// </p>
        ///
        /// <param name="key"> Set the value for this key.
        /// </param>
        /// <param name="value"> Set this value for the key.
        /// </param>
        /// <param name="listener"> This listener defines callback methods
        ///                 for successful completion and for errors.
        /// </param>
        ///
        /// <exception cref="Exception"> The connection is not open.
        /// </exception>
        /// <exception cref="Exception"> The message would exceed the 
        /// eFTL server's maximum message size.
        /// </exception>
        ///
        /// <seealso cref="IKVMapListener"/>
        /// <seealso cref="IConnection.CreateKVMap"/>
        ///
        void Set(String key, IMessage value, IKVMapListener listener);

        /// <summary>
        /// Get the value of a key from the map, or <c>null</c> if the key 
        /// is not set
        /// </summary>
        /// <p>
        /// This call returns immediately; getting continues
        /// asynchronously.  When the get completes successfully,
        /// the eFTL library calls your 
        /// <see cref="IKVListener.OnSuccess"/> callback.
        /// </p>
        ///
        /// <param name="key"> Get the value for this key.
        /// </param>
        /// <param name="listener"> This listener defines callback methods
        ///                 for successful completion and for errors.
        /// </param>
        ///
        /// <exception cref="Exception"> The connection is not open.
        /// </exception>
        ///
        /// <seealso cref="IKVMapListener"/>
        /// <seealso cref="IConnection.CreateKVMap"/>
        ///
        void Get(String key, IKVMapListener listener);

        /// <summary>
        /// Remove a key-value pair from the map.
        /// </summary>
        /// <p>
        /// This call returns immediately; removing continues
        /// asynchronously.  When the remove completes successfully,
        /// the eFTL library calls your 
        /// <see cref="IKVListener.OnSuccess"/> callback.
        /// </p>
        ///
        /// <param name="key"> Remove the value for this key.
        /// </param>
        /// <param name="listener"> This listener defines callback methods
        ///                 for successful completion and for errors.
        /// </param>
        ///
        /// <exception cref="Exception"> The connection is not open.
        /// </exception>
        ///
        /// <seealso cref="IKVMapListener"/>
        /// <seealso cref="IConnection.CreateKVMap"/>
        ///
        void Remove(String key, IKVMapListener listener);
    }
}
