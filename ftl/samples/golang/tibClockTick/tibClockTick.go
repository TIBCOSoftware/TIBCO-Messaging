package main

import (
	"fmt"
	"log"
	"time"
	"flag"
	"tibco.com/ftl"
)

var realmServer string = "http://localhost:8080"
var appName string = "app-name"
var realm ftl.Realm
var pub ftl.Publisher
var msg ftl.Message

func main() {

	// Command Line Arguments
	tickPtr := flag.Int("t", 100, "Number of clock ticks to send")
        intervalPtr := flag.Int("i", 2, "Interval, in seconds, between tick sends")
	
	flag.Parse()

	// print the banner
	log.Println("--------------------------------------------------------------")
	log.Println("TibClockTick")
	log.Println(ftl.Version())
	log.Println("--------------------------------------------------------------")
	clockTick(*tickPtr, *intervalPtr)
}

func clockTick(tick int, interval int) int {
	// Connect to the realm server
	realm, err := ftl.Connect(realmServer, "tibsend", nil)
	if err != nil {
		log.Fatal(ftl.ErrStack(err))
	}
	defer realm.Close()

	// Create the publisher object
	pub, err := realm.NewPublisher("tibsend-endpoint", nil)
	if err != nil {
		log.Fatal(ftl.ErrStack(err))
	}
	defer pub.Close()

	// create the clock tick message.
	msg, err := realm.NewMessage("")
	if err != nil {
		log.Fatal(ftl.ErrStack(err))
	}

	defer msg.Destroy()

	counter := 0

	for counter < tick {

		content := make(ftl.MsgContent)
		content["type"] = "Date"
		t := time.Now()
		content["message"] = t.Format(time.UnixDate)


		msg.Set(content)

		// send message 
		fmt.Println(counter, ": Sending Message: ", t.Format(time.UnixDate))
		pub.Send(msg)
		time.Sleep(time.Duration(interval)*time.Second) 
		counter = counter + 1
	}

	// return
	return 0
}
