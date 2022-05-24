//
// Copyright (c) 2001-2021 TIBCO Software Inc.
// Licensed under a BSD-style license. Refer to [LICENSE]
// For more information, please contact:
// TIBCO Software Inc., Palo Alto, California, USA
//

// This is an example of a basic eFTL client which uses a shared durable subscription
// to receives published messages.

package main

import (
	"log"
	"os"

	"tibco.com/eftl"
)

var (
	url = "ws://localhost:8585/channel"
)

const (
	username = ""
	password = ""
	durable  = "sample-shared-durable"
	count    = 10
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

	// channel for receiving subscription response
	subChan := make(chan *eftl.Subscription, 1)

	// channel for receiving published messages
	msgChan := make(chan eftl.Message, 1000)

	// create the message content matcher
	matcher := "{\"type\":\"hello\"}"

	// create the subscription options
	subOpts := eftl.SubscriptionOptions{
		DurableType: "shared",
	}

	// create the subscription
	err = conn.SubscribeWithOptionsAsync(matcher, durable, subOpts, msgChan, subChan)
	if err != nil {
		log.Printf("subscribe failed: %s", err)
		return
	}

	var total int

	for {
		select {
		case sub := <-subChan:
			if sub.Error != nil {
				log.Printf("subscribe operation failed: %s", sub.Error)
				return
			}
			log.Printf("subscribed with matcher %s", sub.Matcher)
		case msg := <-msgChan:
			total++
			log.Printf("received message: %s", msg)
			if total >= count {
				return
			}
		case err := <-errChan:
			log.Printf("connection error: %s", err)
			return
		}
	}
}
