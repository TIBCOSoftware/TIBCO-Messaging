/*
 * Copyright (c) 2013-2020 Cloud Software Group, Inc.
 * Licensed under a BSD-style license. Refer to [LICENSE]
 */

'use strict'

const eFTL = require('eftl');
const fs = require('fs');

const url = (process.argv[2] || "ws://localhost:8585/channel");

console.log(eFTL.getVersion());

var timer;
var counter = 0;

// Set the server certificate chain for secure connections.
// Self-signed server certificates are not supported.
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
eFTL.connect(url, {
    username: 'user',
    password: 'password',
    trustAll: false,
    onConnect: connection => {
        console.log('Connected to eFTL server');
        timer = setInterval(publish, 1000, connection);
    },
    onDisconnect: (connection, code, reason) => {
        console.log('Disconnected from eFTL server: ' + reason);
        clearInterval(timer);
    },
    onError: (connection, code, reason) => {
        console.log('Error from eFTL server: ' + reason);
    }
});

// Publish an eFTL message. 
// 
function publish(connection) {
    var msg = connection.createMessage();
    msg.set('type', 'hello');
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
