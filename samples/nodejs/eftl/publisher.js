/*
 * Copyright (c) 2013-$Date: 2018-05-15 14:49:05 -0500 (Tue, 15 May 2018) $ TIBCO Software Inc.
 * Licensed under a BSD-style license. Refer to [LICENSE]
 * For more information, please contact:
 * TIBCO Software Inc., Palo Alto, California, USA
 */

'use strict'

const eFTL = require('eftl');
const fs = require('fs');

console.log(eFTL.getVersion());

var timer;
var counter = 0;

// Set the server certificate chain for secure connections.
// Self-signed server certificates are not supported.
// If not set the eFTL client will trust any server certificate.
//
//eFTL.setTrustCertificates(fs.readFileSync('./ca.pem'));

// Start a connection to the eFTL server.
//
// The username and password are required when connecting
// to an eFTL server that has been configured for JAAS.
//
// The onConnect function is invoked following a successful
// connection to the eFTL server. 
//
// The onDisconnect function is invoked following a failed
// connection attempt to the eFTL server.
//
eFTL.connect('ws://localhost:9191/channel', {
    username: undefined,
    password: undefined,
    onConnect: connection => {
        console.log('Connected to eFTL server');
        setInterval(publish, 1000, connection);
    },
    onDisconnect: (connection, code, reason) => {
        console.log('Error connecting to eFTL server: ' + reason);
        clearInterval(timer);
    }
});

// Publish an eFTL message. 
// 
function publish(connection) {
    var msg = connection.createMessage();
    msg.set('_dest', 'sample');
    msg.set('text', 'This is a sample eFTL message');
    msg.set('long', ++counter);
    msg.set('time', new Date());
    connection.publish(msg, {
        onComplete: msg => {
            console.log('Published: ' + msg.toString());
        },
        onError: (msg, code, reason) => {
            console.log('Publish error: ' + reason);
        }
    });
}
