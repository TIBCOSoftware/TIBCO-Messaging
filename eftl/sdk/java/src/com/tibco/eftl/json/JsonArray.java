/*
 * Copyright (c) 2001-$Date$ TIBCO Software Inc.
 * Licensed under a BSD-style license. Refer to [LICENSE]
 * For more information, please contact:
 * TIBCO Software Inc., Palo Alto, California, USA
 *
 * $Id$
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
            return "";
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
