/*
 * Copyright (c) 2001-$Date: 2016-03-11 16:29:10 -0800 (Fri, 11 Mar 2016) $ TIBCO Software Inc.
 * Licensed under a BSD-style license. Refer to [LICENSE]
 * For more information, please contact:
 * TIBCO Software Inc., Palo Alto, California, USA
 *
 * $Id: JsonArray.java 84680 2016-03-12 00:29:10Z bpeterse $
 */

using System.Collections;
using System.Collections.Generic;
using System.Text;

public sealed class JsonArray : List<object>
{
    public JsonArray() : base()
    {
    }

    public override string ToString()
    {
        StringBuilder sb = new StringBuilder();
        WriteJson(this, sb);
        return sb.ToString();
    }

    public static void WriteJson(JsonArray jsonArray, StringBuilder sb)
    {
        bool first = true;
        sb.Append('[');
        foreach (object value in jsonArray)
        {
            if (first)
            {
                first = false;
            }
            else
            {
                sb.Append(',');
            }
            JsonValue.WriteJson(value, sb);
        }
        sb.Append(']');
    }
}
