//
// Copyright (c) 2001-2021 TIBCO Software Inc.
// All Rights Reserved. Confidential & Proprietary.
// For more information, please contact:
// TIBCO Software Inc., Palo Alto, California, USA
//

// This is an example of a basic eFTL client which removes a key-value pair from a map.

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
        log.Printf("%s : TIBCO eFTL Version: %s\n", os.Args[0], eftl.Version)

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

	// remove a key-value pair from the map
	if err := kv.Remove(keyName); err != nil {
		log.Printf("key-value remove failed: %s\n", err)
	} else {
		log.Printf("key-value removed\n")
	}
}
