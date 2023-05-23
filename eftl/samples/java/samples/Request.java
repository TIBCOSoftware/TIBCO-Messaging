/*
 * Copyright (c) 2013-2020 Cloud Software Group, Inc.
 * Licensed under a BSD-style license. Refer to [LICENSE]
 */

package com.tibco.eftl.samples;

import java.util.Properties;

import com.tibco.eftl.*;

/*
 * This is a sample of a basic eFTL request program which
 * publishes a request message and waits for a reply.
 */

public class Request {
    
    static String url = "ws://localhost:8585/channel";
    static String username = null;
    static String password = null;
    
    public static void main(String[] args) {
 
        if (args.length > 0)
            url = args[0];
           
        System.out.printf("#\n# %s\n#\n# %s\n#\n",
                Request.class.getName(), EFTL.getVersion());

        Properties props = new Properties();
    
        // set the connection properties
        if (username != null)
            props.setProperty(EFTL.PROPERTY_USERNAME, username);
        if (password != null)
            props.setProperty(EFTL.PROPERTY_PASSWORD, password);
            
        System.out.printf("Connecting to %s\n", url);
        
        // connect.
        EFTL.connect(url, props, new ConnectionListener() {

            @Override
            public void onConnect(Connection connection) {
         
                try {       
                    // create a request message
                    Message request = connection.createMessage();

                    request.setString("type", "request");
                    request.setString("text", "This is a sample eFTL request message");

                    System.out.printf("Sending request message %s\n", request);

                    // send the request message with a 10 second timeout
                    connection.sendRequest(request, 10.0, new RequestListener() {

                        @Override
                        public void onReply(Message reply) {
                            
                            System.out.printf("Received reply message %s\n", reply);
                         
                            // received the reply, disconnet and exit
                            connection.disconnect();
                        }

                        @Override
                        public void onError(Message message, int code, String reason) {
                            
                            System.out.printf("Request error: %s\n", reason);
                            
                            // received an error, disconnect and exit
                            connection.disconnect();
                        }
                    });
                } catch (Exception e) {
                    e.printStackTrace();
                }
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
