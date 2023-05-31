/*
 * Copyright (c) 2013-$Date$ Cloud Software Group, Inc.
 * Licensed under a BSD-style license. Refer to [LICENSE]
 *
 * $Id$
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
    private final String username;
    private final String password;
    private final String clientId;
    
    private static final SecureRandom random = new SecureRandom();
    
    private static String generateKey() {
        byte[] bytes = new byte[16];
        random.nextBytes(bytes);
        return Base64.encode(bytes);
    }
    
    public UpgradeRequest(URI uri, List<String> protocols, String username, String password, String clientId) {
        this.uri = uri;
        this.protocols = protocols.toArray(new String[0]);
        this.key = generateKey();
        this.username = username;
        this.password = password;
        this.clientId = clientId;
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
        if (clientId != null &&
            !clientId.isEmpty()) {
            request.append("?client_id=").append(clientId);
        }
        request.append(" HTTP/1.1\r\n");

        request.append("Host: ").append(uri.getHost());
        if (uri.getPort() > 0) {
            request.append(":").append(uri.getPort());
        }
        request.append("\r\n");

        request.append("Upgrade: websocket\r\n");
        request.append("Connection: Upgrade\r\n");

        if ((username != null && !username.isEmpty()) ||
            (password != null && !password.isEmpty())) {
            StringBuilder auth = new StringBuilder(256);
            auth.append(username != null ? username : "");
            auth.append(":");
            auth.append(password != null ? password : "");

            String auth64 = Base64.encode(auth.toString().getBytes());
            request.append("Authorization: Basic ").append(auth64).append("\r\n");
        }

        request.append("Sec-WebSocket-Key: ").append(getKey()).append("\r\n");
        request.append("Sec-WebSocket-Version: 13\r\n");

        for (String protocol : protocols) {
            request.append("Sec-WebSocket-Protocol: ").append(protocol).append("\r\n");
        }

        request.append("\r\n");

        return request.toString();
    }
}
