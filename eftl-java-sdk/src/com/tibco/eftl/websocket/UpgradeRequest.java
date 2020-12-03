/*
 * Copyright (c) 2013-$Date: 2020-05-05 14:34:09 -0700 (Tue, 05 May 2020) $ TIBCO Software Inc.
 * Licensed under a BSD-style license. Refer to [LICENSE]
 * For more information, please contact:
 * TIBCO Software Inc., Palo Alto, California, USA
 *
 * $Id: UpgradeRequest.java 124749 2020-05-05 21:34:09Z bpeterse $
 *
 */
package com.tibco.eftl.websocket;

import java.io.IOException;
import java.io.OutputStream;
import java.net.URI;
import java.nio.charset.Charset;
import java.security.SecureRandom;
import java.util.List;

public class UpgradeRequest {

    private final URI uri;
    private final String[] protocols;
    private final String key;
    
    private static final SecureRandom random = new SecureRandom();
    
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
