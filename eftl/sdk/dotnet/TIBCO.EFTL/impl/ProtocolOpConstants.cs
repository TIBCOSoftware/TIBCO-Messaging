/*
 * Copyright (c) 2001-$Date$ TIBCO Software Inc.
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
    internal static class ProtocolOpConstants
    {
        // Access Point protocol op codes
        internal const int OP_HEARTBEAT =     0;
        internal const int OP_LOGIN =         1;
        internal const int OP_WELCOME =       2;
        internal const int OP_SUBSCRIBE =     3;
        internal const int OP_SUBSCRIBED =    4;
        internal const int OP_UNSUBSCRIBE =   5;
        internal const int OP_UNSUBSCRIBED =  6;
        internal const int OP_EVENT =         7;
        internal const int OP_MESSAGE =       8;
        internal const int OP_ACK =           9;
        internal const int OP_ERROR =         10;
        internal const int OP_DISCONNECT =    11;
        internal const int OP_GOODBYE =       12;
        internal const int OP_REQUEST =       13;
        internal const int OP_REQUEST_REPLY = 14;
        internal const int OP_REPLY =         15;
        internal const int OP_MAP_CREATE =    16;
        internal const int OP_MAP_DESTROY =   18;
        internal const int OP_MAP_SET =       20;
        internal const int OP_MAP_GET =       22;
        internal const int OP_MAP_REMOVE =    24;
        internal const int OP_MAP_RESPONSE =  26;
        internal const int OP_STOP =          28;
        internal const int OP_START =         30;
        internal const int OP_STARTED =       31;
    }
}

