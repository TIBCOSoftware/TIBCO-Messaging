//
// Copyright (c) 2001-2021 TIBCO Software Inc.
// All Rights Reserved. Confidential & Proprietary.
// For more information, please contact:
// TIBCO Software Inc., Palo Alto, California, USA
//

// This is an example of a basic eFTL client which sets a key-value pair in a map.

package main

import (
	"log"
	"os"
	"time"

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

	// create a messsage
	val := eftl.Message{
		"text": "sample text",
		"time": time.Now(),
	}

	// set the key-value pair in the map
	if err = kv.Set(keyName, val); err != nil {
		log.Printf("key-value set failed: %s\n", err)
	} else {
		log.Printf("key-value set: %s\n", val)
	}
}
