/*
 * Copyright (c) 2013-$Date: 2014-05-15 12:37:50 -0500 (Thu, 15 May 2014) $ TIBCO Software Inc.
 * Licensed under a BSD-style license. Refer to [LICENSE]
 * For more information, please contact:
 * TIBCO Software Inc., Palo Alto, California, USA
 *
 * $Id: UpgradeRequest.java 73712 2014-05-15 17:37:50Z bpeterse $
 *
 */
package com.tibco.eftl.websocket;

import java.io.IOException;
import java.io.OutputStream;
import java.net.URI;
import java.nio.charset.Charset;
import java.util.List;
import java.util.Random;

public class UpgradeRequest {

    private final URI uri;
    private final String[] protocols;
    private final String key;
    
    private static final Random random = new Random();
    
    private static String generateKey() {
        byte[] bytes = new byte[16];
        random.nextBytes(bytes);
        return Base64.encode(bytes);
    }
    
    public UpgradeRequest(URI uri, List<String> protocols) {
        this.uri = uri;
        this.protocols = protocols.toArray(new String[0]);
        this.key = generateKey();
    }
    
    public String getKey() {
        return key;
    }
    
    public void write(OutputStream stream) throws IOException {
        stream.write(toString().getBytes(Charset.forName("UTF-8")));
        stream.flush();
    }
    
    @Override
    public String toString() {
        StringBuilder request = new StringBuilder(256);

        request.append("GET ");
        if (uri.getPath() == null ||
            uri.getPath().isEmpty()) {
            request.append("/");
        } else {
            request.append(uri.getPath());
        }
        request.append(" HTTP/1.1\r\n");

        request.append("Host: ").append(uri.getHost());
        if (uri.getPort() > 0) {
            request.append(":").append(uri.getPort());
        }
        request.append("\r\n");

        request.append("Upgrade: websocket\r\n");
        request.append("Connection: Upgrade\r\n");
        request.append("Sec-WebSocket-Key: ").append(getKey()).append("\r\n");
        request.append("Sec-WebSocket-Version: 13\r\n");

        for (String protocol : protocols) {
            request.append("Sec-WebSocket-Protocol: ").append(protocol).append("\r\n");
        }

        request.append("\r\n");

        return request.toString();
    }
}
