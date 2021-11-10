/*
 * Copyright (c) 2013-2021 TIBCO Software Inc.
 * Licensed under a BSD-style license. Refer to [LICENSE]
 * For more information, please contact:
 * TIBCO Software Inc., Palo Alto, California, USA
 */

package com.tibco.eftl.samples;

import java.util.Properties;

import com.tibco.eftl.*;

/*
 * This is a sample of a basic eFTL reply program which
 * subscribes to request messages and sends a reply.
 */

public class Reply {
    
    static String url = "ws://localhost:8585/channel";
    static String username = null;
    static String password = null;
     
    public static void main(String[] args) {
     
        if (args.length > 0)
            url = args[0];
   
        System.out.printf("#\n# %s\n#\n# %s\n#\n",
                Reply.class.getName(), EFTL.getVersion());

        Properties props = new Properties();
    
        // set the connection properties
        if (username != null)
            props.setProperty(EFTL.PROPERTY_USERNAME, username);
        if (password != null)
            props.setProperty(EFTL.PROPERTY_PASSWORD, password);
            
        System.out.printf("Connecting to %s\n", url);

        // connect to the server
        EFTL.connect(url, props, new ConnectionListener() {

            @Override
            public void onConnect(Connection connection) {
                
                String matcher = "{\"type\":\"request\"}";
                
                // subscribe to request messages
                connection.subscribe(matcher, null, props, new SubscriptionListener() {

                    @Override
                    public void onMessages(Message[] requests) {
                        
                        for (Message request : requests) {

                            System.out.printf("Received request message %s\n", request);

                            try {
                                // create a reply message
                                Message reply = connection.createMessage();

                                reply.setString("text", "This is a sample eFTL reply message");

                                System.out.printf("Sending reply message %s\n", reply);

                                // send the reply message
                                connection.sendReply(reply, request, null);
                            } catch (Exception e) {
                                e.printStackTrace();
                            }
                        }

                        // sent the reply, disconnect and exit
                        connection.disconnect();
                    }

                    @Override
                    public void onSubscribe(String subscriptionId) {
                    }

                    @Override
                    public void onError(String subscriptionId, int code, String reason) {
                        
                        System.out.printf("Subscription error: %s\n", reason);

                        // received an error, disconnect and exit
                        connection.disconnect();
                    }
                });
            }

            @Override
            public void onDisconnect(Connection connection, int code, String reason) {
                
                System.out.printf("Disconnected: %s\n", reason);
            }

            @Override
            public void onError(Connection connection, int code, String reason) {
                
                System.out.printf("Error: %s\n", reason);
            }

            @Override
            public void onReconnect(Connection connection) {
                
                System.out.printf("Reconnected\n");
            }
        });
    }
}
