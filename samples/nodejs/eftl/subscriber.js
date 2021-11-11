/*
 * Copyright (c) 2013-$Date: 2020-09-24 12:20:18 -0700 (Thu, 24 Sep 2020) $ TIBCO Software Inc.
 * Licensed under a BSD-style license. Refer to [LICENSE]
 * For more information, please contact:
 * TIBCO Software Inc., Palo Alto, California, USA
 */

'use strict'

const eFTL = require('eftl');
const fs = require('fs');

const url = (process.argv[2] || "ws://localhost:8585/channel");

console.log(eFTL.getVersion());

// Set the server certificate chain for secure connections.
// Self-signed server certificates are not supported.
//
//eFTL.setTrustCertificates(fs.readFileSync('./ca.pem'));

// Start a connection to the eFTL server.
//
// The username and password are required when connecting
// to an eFTL server that has been configured for JAAS.
//
// The clientId uniquely defines the eFTL client. If not
// provided the eFTL server will generate one for the client.
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
    trustAll: false,
    clientId: 'sample-node-client',
    onConnect: connection => {
        console.log('Connected to eFTL server');
        subscribe(connection);
    },
    onDisconnect: (connection, code, reason) => {
        console.log('Disconnected from eFTL server: ' + reason);
    },
    onError: (connection, code, reason) => {
        console.log('Error from eFTL server: ' + reason);
    }
});

// Subscribe to eFTL messages using a durable subscription.
//
// This subscription defines a matcher that will match published 
// messages containing a field named "type" with a value of "hello".
//
// The durable name is used to create a durable subscription. When
// setting a durable name also set the clientId in the connect
// method as durable subscriptions are mapped to specific clientIds.
//
// The subscription's message acknowledgment mode can be set to
// 'auto', 'client', or 'none'. When set to 'client' the consumed
// messages must be explicitly acknowledged. The default message
// acknowledgment mode is 'auto'.
//
// The onSubscribe function is invoked following a successful
// subscription.
//
// The onError function is invoked following a failed subscription.
//
// the onMessage function is invoked whenever a published message
// is received by the subscription.
//
function subscribe(connection) {
    connection.subscribe({
        matcher: `{"type":"hello"}`,
        durable: 'sample-durable',
        ack: 'client',
        onSubscribe: id => {
            console.log('Subscribed');

            // Unsubscribe and disconnect after 30 seconds.
            setTimeout(function() {
                // This will permanently remove a durable subscription.
                connection.unsubscribe(id);
                connection.disconnect();
            }, 30000);
        },
        onError: (id, code, reason) => {
            console.log('Subscription failed: ' + reason);
        },
        onMessage: message => {
            console.log('Received message: ' + message.toString());
        }
    });
}
