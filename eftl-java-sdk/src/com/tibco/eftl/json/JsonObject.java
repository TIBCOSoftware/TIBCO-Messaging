/*
 * Copyright (c) 2001-$Date: 2020-03-31 10:23:10 -0700 (Tue, 31 Mar 2020) $ TIBCO Software Inc.
 * Licensed under a BSD-style license. Refer to [LICENSE]
 * For more information, please contact:
 * TIBCO Software Inc., Palo Alto, California, USA
 *
 * $Id: JsonObject.java 123342 2020-03-31 17:23:10Z bpeterse $
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
            return "";
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
