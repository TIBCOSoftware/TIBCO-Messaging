/*
 * Copyright (c) 2001-$Date: 2016-03-14 14:37:06 -0700 (Mon, 14 Mar 2016) $ TIBCO Software Inc.
 * Licensed under a BSD-style license. Refer to [LICENSE]
 * For more information, please contact:
 * TIBCO Software Inc., Palo Alto, California, USA
 *
 * $Id: JsonValue.java 84701 2016-03-14 21:37:06Z $
 */
package com.tibco.eftl.json;

import java.io.IOException;

public class JsonValue {

    public static Object parse(String text) {
        return new JsonParser(text).parse();
    }
    
    protected static void writeJson(Object value, Appendable out) throws IOException {
        if (value == null) {
            out.append("null");
        } else if (value instanceof String) {
            writeJsonString((String) value, out);
        } else if (value instanceof Double) {
            if (Double.isInfinite((Double) value) || Double.isNaN((Double) value)) {
                out.append("null");
            } else {
                out.append(value.toString());
            }
        } else if (value instanceof Float) {
            if (Float.isInfinite((Float) value) || Float.isNaN((Float) value)) {
                out.append("null");
            } else {
                out.append(value.toString());
            }
        } else if (value instanceof JsonArray) {
            JsonArray.writeJson((JsonArray) value, out);
        } else if (value instanceof JsonObject) {
            JsonObject.writeJson((JsonObject) value, out);
        } else {
            out.append(value.toString());
        }
    }
    
    private static void writeJsonString(String value, Appendable out) throws IOException {
        out.append('"');
        for (int i = 0, len = value.length(); i < len; i++) {
            char c = value.charAt(i);
            switch (c) {
            case '"':
                out.append("\\\"");
                break;
            case '\\':
                out.append("\\\\");
                break;
            case '\t':
                out.append("\\t");
                break;
            case '\b':
                out.append("\\b");
                break;
            case '\n':
                out.append("\\n");
                break;
            case '\r':
                out.append("\\r");
                break;
            case '\f':
                out.append("\\f");
                break;
            case '\u2028':
            case '\u2029':
                out.append(String.format("\\u%04x", (int) c));
                break;
            default:
                if (c <= 0x1F) {
                    out.append(String.format("\\u%04x", (int) c));
                } else {
                    out.append(c);
                }
                break;            
            }
        }
        out.append('"');
    }
}
