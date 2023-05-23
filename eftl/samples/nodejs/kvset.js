/*
 * Copyright (c) 2013-$Date: 2017-09-06 11:06:54 -0700 (Wed, 06 Sep 2017) $ Cloud Software Group, Inc.
 * Licensed under a BSD-style license. Refer to [LICENSE]
 */

'use strict'

const eFTL = require('eftl');
const fs = require('fs');

const url = (process.argv[2] || "ws://localhost:8585/map");

console.log(eFTL.getVersion());

// Set the server certificate chain for secure connections.
// Self-signed server certificates are not supported.
// When not set the eFTL client will trust any server certificate.
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
    trustAll: true,
    onConnect: connection => {
        console.log('Connected to eFTL server');
        setKeyValue(connection);
    },
    onDisconnect: (connection, code, reason) => {
        console.log('Disconnected from eFTL server: ' + reason);
    },
    onError: (connection, code, reason) => {
        console.log('Error from eFTL server: ' + reason);
    }
});

// Set a key-value pair.
// 
function setKeyValue(connection) {
    var msg = connection.createMessage();
    msg.set('text', 'This is sample eFTL text');
    msg.set('long', 101);
    msg.set('time', new Date());

    var map = connection.createKVMap('sample_map');
    map.set('key1', msg, {
        onSuccess: (key, value) => {
            console.log('Set: ' + key + '=' + value);
            connection.disconnect();
        },
        onError: (key, value, code, reason) => {
            console.log('Set error: ' + reason);
            connection.disconnect();
        }
    });
}
