/*
 * Copyright (c) 2013-$Date: 2016-01-25 19:33:46 -0600 (Mon, 25 Jan 2016) $ TIBCO Software Inc.
 * Licensed under a BSD-style license. Refer to [LICENSE]
 * For more information, please contact:
 * TIBCO Software Inc., Palo Alto, California, USA
 *
 * $Id: WebSocketException.cs 83756 2016-01-26 01:33:46Z bmahurka $
 *
 */
using System;

namespace TIBCO.EFTL
{
    public class WebSocketException: Exception 
    {
        public WebSocketException(String message): base(message) {}
    }
}
