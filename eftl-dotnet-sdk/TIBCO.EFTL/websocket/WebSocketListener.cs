/*
 * Copyright (c) 2013-$Date: 2014-10-06 14:32:56 -0500 (Mon, 06 Oct 2014) $ TIBCO Software Inc.
 * Licensed under a BSD-style license. Refer to [LICENSE]
 * For more information, please contact:
 * TIBCO Software Inc., Palo Alto, California, USA
 *
 * $Id: WebSocketListener.cs 75907 2014-10-06 19:32:56Z bmahurka $
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
