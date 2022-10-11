/*
 * Copyright (c) 2001-$Date: 2016-03-11 16:29:10 -0800 (Fri, 11 Mar 2016) $ TIBCO Software Inc.
 * Licensed under a BSD-style license. Refer to [LICENSE]
 * For more information, please contact:
 * TIBCO Software Inc., Palo Alto, California, USA
 *
 * $Id: JsonValue.java 84680 2016-03-12 00:29:10Z $
 */

using System;
using System.Text;

public class JsonValue
{
    public static object Parse(string text)
    {
        return new JsonParser(text).Parse();
    }

    public static void WriteJson(object value, StringBuilder sb)
    {
        if (value == null)
        {
            sb.Append("null");
        }
        else if (value is string)
        {
            WriteJsonString((string)value, sb);
        }
        else if (value is double)
        {
            if (Double.IsInfinity((double)value) || Double.IsNaN((double)value))
            {
                sb.Append("null");
            }
            else
            {
                sb.Append(value.ToString());
            }
        }
        else if (value is float)
        {
            if (Single.IsInfinity((float)value) || Single.IsNaN((float)value))
            {
                sb.Append("null");
            }
            else
            {
                sb.Append(value.ToString());
            }
        }
        else if (value is JsonObject)
        {
            JsonObject.WriteJson((JsonObject)value, sb);
        }
        else if (value is JsonArray)
        {
            JsonArray.WriteJson((JsonArray)value, sb);
        }
        else
        {
            sb.Append(value.ToString());
        }
    }

    private static void WriteJsonString(string value, StringBuilder sb)
    {
        sb.Append('"');
        for (int i = 0, len = value.Length; i < len; i++)
        {
            char c = value[i];
            switch (c)
            {
                case '"':
                    sb.Append("\\\"");
                    break;
                case '\\':
                    sb.Append("\\\\");
                    break;
                case '\t':
                    sb.Append("\\t");
                    break;
                case '\b':
                    sb.Append("\\b");
                    break;
                case '\n':
                    sb.Append("\\n");
                    break;
                case '\r':
                    sb.Append("\\r");
                    break;
                case '\f':
                    sb.Append("\\f");
                    break;
                case '\u2028':
                case '\u2029':
                    sb.Append(String.Format("\\u%04x", (int)c));
                    break;
                default:
                    if (c <= 0x1F)
                    {
                        sb.Append(String.Format("\\u%04x", (int)c));
                    }
                    else
                    {
                        sb.Append(c);
                    }
                    break;
            }
        }
        sb.Append('"');
    }
}
