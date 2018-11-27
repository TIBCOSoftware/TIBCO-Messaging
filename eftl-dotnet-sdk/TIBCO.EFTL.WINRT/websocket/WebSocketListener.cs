/*
 * Copyright (c) 2013-$Date: 2016-01-25 19:33:46 -0600 (Mon, 25 Jan 2016) $ TIBCO Software Inc.
 * Licensed under a BSD-style license. Refer to [LICENSE]
 * For more information, please contact:
 * TIBCO Software Inc., Palo Alto, California, USA
 *
 * $Id: WebSocketListener.cs 83756 2016-01-26 01:33:46Z bmahurka $
 *
 */
using System;
using System.Collections;
using System.Linq;
using System.Text;

namespace TIBCO.EFTL
{
    public interface WebSocketListener 
    {
        void OnOpen();
        
        void OnClose(int code, String reason);
    
        void OnError(Exception cause);
        
        void OnMessage(String text);
        
        void OnMessage(byte[] data, int offset, int length);
        
        void OnPong(byte[] data, int offset, int length);
    }
}
