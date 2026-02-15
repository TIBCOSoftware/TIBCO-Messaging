//
// Copyright (c) 2001-2017 TIBCO Software Inc.
// Licensed under a BSD-style license. Refer to [LICENSE]
// For more information, please contact:
// TIBCO Software Inc., Palo Alto, California, USA
//

// This is an example of a basic eFTL client publisher.

package main

import (
	"log"
	"os"
	"time"

	"github.com/TIBCOSoftware/TIBCO-Messaging/eftl/sdk/golang/eftl"
)

var (
	url = "ws://localhost:8585/channel"
)

const (
	username = ""
	password = ""
	count    = 10
	rate     = 1
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

	// channel for receiving publish completions
	compChan := make(chan *eftl.Completion, 1000)

	// publish messages once per second
	ticker := time.NewTicker(time.Second / time.Duration(rate))

	var total int

	for {
		select {
		case now := <-ticker.C:
			total++
			// publish the message
			conn.PublishAsync(eftl.Message{
				"type": "hello",
				"text": "sample text",
				"time": now,
			}, compChan)
			if total >= count {
				ticker.Stop()
			}
		case comp := <-compChan:
			if comp.Error != nil {
				log.Printf("publish operation failed: %s", comp.Error)
				return
			}
			log.Printf("published message: %s", comp.Message)
			if total >= count {
				return
			}
		case err := <-errChan:
			log.Printf("connection error: %s", err)
			return
		}
	}
}
