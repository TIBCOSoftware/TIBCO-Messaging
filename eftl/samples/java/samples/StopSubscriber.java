/*
 * Copyright (c) 2013-2020 Cloud Software Group, Inc.
 * Licensed under a BSD-style license. Refer to [LICENSE]
 */

package com.tibco.eftl.samples;

import java.io.FileInputStream;
import java.io.InputStream;
import java.security.KeyStore;
import java.util.Properties;
import java.util.Timer;
import java.util.TimerTask;

import com.tibco.eftl.*;

/*
 * This is a sample of a basic eFTL subscriber program.
 */

public class StopSubscriber extends Thread {
    
    String url = "ws://localhost:8585/channel";
    String username = null;
    String password = null;
    String clientId = "sample-java-client";
    String durableName = "sample-durable";
    String trustStoreFilename = null;
    String trustStorePassword = "";
    String subId = null;
    int received = 0;
    boolean trustAll = false;

    boolean clientAcknowledge = false;

    public StopSubscriber(String[] args) {
        
         System.out.printf("#\n# %s\n#\n# %s\n#\n",
                 this.getClass().getName(),
                 EFTL.getVersion());

         parseArgs(args);
    }
     
    public void parseArgs(String[] args) {
        
        for (int i = 0; i < args.length; i++) {
            if (args[i].equalsIgnoreCase("--username") || args[i].equals("-u")) {
                if (i+1 < args.length && !args[i+1].startsWith("-")) {
                    username = args[++i];
                } else {
                    printUsage();
                }
            } else if (args[i].equalsIgnoreCase("--password") || args[i].equals("-p")) {
                if (i+1 < args.length && !args[i+1].startsWith("-")) {
                    password = args[++i];
                } else {
                    printUsage();
                }
            } else if (args[i].equalsIgnoreCase("--clientId") || args[i].equals("-i")) {
                if (i+1 < args.length && !args[i+1].startsWith("-")) {
                    clientId = args[++i];
                } else {
                    printUsage();
                }
            } else if (args[i].equalsIgnoreCase("--durableName") || args[i].equals("-n")) {
                if (i+1 < args.length && !args[i+1].startsWith("-")) {
                    durableName = args[++i];
                } else {
                    printUsage();
                }
            } else if (args[i].equalsIgnoreCase("--trustStore")) {
                if (i+1 < args.length && !args[i+1].startsWith("-")) {
                    trustStoreFilename = args[++i];
                } else {
                    printUsage();
                }
            } else if (args[i].equalsIgnoreCase("--trustStorePassword")) {
                if (i+1 < args.length && !args[i+1].startsWith("-")) {
                    trustStorePassword = args[++i];
                } else {
                    printUsage();
                }
            } else if (args[i].equalsIgnoreCase("--trustAll")) {
                trustAll = true;
            } else if (args[i].equalsIgnoreCase("--clientAcknowledge")) { 
                clientAcknowledge = true;
            } else if (args[i].startsWith("-")) {
                printUsage();
            } else {
                url = args[i];
            }
        }
    }
    
    public void printUsage() {
        
        System.out.println();
        System.out.println("usage: Subscriber [options] url");
        System.out.println();
        System.out.println("options:");
        System.out.println("  -u, --username <username>");
        System.out.println("  -p, --password <password>");
        System.out.println("  -i, --clientId <client id>");
        System.out.println("  -n, --durableName <durable name>");
        System.out.println("      --trustStore <trust store filename>");
        System.out.println("      --trustStorePassword <trust store password");
        System.out.println("      --trustAll");
        System.out.println("      --clientAcknowledge");
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
        EFTL.setSSLTrustStore(loadTrustStore(trustStoreFilename, 
                                             trustStorePassword));

        // In a development-only environment there may be a need to 
        // skip server certificate authentication.
        EFTL.setSSLTrustAll(trustAll); 

        // Asynchronously connect to the eFTL server.
        EFTL.connect(url, props, new ConnectionListener() {

            @Override
            public void onConnect(Connection connection) {
                
                // Invoked when a connection to the eFTL server is successful.
                System.out.printf("Connected\n");
                // Subscribe to messages
                new Thread(new Runnable() {

                    @Override
                    public void run() {
                        subscribe(connection);
                    }
                }).start();
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
    
    public void subscribe(final Connection connection) {
        
        final Properties props = new Properties();

        // Set the subscription properties.
        //
        // Configure the subscription for client acknowledgments if requested.
        //
        if (clientAcknowledge) {
            props.setProperty(EFTL.PROPERTY_ACKNOWLEDGE_MODE, EFTL.ACKNOWLEDGE_MODE_CLIENT);
        }

        // Create a subscription matcher for messages containing a
        // field named "type" with the value "hello".
        //
        // When connected to an FTL channel the content matcher
        // can be used to match any field in a published message.
        // Only matching messages will be received by the
        // subscription. The content matcher can match on string
        // and integer fields, or test for the presence or absence
        // of a field by setting it's value to the boolean true or
        // false.
        //
        // When connected to an EMS channel the content matcher
        // must only contain the destination field "_dest" set to
        // the EMS topic on which to subscribe.
        //
        // To match all messages use the empty matcher "{}".
        //
        // The durable name is used to create a durable subscription.
        // When setting a durable name also set the client identifier
        // when connecting as standard durable subscriptions are 
        // mapped to a specific client identifier.
        // 
        final String matcher = "{\"type\":\"hello\"}";
        
        System.out.printf("Subscribing to %s\n", matcher);
        
        // Asynchronously subscribe to messages with a content matcher.
        connection.subscribe(matcher, durableName, props, new SubscriptionListener() {

            @Override
            public void onMessages(Message[] messages) {
                
                // Invoked when matching messages are received.
                for (Message message : messages) {
                    System.out.printf("Received message %s\n", message);

                    if (clientAcknowledge) {
                        connection.acknowledge(message);
                    }

                    received++;
                    if (received == 5) {
                        System.out.printf("Stopping subscription\n");
                        try{
                            connection.stopSubscription(subId);
                        } catch(Exception e) {
                            System.out.println(e);
                            return;
                        }
                        
                        try {
                            sleep(3000);
                        } catch(Exception e) {
                            System.out.println(e);
                        }

                        System.out.printf("Starting subscription\n");
                        try {
                            connection.startSubscription(subId);
                            return;
                        } catch(Exception e) {
                            System.out.println(e);
                        }
                    }
                }
            }

            @Override
            public void onSubscribe(String subscriptionId) {
                subId = subscriptionId;
                
                // The eFTL server has created the subscription.
                System.out.printf("Subscribed\n");

                // Unsubscribe and disconnect after 30 seconds.
                new Timer(true).schedule(new TimerTask() {
                    @Override
                    public void run() {
                        // This will permanently remove a durable subscription.
                        connection.unsubscribe(subscriptionId);

                        // Disconnect from the eFTL server.
                        connection.disconnect();
                    }
                }, 30000);
            }

            @Override
            public void onError(String subscriptionId, int code, String reason) {
                
                // The eFTL server was unable to create the subscription.
                System.out.printf("Subscription error: %s\n", reason);
                // Disconnect from the eFTL server.
                connection.disconnect();
            }
        });
    }

    public static void main(String[] args) {
        
        try {
            new StopSubscriber(args).start();
        } catch (Throwable t) {
            t.printStackTrace(System.out);
        }
    }
}
