/*
 * Copyright (c) 2001-$Date: 2016-03-15 18:42:55 -0500 (Tue, 15 Mar 2016) $ TIBCO Software Inc.
 * Licensed under a BSD-style license. Refer to [LICENSE]
 * For more information, please contact:
 * TIBCO Software Inc., Palo Alto, California, USA
 *
 * $Id: JsonObject.java 84746 2016-03-15 23:42:55Z bpeterse $
 */
package com.tibco.eftl.json;

import java.io.IOException;
import java.util.Map;
import java.util.HashMap;

public final class JsonObject extends HashMap<String, Object> {

    private static final long serialVersionUID = 1L;

    @Override
    public String toString() {
        try {
            StringBuilder sb = new StringBuilder();
            writeJson(this, sb);
            return sb.toString();
        } catch (IOException e) {
            // can't happen with StringBuilder
            return null;
        }
    }
    
    protected static void writeJson(JsonObject object, Appendable out) throws IOException {
        boolean first = true;
        out.append('{');
        for (Map.Entry<String, Object> entry : object.entrySet()) {
            if (first) {
                first = false;
            } else {
                out.append(',');
            }
            JsonValue.writeJson(entry.getKey(), out);
            out.append(':');
            JsonValue.writeJson(entry.getValue(), out);
        }
        out.append('}');
    }
}
