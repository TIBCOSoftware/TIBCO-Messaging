/*
 * Copyright (c) 2013-$Date$ TIBCO Software Inc.
 * Licensed under a BSD-style license. Refer to [LICENSE]
 * For more information, please contact:
 * TIBCO Software Inc., Palo Alto, California, USA
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
