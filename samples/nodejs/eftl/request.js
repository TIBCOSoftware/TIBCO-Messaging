/*
 * Copyright (c) 2013-$Date: 2019-06-21 12:47:57 -0700 (Fri, 21 Jun 2019) $ TIBCO Software Inc.
 * Licensed under a BSD-style license. Refer to [LICENSE]
 * For more information, please contact:
 * TIBCO Software Inc., Palo Alto, California, USA
 */

'use strict'

const eFTL = require('eftl');

const url = (process.argv[2] || "ws://localhost:8585/channel");

console.log(eFTL.getVersion());

// Start a connection to the server.
//
// The username and password are required when connecting
// to a server that has been configured for JAAS.
//
// The onConnect function is invoked following a successful
// connection to the server. 
//
// The onDisconnect function is invoked following a failed
// connection attempt to the server.
//
eFTL.connect(url, {
    username: 'user',
    password: 'password',
    trustAll: true,
    onConnect: connection => {

        console.log("Connected");

        try {
            // create a request message
            var msg = connection.createMessage();
            msg.set('type', 'request');
            msg.set('text', 'This is a sample eFTL request message');

            // send a request message and wait 10 seconds for a reply
            connection.sendRequest(msg, 10000, {

                onReply: reply => {

                    console.log('Received reply: ' + reply.toString());

                    connection.disconnect();
                },

                onError: (request, code, reason) => {

                    console.log('Request error: ' + reason);

                    connection.disconnect();
                }
            });
        } catch (err) {
            console.log(err);

            connection.disconnect();
        }
    },
    onDisconnect: (connection, code, reason) => {

        console.log(reason);
    },
    onError: (connection, code, reason) => {

        console.log(reason);
    }
});
