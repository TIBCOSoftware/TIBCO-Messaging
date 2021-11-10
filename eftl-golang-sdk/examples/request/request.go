//
// Copyright (c) 2001-2021 TIBCO Software Inc.
// All Rights Reserved. Confidential & Proprietary.
// For more information, please contact:
// TIBCO Software Inc., Palo Alto, California, USA
//

// This is an example of a basic eFTL client which publishes a request message
// and waits for a reply.

package main

import (
	"log"
	"os"
	"time"

	"tibco.com/eftl"
)

var (
	url = "ws://localhost:8585/channel"
)

const (
	username = ""
	password = ""
)

func main() {

	if len(os.Args) > 1 {
		url = os.Args[1]
	}
        log.Printf("%s : TIBCO eFTL Version: %s\n", os.Args[0], eftl.Version)

	// channel for receiving connection errors
	errChan := make(chan error, 1)

	// set connection options
	opts := eftl.DefaultOptions()
	opts.Username = username
	opts.Password = password

	// connect to the server
	conn, err := eftl.Connect(url, opts, errChan)
	if err != nil {
		log.Printf("connect failed: %s", err)
		return
	}

	// close the connection when done
	defer conn.Disconnect()

	// create a request message
	request := eftl.Message{}
	request["type"] = "request"
	request["text"] = "this is a sample request message"

	// send the request message and receive a reply
	reply, err := conn.SendRequest(request, 10*time.Second)
	if err != nil {
		log.Printf("request error: %s\n", err)
	} else {
		log.Printf("reply message: %s\n", reply)
	}
}
