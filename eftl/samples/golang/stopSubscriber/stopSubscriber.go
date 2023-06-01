//
// Copyright (c) 2001-$Date$ Cloud Software Group, Inc.
// Licensed under a BSD-style license. Refer to [LICENSE]
//

// This is an example of a basic eFTL client subscriber.

package main

import (
    "time"
	"log"
	"os"

	"tibco.com/eftl"
)

var (
	url = "ws://localhost:8585/channel"
)

const (
	username  = ""
	password  = ""
	clientID  = "sample-go-client"
	durable   = "sample-durable"
	clientAck = false
	count     = 10
)

func main() {

	if len(os.Args) > 1 {
		url = os.Args[1]
	}
        log.Printf("%s : TIBCO eFTL Version: %s\n", os.Args[0], eftl.Version)

	// set the log level log file
        eftl.SetLogLevel(eftl.LogLevelDebug)
        eftl.SetLogFile("subscriber.log")

	// channel for receiving connection errors
	errChan := make(chan error, 1)

	// set connection options
	opts := eftl.DefaultOptions()
	opts.ClientID = clientID
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
	subopts := eftl.SubscriptionOptions{}

	if clientAck {
		subopts.AcknowledgeMode = eftl.AcknowledgeModeClient
	}

	// create the subscription
	//
	// always set the client identifier when creating a durable subscription
	// as durable subscriptions are mapped to the client identifier
	//
	err = conn.SubscribeWithOptionsAsync(matcher, durable, subopts, msgChan, subChan)
	if err != nil {
		log.Printf("subscribe failed: %s", err)
		return
	}

	var total int
    var mySub *eftl.Subscription

	for {
		select {
		case sub := <-subChan:
			if sub.Error != nil {
				log.Printf("subscribe operation failed: %s", sub.Error)
				return
			}
			log.Printf("subscribed with matcher: %s", sub.Matcher)
			// unsubscribe, this will permanently remove a durable subscription
			mySub = sub
			defer conn.Unsubscribe(sub)
		case msg := <-msgChan:
			total++
			log.Printf("received message: %s", msg)

			if clientAck {
				conn.Acknowledge(msg)
			}

			if total == 5 {
				log.Printf("Stopping subscription")
				err = conn.StopSubscription(mySub)
				if err != nil {
					log.Printf("stop failed %s", err)
				} else {
					time.Sleep(3 * time.Second)
					log.Printf("Starting subscription")
					err = conn.StartSubscription(mySub)
					if err != nil {
						log.Printf("start failed %s", err)
					}
				}
			}

			if total >= count {
				return
			}
		case err := <-errChan:
			log.Printf("connection error: %s", err)
			return
		}
	}
}
