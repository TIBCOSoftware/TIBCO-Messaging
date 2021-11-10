/*
 * Copyright (c) 2013-$Date: 2020-05-05 14:34:09 -0700 (Tue, 05 May 2020) $ TIBCO Software Inc.
 * All rights reserved.
 * For more information, please contact:
 * TIBCO Software Inc., Palo Alto, California, USA
 *
 * $Id: UpgradeResponse.java 124749 2020-05-05 21:34:09Z bpeterse $
 *
 */
package com.tibco.eftl.websocket;

import java.io.BufferedReader;
import java.io.IOException;
import java.io.InputStream;
import java.io.InputStreamReader;
import java.security.MessageDigest;
import java.security.NoSuchAlgorithmException;
import java.util.HashMap;
import java.util.List;
import java.util.Map;

public class UpgradeResponse {

    private final String version;
    private final int statusCode;
    private final String statusReason;
    private final Map<String, String> headers;

    public UpgradeResponse(String version, int statusCode, String statusReason)
    {
        this.version = version;
        this.statusCode = statusCode;
        this.statusReason = statusReason;
        this.headers = new HashMap<String, String>();
    }

    public static UpgradeResponse read(InputStream stream) throws IOException {
        UpgradeResponse response = null;

        BufferedReader reader = new BufferedReader(new InputStreamReader(stream));

        for (String line; (line = reader.readLine()) != null;) {
            if (line.isEmpty())
                break;
            if (response == null) {
                String[] status = line.split(" ", 3);
                response = new UpgradeResponse(status[0], Integer.parseInt(status[1]), status[2]);
            } else {
                String[] header = line.split(": ", 2);
                response.setHeader(header[0], header[1]);
            }
        }

        if (response == null) {
            throw new IOException("no HTTP response");
        }

        return response;
    }

    public String getVersion() {
        return version;
    }

    public int getStatusCode() {
        return statusCode;
    }

    public String getStatusReason() {
        return statusReason;
    }

    public String getHeader(String name) {
        for (String key : headers.keySet()) {
            if (key.equalsIgnoreCase(name)) {
                return headers.get(key);
            }
        }
        return null;
    }

    public void setHeader(String name, String value) {
        headers.put(name, value);
    }

    public String getProtocol() {
        return getHeader("Sec-WebSocket-Protocol");
    }
    
    public void validate(UpgradeRequest request, List<String> protocols) throws UpgradeException {
        // check for 101 - Switching Protocols
        int statusCode = getStatusCode();
        if (statusCode != 101) {
            throw new UpgradeException(statusCode, getStatusReason());
        }

        // Connection header must be "upgrade"
        String connection = getHeader("Connection");
        if (!"upgrade".equalsIgnoreCase(connection)) {
            throw new UpgradeException(statusCode,
                    "expected Connection header value of [upgrade], received [" + connection + "]");
        }

        // Upgrade header must be "websocket"
        String upgrade = getHeader("Upgrade");
        if (!"websocket".equalsIgnoreCase(upgrade)) {
            throw new UpgradeException(statusCode,
                    "expected Upgrade header value of [websocket], received [" + upgrade + "]");
        }

        // check for properly hashed request key
        String key = request.getKey();
        String keyHash = getHeader("Sec-WebSocket-Accept");
        String expectedKeyHash = generateKeyHash(key);
        if (!expectedKeyHash.equalsIgnoreCase(keyHash)) {
            throw new UpgradeException(statusCode,
                    "invalid Sec-WebSocket-Accept value");
        }
        
        // check for properly negotiated protocol
        if (protocols.size() > 0) {
            String protocol = getHeader("Sec-WebSocket-Protocol");
            if (protocol == null || !protocols.contains(protocol)) {
            throw new UpgradeException(statusCode,
                    "invalid Sec-WebSocket-Protocol value");
            }
        }
    }

    private static String generateKeyHash(String key) {
        try {
            // websocket protocol requires a SHA-1 hash of the key
            MessageDigest md = MessageDigest.getInstance("SHA-1"); //NOSONAR
            md.update((key + "258EAFA5-E914-47DA-95CA-C5AB0DC85B11").getBytes());
            return Base64.encode(md.digest()).trim();
        } catch (NoSuchAlgorithmException e) {
            throw new RuntimeException(e);
        }
    }

    @Override
    public String toString() {
        StringBuilder string = new StringBuilder(256);

        string.append(version).append(" ");
        string.append(statusCode).append(" ");
        string.append(statusReason).append("\r\n");

        for (String key : headers.keySet()) {
            string.append(key).append(": ");
            string.append(getHeader(key)).append("\r\n");
        }

        string.append("\r\n");

        return string.toString();
    }
}
