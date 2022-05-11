/*
 * Copyright (c) 2001-$Date: 2016-03-14 09:51:08 -0700 (Mon, 14 Mar 2016) $ TIBCO Software Inc.
 * Licensed under a BSD-style license. Refer to [LICENSE]
 * For more information, please contact:
 * TIBCO Software Inc., Palo Alto, California, USA
 *
 * $Id: JsonObject.java 84689 2016-03-14 16:51:08Z $
 */

using System.Collections.Generic;
using System.Text;

public sealed class JsonObject : Dictionary<string, object>
{

    public JsonObject() : base()
    {
    }

    public override string ToString()
    {
        StringBuilder sb = new StringBuilder();
        WriteJson(this, sb);
        return sb.ToString();
    }

    public static void WriteJson(JsonObject jsonObject, StringBuilder sb)
    {
        bool first = true;
        sb.Append('{');
        foreach (KeyValuePair<string, object> entry in jsonObject)
        {
            if (first)
            {
                first = false;
            }
            else
            {
                sb.Append(',');
            }
            JsonValue.WriteJson(entry.Key, sb);
            sb.Append(':');
            JsonValue.WriteJson(entry.Value, sb);
        }
        sb.Append('}');
    }
}
