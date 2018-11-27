/*
 * Copyright (c) 2001-$Date: 2018-09-04 17:57:51 -0500 (Tue, 04 Sep 2018) $ TIBCO Software Inc.
 * Licensed under a BSD-style license. Refer to [LICENSE]
 * For more information, please contact:
 * TIBCO Software Inc., Palo Alto, California, USA
 *
 * $Id: ProtocolOpConstants.cs 103512 2018-09-04 22:57:51Z bpeterse $
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
        internal const int OP_HEARTBEAT =    0;
        internal const int OP_LOGIN =        1;
        internal const int OP_WELCOME =      2;
        internal const int OP_SUBSCRIBE =    3;
        internal const int OP_SUBSCRIBED =   4;
        internal const int OP_UNSUBSCRIBE =  5;
        internal const int OP_UNSUBSCRIBED = 6;
        internal const int OP_EVENT =        7;
        internal const int OP_MESSAGE =      8;
        internal const int OP_ACK =          9;
        internal const int OP_ERROR =        10;
        internal const int OP_DISCONNECT =   11;
        internal const int OP_GOODBYE =      12;
        internal const int OP_MAP_SET =      20;
        internal const int OP_MAP_GET =      22;
        internal const int OP_MAP_REMOVE =   24;
        internal const int OP_MAP_RESPONSE = 26;
    }
}

