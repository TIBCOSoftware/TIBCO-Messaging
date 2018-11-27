/*
 * Copyright (c) 2001-$Date: 2016-03-15 18:26:14 -0500 (Tue, 15 Mar 2016) $ TIBCO Software Inc.
 * Licensed under a BSD-style license. Refer to [LICENSE]
 * For more information, please contact:
 * TIBCO Software Inc., Palo Alto, California, USA
 *
 * $Id: JsonArray.java 84744 2016-03-15 23:26:14Z bpeterse $
 */
package com.tibco.eftl.json;

import java.io.IOException;
import java.util.ArrayList;

public final class JsonArray extends ArrayList<Object> {

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
    
    protected static void writeJson(JsonArray array, Appendable out) throws IOException {
        boolean first = true;
        out.append('[');
        for (Object obj : array) {
            if (first) {
                first = false;
            } else {
                out.append(',');
            }
            JsonValue.writeJson(obj, out);
        }
        out.append(']');
    }
}
