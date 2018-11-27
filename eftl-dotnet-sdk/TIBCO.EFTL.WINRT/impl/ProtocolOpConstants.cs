/*
 * Copyright (c) 2001-$Date: 2016-01-25 19:33:46 -0600 (Mon, 25 Jan 2016) $ TIBCO Software Inc.
 * Licensed under a BSD-style license. Refer to [LICENSE]
 * For more information, please contact:
 * TIBCO Software Inc., Palo Alto, California, USA
 *
 * $Id: ProtocolOpConstants.cs 83756 2016-01-26 01:33:46Z bmahurka $
 *
 */
using System;
using System.Collections;
using System.Linq;
using System.Text;

namespace TIBCO.EFTL
{
    public static class ProtocolOpConstants
    {
        // Access Point protocol op codes
        public const int OP_HEARTBEAT =    0;
        public const int OP_LOGIN =        1;
        public const int OP_WELCOME =      2;
        public const int OP_SUBSCRIBE =    3;
        public const int OP_SUBSCRIBED =   4;
        public const int OP_UNSUBSCRIBE =  5;
        public const int OP_UNSUBSCRIBED = 6;
        public const int OP_EVENT =        7;
        public const int OP_MESSAGE =      8;
        public const int OP_ACK =          9;
        public const int OP_ERROR =        10;
        public const int OP_DISCONNECT =   11;
        public const int OP_GOODBYE =      12;
    }
}

