package main

import (
	"fmt"
	"log"
	"tibco.com/ftl"
)

var (
	realm       ftl.Realm
	realmServer string = "http://localhost:8080"
	sub         ftl.Subscriber
	queue       ftl.Queue
	done        bool = false
	matcher     string
)

func recv() int {
	// Connect to the realm server
	realm, err := ftl.Connect(realmServer, "default", nil)
	if err != nil {
		log.Fatal(ftl.ErrStack(err))
	}

	// Create a queue and add subscriber to it
	errorchan := make(chan error)
	go func() {
		for problem := range errorchan {
			log.Fatal(ftl.ErrStack(problem))
		}
	}()
	queue, _ = realm.NewQueue(errorchan, nil)

	msgPipe := make(chan ftl.Message)
	matcher := "{\"type\":\"hello\"}"

	sub, err = queue.Subscribe("default", matcher, nil, msgPipe)
	if err != nil {
		log.Fatal(ftl.ErrStack(err))
	}

	// Receive expected messages
	fmt.Println("Waiting for message(s)\n")
	for msg := range msgPipe {
		messagesReceived(msg)
		if done {
			break
		}
	}

	// Clean up
	queue.CloseSubscription(sub)
	queue.Destroy()
	sub.Close()
	realm.Close()
	return 0
}

func messagesReceived(msg ftl.Message) {
	m := msg
	log.Printf("Received message: %s\n", m.String())
	done = true
	return
}

func main() {
	// print the banner
	log.Println("--------------------------------------------------------------")
	log.Println("TibRecv")
	log.Println(ftl.Version())
	log.Println("--------------------------------------------------------------")
	recv()
}
