//
// Copyright (c) 2001-$Date: 2020-05-27 09:56:45 -0700 (Wed, 27 May 2020) $ TIBCO Software Inc.
// All Rights Reserved. Confidential & Proprietary.
// For more information, please contact:
// TIBCO Software Inc., Palo Alto, California, USA
//

// This is an example of a basic eFTL client which gets a key-value pair from a map.

package main

import (
	"log"
	"os"

	"tibco.com/eftl"
)

var (
	url = "ws://localhost:8585/map"
)

const (
	username = ""
	password = ""
	mapName  = "sample_map"
	keyName  = "key1"
)

func main() {

	if len(os.Args) > 1 {
		url = os.Args[1]
	}

	// set connection options
	opts := eftl.DefaultOptions()
	opts.Username = username
	opts.Password = password

	// connect to the server
	conn, err := eftl.Connect(url, opts, nil)
	if err != nil {
		log.Printf("connect failed: %s\n", err)
		return
	}

	// close the connection when done
	defer conn.Disconnect()

	// get a key-value map
	kv := conn.KVMap(mapName)

	// get a key-value pair from the map
	if val, err := kv.Get(keyName); err != nil {
		log.Printf("key-value get failed: %s\n", err)
	} else if val == nil {
		log.Printf("key-value: not found\n")
	} else {
		log.Printf("key-value: %s\n", val)
	}
}
