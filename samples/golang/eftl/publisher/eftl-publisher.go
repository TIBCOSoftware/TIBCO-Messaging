// Copyright © 2018. TIBCO Software Inc.

package main

// Golang Imports
import (
	"flag"
	"fmt"
	"log"
	"os"
	"path/filepath"
	"time"

	"github.com/TIBCOSoftware/TIBCO-Messaging/eftl-golang-sdk"
	"os/signal"
)

//eftl-publisher global variable list
var (
	serverUrl 	= "ws://localhost:9191/channel"
	username	= "user"
	password	= "pass"
)

// parseArgs parses the commandline arguments using the golang flag operations
func parseArgs() error {
	// Usage output function to display eftl-publisher.go
	flag.Usage = func() {
		usage := filepath.Base(os.Args[0]) + " [options]\n\tA sample TIBCO eFTL publisher"
		fmt.Printf("\nUsage: %s\n\n", usage)
		fmt.Println("Options:")
		flag.PrintDefaults()
	}

	flag.StringVar(&username, "u", username, "Username used when connecting to TIBCO eFTL")
	flag.StringVar(&password, "p", password, "Password used when connecting to TIBCO eFTL")
	flag.StringVar(&serverUrl, "s", serverUrl, "TIBCO eFTL server URL for publisher to connect to ")

	flag.Parse()

	return nil
}

// eftl-publisher.go main function that publishes data via eFTL
func main() {

	// Print Application Banner that includes the eFTL version
	fmt.Println(" ")
	fmt.Println("Starting " + filepath.Base(os.Args[0]) + ":")
	fmt.Println("   Sample eFTL publisher application")
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
		log.Printf("[eFTL Info] - Recieved os.Interrupt Exiting \n")
		os.Exit(1)
	}()

	// Set eFTL connection options for eFTL connection
	opts := &eftl.Options{
		Username: username,
		Password: password,
	}

	// Create error channel for receiving eFTL connection errors
	errChan := make(chan error, 1)

	// Connect to TIBCO eFTL calling eftl.Connect if err does not equal nil log err and exit
	connection, err := eftl.Connect(serverUrl, opts, errChan)
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
			log.Println("[eFTL Error] - TIBCO eFTL connection error:", err)

			// Connection Error encountered try to reconnect
			connection.Reconnect()
		}
	}()

	// Create a message and populate its contents.
	//
	// Message fields can be of type string, long, double, date, and message, along with arrays
	// of each of the types.
	//
	// Subscribers will create matchers that match on one or more of the message fields. Only
	// string and long fields are supported by matchers.
	//

	messageCounter := 0

	for {

		messageCounter++

		message := eftl.Message{
			"type": "SAMPLE",
			"text": "This is an sample message",
			"long": int64(messageCounter),
			"time": time.Now(),
		}

		// Publish the message via TIBCO eFTL if error log and return
		err = connection.Publish(message)
		if err != nil {
			log.Println("[eFTL Error] - TIBCO eFTL publish failure:", err)
			return
		}

		log.Printf("[eFTL Info] - Published Message %d: %s\n", messageCounter, message)

		// Sleep 1000 milliseconds between gathering each quote
		time.Sleep(1000 * time.Millisecond)
	}
}