//
// Copyright (c) 2001-2021 Cloud Software Group, Inc.
// Licensed under a BSD-style license. Refer to [LICENSE]
//

// This is an example of a basic eFTL client which subscribes to request messages
// and sends a reply.

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

	// channel for receiving request messages
	msgChan := make(chan eftl.Message, 1000)

	// create the message content matcher
	matcher := `{"type":"request"}`

	// create the subscription options
	subopts := eftl.SubscriptionOptions{}

	// create the subscription
	err = conn.SubscribeWithOptionsAsync(matcher, "", subopts, msgChan, subChan)
	if err != nil {
		log.Printf("subscribe failed: %s", err)
		return
	}

	for {
		select {
		case sub := <-subChan:

			if sub.Error != nil {
				log.Printf("subscribe failed: %s", sub.Error)
				return
			}

			log.Printf("subscribed with matcher %s", sub.Matcher)

		case msg := <-msgChan:

			log.Printf("request message: %s", msg)

			// send a reply message
			conn.SendReply(eftl.Message{"text": "this is a sample reply message"}, msg)

			return

		case err := <-errChan:

			log.Printf("connection error: %s", err)

			return
		}
	}
}
