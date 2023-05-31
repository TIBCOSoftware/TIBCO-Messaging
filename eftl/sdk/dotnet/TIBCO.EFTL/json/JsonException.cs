/*
 * Copyright (c) 2001-$Date: 2016-03-11 16:29:10 -0800 (Fri, 11 Mar 2016) $ Cloud Software Group, Inc.
 * Licensed under a BSD-style license. Refer to [LICENSE]
 *
 * $Id: JsonException.java 84680 2016-03-12 00:29:10Z $
 */

using System;

public class JsonException : Exception
{

    public JsonException(string message) : base(message)
    {
    }

    public JsonException(string message, Exception inner) : base(message, inner)
    {
    }
}
