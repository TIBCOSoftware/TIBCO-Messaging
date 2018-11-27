package main

import (
	"fmt"
	"log"
	"tibco.com/ftl"
)

var realmServer string = "http://localhost:8080"
var appName string = "app-name"
var realm ftl.Realm
var pub ftl.Publisher
var msg ftl.Message

func main() {
	// print the banner
	log.Println("--------------------------------------------------------------")
	log.Println("TibSend")
	log.Println(ftl.Version())
	log.Println("--------------------------------------------------------------")
	send()
}

func send() int {
	// Connect to the realm server
	realm, err := ftl.Connect(realmServer, "default", nil)
	if err != nil {
		log.Fatal(ftl.ErrStack(err))
	}
	defer realm.Close()

	// Create the publisher object
	pub, err := realm.NewPublisher("default", nil)
	if err != nil {
		log.Fatal(ftl.ErrStack(err))
	}
	defer pub.Close()

	// create the hello world msg.
	msg, err := realm.NewMessage("")
	if err != nil {
		log.Fatal(ftl.ErrStack(err))
	}

	defer msg.Destroy()

	content := make(ftl.MsgContent)
	content["type"] = "hello"
	content["message"] = "hello world earth"
	msg.Set(content)

	// send hello world ftl msg
	fmt.Println("sending 'hello world' message\n")
	pub.Send(msg)

	// return
	return 0
}
