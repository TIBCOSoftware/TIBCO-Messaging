/*
 * Copyright (c) 2001-$Date: 2017-01-31 13:28:19 -0800 (Tue, 31 Jan 2017) $ TIBCO Software Inc.
 * Licensed under a BSD-style license. Refer to [LICENSE]
 * For more information, please contact:
 * TIBCO Software Inc., Palo Alto, California, USA
 *
 * $Id: JSONMessage.cs 91161 2017-01-31 21:28:19Z $
 *
 */

using System;

namespace TIBCO.EFTL 
{
    public class KVMap : IKVMap 
    {

        private WebSocketConnection conn;
        private String name;

        internal KVMap(WebSocketConnection conn, String name) 
        {
            this.conn = conn;
            this.name = name;
        }

        public void Set(String key, IMessage value, IKVMapListener listener) 
        {
            conn.MapSet(name, key, value, listener);
        }

        public void Get(String key, IKVMapListener listener) 
        {
            conn.MapGet(name, key, listener);
        }

        public void Remove(String key, IKVMapListener listener) 
        {
            conn.MapRemove(name, key, listener);
        }
    }
}
