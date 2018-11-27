// Copyright © 2018. TIBCO Software Inc.

package main

// Golang Imports
import (
	"flag"
	"fmt"
	"log"
	"os"
	"os/signal"
	"path/filepath"
	"time"

	"github.com/TIBCOSoftware/TIBCO-Messaging/eftl-golang-sdk"
)

// eftl-subscriber global variable list
var (
	serverUrl 	= "ws://localhost:9191/channel"
	username	= "user"
	password	= "pass"
	clientID 	= "client-go"
)

// eftl-subscriber message structure populated from the eFTL message.
type subscriberMessage struct {
	MessageType		string
	MessageText		string
	MessageLong		int64
	MessageTime		time.Time
}

// parseArgs parses the commandline arguments using the golang flag operations
func parseArgs() error {
	// Usage output function to display eftl-subscriber.go
	flag.Usage = func() {
		usage := filepath.Base(os.Args[0]) + " [options]\n\tA sample TIBCO eFTL subscriber"
		fmt.Printf("\nUsage: %s\n\n", usage)
		fmt.Println("Options:")
		flag.PrintDefaults()
	}

	flag.StringVar(&username, "c", clientID, "Client used when connecting to TIBCO eFTL")
	flag.StringVar(&username, "u", username, "Username used when connecting to TIBCO eFTL")
	flag.StringVar(&password, "p", password, "Password used when connecting to TIBCO eFTL")
	flag.StringVar(&serverUrl, "s", serverUrl, "TIBCO eFTL server URL for publisher to connect to ")

	flag.Parse()

	return nil
}

// eftl-subscriber.go main function that drives subscription from eFTL
func main() {

	// Print Application Banner that includes the eFTL version
	fmt.Println(" ")
	fmt.Println("Starting " + filepath.Base(os.Args[0]) + ":")
	fmt.Println("   Sample eFTL subscriber application")
	fmt.Println("   eFTL version ", eftl.Version)
	fmt.Println("")

	fmt.Println("   Parsing Command Line Arguments")
	fmt.Println(" ")

	parseArgs()

	// setup shutdown channel that captures os.interrupt and exits gracefully
	shutdownChannel := make(chan os.Signal, 1)

	signal.Notify(shutdownChannel, os.Interrupt)
	go func(){
		<-shutdownChannel
		log.Printf("[TCM Info] - Recieved os.Interrupt Exiting \n")
		os.Exit(1)
	}()

	// Set eFTL connection options for eFTL connection
	opts := &eftl.Options{
		Username:  username,
		Password:  password,
		ClientID:  clientID,
	}

	// Create error channel for receiving eFTL connection errors
	errChan := make(chan error, 1)

	// Connect to TIBCO eFTL calling eftl.Connect if err does not equal nil log err and exit
	connection, err := eftl.Connect( serverUrl, opts, errChan)
	if err != nil {
		log.Println("[eFTL Error] - Failure to Connect to TIBCO eFTL server: ", err)
		return
	}

	// Connection succesful, defer connection Disconnect to disconnect from eFTL at application close
	defer connection.Disconnect()

	log.Println("[eFTL Info] - Connect to TIBCO eFTL server: ", serverUrl)

	// Listen for eFTL connection errors.
	go func() {
		for err := range errChan {
			log.Println("[TCM Error] - TIBCO eFTL connection error:", err)

			// Connection Error encountered try to reconnect
			connection.Reconnect()
		}
	}()

	// Create message channel for recieving eFTL messages
	messageChannel := make(chan eftl.Message, 100)


	// Create a subscription to an eFTL server
	//
	// This subscription receives only those messages whose fields match the subscription's matcher
	// string. A subscription can only match on string and long fields.
	//
	// This subscription matches messages containing a string field with name "type" and value "SAMPLE".
	//
	// No durable name is being used for this subscription, as the default setup is non-persistent.

	subscriber, err := connection.Subscribe("{\"type\":\"SAMPLE\"}", "", messageChannel)
	if err != nil {
		log.Println("[TCM Error] - TIBCO eFTL subscribe failed: ", err)
	}

	// unsubscribe from messages when done
	defer connection.Unsubscribe(subscriber)

	receivedMessage := subscriberMessage{}

	// Listen for messages on the messageChannel
	for message := range messageChannel {

		// Populate a local message structure with the information from the message.
		for messageFieldKey, messageFieldValue := range message {
			switch messageFieldKey {
			case "type":
				receivedMessage.MessageType = messageFieldValue.(string)
			case "text":
				receivedMessage.MessageText = messageFieldValue.(string)
			case "long":
				receivedMessage.MessageLong = messageFieldValue.(int64)
			case "time":
				receivedMessage.MessageTime = messageFieldValue.(time.Time)
			}
		}

		// Print the message information
		log.Printf("[eFTL Subscriber Received Message]: ")
		log.Printf("\tMsg Time[%s]; Msg Type: %s, Msg Number: %d, Msg Text: %s",
			receivedMessage.MessageTime, receivedMessage.MessageType, receivedMessage.MessageLong,
			receivedMessage.MessageText)
	}
}
