/*
 * Copyright (c) 2001-$Date: 2016-01-25 19:33:46 -0600 (Mon, 25 Jan 2016) $ TIBCO Software Inc.
 * Licensed under a BSD-style license. Refer to [LICENSE]
 * For more information, please contact:
 * TIBCO Software Inc., Palo Alto, California, USA
 *
 * $Id: WebSocketFrameConstants.cs 83756 2016-01-26 01:33:46Z bmahurka $
 *
 */

using System;

namespace TIBCO.EFTL
{
    public static class WebSocketFrameConstants
    {
        public const  byte CONTINUATION = 0x00;
        public const  byte TEXT         = 0x01;
        public const  byte BINARY       = 0x02;
        public const  byte CLOSE        = 0x08;
        public const  byte PING         = 0x09;
        public const  byte PONG         = 0x0A;
    }
}
