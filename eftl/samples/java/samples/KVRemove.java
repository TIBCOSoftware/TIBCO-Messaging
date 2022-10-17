/*
 * Copyright (c) 2013-2021 TIBCO Software Inc.
 * Licensed under a BSD-style license. Refer to [LICENSE]
 * For more information, please contact:
 * TIBCO Software Inc., Palo Alto, California, USA
 */

package com.tibco.eftl.samples;

import java.io.FileInputStream;
import java.io.InputStream;
import java.security.KeyStore;
import java.util.Properties;

import com.tibco.eftl.*;

/*
 * This is a sample of a basic eFTL program which removes
 * a key/value pair from a map.
 */

public class KVRemove extends Thread {

    String url = "ws://localhost:8585/map";
    String username = null;
    String password = null;
    String clientId = null;
    String trustStoreFilename = null;
    String trustStorePassword = "";
    boolean trustAll = false;
    String map = "sample_map";
    String key = "key1";

    public KVRemove(String[] args) {

        System.out.printf("#\n# %s\n#\n# %s\n#\n", this.getClass().getName(), EFTL.getVersion());

        parseArgs(args);
    }

    public void parseArgs(String[] args) {

        for (int i = 0; i < args.length; i++) {
            if (args[i].equalsIgnoreCase("--username") || args[i].equals("-u")) {
                if (i + 1 < args.length && !args[i + 1].startsWith("-")) {
                    username = args[++i];
                } else {
                    printUsage();
                }
            } else if (args[i].equalsIgnoreCase("--password") || args[i].equals("-p")) {
                if (i + 1 < args.length && !args[i + 1].startsWith("-")) {
                    password = args[++i];
                } else {
                    printUsage();
                }
            } else if (args[i].equalsIgnoreCase("--clientId") || args[i].equals("-i")) {
                if (i + 1 < args.length && !args[i + 1].startsWith("-")) {
                    clientId = args[++i];
                } else {
                    printUsage();
                }
            } else if (args[i].equalsIgnoreCase("--map") || args[i].equals("-m")) {
                if (i + 1 < args.length && !args[i + 1].startsWith("-")) {
                    map = args[++i];
                } else {
                    printUsage();
                }
            } else if (args[i].equalsIgnoreCase("--key") || args[i].equals("-k")) {
                if (i + 1 < args.length && !args[i + 1].startsWith("-")) {
                    key = args[++i];
                } else {
                    printUsage();
                }
            } else if (args[i].equalsIgnoreCase("--trustStore")) {
                if (i + 1 < args.length && !args[i + 1].startsWith("-")) {
                    trustStoreFilename = args[++i];
                } else {
                    printUsage();
                }
            } else if (args[i].equalsIgnoreCase("--trustStorePassword")) {
                if (i + 1 < args.length && !args[i + 1].startsWith("-")) {
                    trustStorePassword = args[++i];
                } else {
                    printUsage();
                }
            } else if (args[i].equalsIgnoreCase("--trustAll")) {
                trustAll = true;
            } else if (args[i].startsWith("-")) {
                printUsage();
            } else {
                url = args[i];
            }
        }
    }

    public void printUsage() {

        System.out.println();
        System.out.println("usage: MapRemove [options] url");
        System.out.println();
        System.out.println("options:");
        System.out.println("  -u, --username <username>");
        System.out.println("  -p, --password <password>");
        System.out.println("  -i, --clientId <client id>");
        System.out.println("  -m, --map <map>");
        System.out.println("  -k, --key <key>");
        System.out.println("      --trustStore <trust store filename>");
        System.out.println("      --trustStorePassword <trust store password");
        System.out.println("      --trustAll");
        System.out.println();
        System.exit(1);
    }

    private KeyStore loadTrustStore(String filename, String password) {

        // Load the specified TLS trust store from a file.
        if (filename != null) {
            try {
                final KeyStore trustStore = KeyStore.getInstance(KeyStore.getDefaultType());
                final InputStream in = new FileInputStream(filename);
                try {
                    trustStore.load(in, password.toCharArray());
                    return trustStore;
                } finally {
                    in.close();
                }
            } catch (Exception e) {
                e.printStackTrace(System.out);
            }
        }
        return null;
    }

    public void run() {

        final Properties props = new Properties();

        // Set the connection properties.
        if (username != null)
            props.setProperty(EFTL.PROPERTY_USERNAME, username);
        if (password != null)
            props.setProperty(EFTL.PROPERTY_PASSWORD, password);
        if (clientId != null)
            props.setProperty(EFTL.PROPERTY_CLIENT_ID, clientId);

        System.out.printf("Connecting to the eFTL server at %s\n", url);

        // Set the trust store if specified on the command line.
        EFTL.setSSLTrustStore(loadTrustStore(trustStoreFilename, trustStorePassword));

        // In a development-only environment there may be a need to 
        // skip server certificate authentication.
        EFTL.setSSLTrustAll(trustAll); 

        // Asynchronously connect to the eFTL server.
        EFTL.connect(url, props, new ConnectionListener() {

            @Override
            public void onConnect(Connection connection) {

                // Invoked when a connection to the eFTL server is successful.
                System.out.printf("Connected\n");

                // Remove the key/value pair from the map.
                connection.createKVMap(map).remove(key, new KVMapListener() {

                    @Override
                    public void onSuccess(String key, Message value) {

                        System.out.printf("Remove map='%s' key='%s'\n", map, key);

                        // Disconnect from the eFTL server.
                        connection.disconnect();
                    }

                    @Override
                    public void onError(String key, Message value, int code, String reason) {

                        System.out.printf("ERROR: Remove map='%s' key='%s': %s\n", map, key, reason);

                        // Disconnect from the eFTL server.
                        connection.disconnect();
                    }
                });
            }

            @Override
            public void onDisconnect(Connection connection, int code, String reason) {

                // Invoked when a connection to the eFTL server cannot be
                // established or is disconnected.
                if (code == ConnectionListener.NORMAL) {
                    // A normal disconnect indicates that
                    // Connection.disconnect() was invoked.
                    System.out.printf("Disconnected\n");
                    System.exit(0);
                } else {
                    // The connection could not be established
                    // or was lost.
                    System.out.printf("Connection error: %s\n", reason);
                    System.exit(1);
                }
            }

            @Override
            public void onError(Connection connection, int code, String reason) {

                // The eFTL server has sent an error to the client.
                System.out.printf("Error: %s\n", reason);
            }

            @Override
            public void onReconnect(Connection connection) {

                // The client has reconnected to the eFTL server.
                System.out.printf("Reconnected\n");
            }
        });
    }

    public static void main(String[] args) {

        try {
            new KVRemove(args).start();
        } catch (Throwable t) {
            t.printStackTrace(System.out);
        }
    }
}
