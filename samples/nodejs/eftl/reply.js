/*
 * Copyright (c) 2013-$Date: 2019-10-16 10:49:36 -0700 (Wed, 16 Oct 2019) $ TIBCO Software Inc.
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
// to a server that has been configured for authentication.
//
// The onConnect function is invoked following a successful
// connection to the eFTL server.
//
// The onDisconnect function is invoked following a failed
// connection attempt to the eFTL server.
//
eFTL.connect(url, {
    username: 'user',
    password: 'password',
    onConnect: connection => {

        console.log("Connected");

        // Subscribe to request messages.
        connection.subscribe({
            matcher: `{"type":"request"}`,
            onMessage: request => {

                console.log('Received request ' + request.toString());

                try {
                    // create a reply message
                    var msg = connection.createMessage();
                    msg.set('text', 'This is a sample eFTL reply message');

                    // send the reply
                    connection.sendReply(msg, request);
                } catch (err) {
                    console.log(err);
                }

                connection.disconnect();
            },
            onSubscribe: id => {
            },
            onError: (id, code, reason) => {

                console.log('Subscription failed: ' + reason);

                connection.disconnect();
            }
        });
    },
    onDisconnect: (connection, code, reason) => {

        console.log(reason);
    },
    onError: (connection, code, reason) => {

        console.log(reason);
    }
});

