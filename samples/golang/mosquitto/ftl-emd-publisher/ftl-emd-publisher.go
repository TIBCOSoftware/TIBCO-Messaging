package main

import (
	"flag"
	"fmt"
	"log"
	"os"
	"path/filepath"
	"sync"
	"time"

	"tibco.com/ftl"
	"strconv"
)

// Varible list used by ftl-emd-publisher
var (
	realmUser          		string        = "guest"
	realmPassword      		string        = "guest-pw"
	realmHost           		string        = "http://localhost"
	realmPort          		string        = "8080"
	realmServerAddress 		string        = realmHost + ":" + realmPort
	realmCertificateFile    string        = ""
	realmTrustAll      		bool          = false

	ftlAppName      		string 		  = "tibfmbridge"
	ftlEndpointName 		string 		  = "tibfmbridge_client_endpoint"
	// ftlClientLabel and ftlFormatName currently cannot be changed on command-line
	ftlClientLabel  		string 		  = "ftl-emd-publisher"
	ftlFormatName   	    string 		  = ""

	mqttTopic         		string        = "test"
	mqttRetainedFlag        bool          = false
	mqttQos		            int           = 0

	messagePayload	        string        = "ftl2mqtt message "
	messageCount            int           = 1
	messageBatch            int           = 1
	messageNumber			int			  = 1

	sendInterval	        time.Duration = 1 * time.Second

	// Varible Initialization list
	msg         		ftl.Message
	msgBlock    		ftl.MsgBlock
	realm       		ftl.Realm
	timer       		*time.Ticker

	messagesSent        int  			  = 0
	doneSending 		bool 			  = false

	traceLevel 			string 			  = ""
)

// parseArgs parses the commandline arguments using the golang flag operations
func parseArgs() error {

	// Usage output function to display ftl-emd-publisher
	flag.Usage = func() {
		usage := filepath.Base(os.Args[0]) + " [options]\n\tA sample FTL publisher that publishes to a MQTT subscriber"
		fmt.Printf("\nUsage: %s\n\n", usage)
		fmt.Println("Options:")
		flag.PrintDefaults()
	}

	flag.StringVar(&realmUser, "u", realmUser, "Username")
	flag.StringVar(&realmUser, "user", realmUser, "Username")

	flag.StringVar(&realmPassword, "pw", realmPassword, "Password")
	flag.StringVar(&realmPassword, "password", realmPassword, "Password")

	flag.StringVar(&realmHost, "h", realmHost, "Realmserver host")
	flag.StringVar(&realmHost, "host", realmHost, "Realmserver host")

	flag.StringVar(&realmPort, "p", realmPort, "Realmserver port")
	flag.StringVar(&realmPort, "port", realmPort, "Realmserver port")

	flag.IntVar(&messageCount, "c", messageCount,
		"Total number of messages to send. 0 means endless.")
	flag.IntVar(&messageCount, "count", messageCount,
		"Total number of messages to send. 0 means endless.")

	flag.StringVar(&mqttTopic, "t", mqttTopic, "Set MQTT topic")
	flag.StringVar(&mqttTopic, "topic", mqttTopic, "Set MQTT topic")

	flag.StringVar(&realmCertificateFile, "cafile", realmCertificateFile, "Realmserver certificate file")

	flag.BoolVar(&realmTrustAll, "trustall", realmTrustAll, "Realmserver Trust all flag")

	flag.StringVar(&messagePayload, "m", messagePayload, "Set MQTT message")
	flag.StringVar(&messagePayload, "message", messagePayload, "Set MQTT message")

	flag.BoolVar(&mqttRetainedFlag, "r", mqttRetainedFlag, "Set MQTT retained flag")
	flag.BoolVar(&mqttRetainedFlag, "retained", mqttRetainedFlag, "Set MQTT retained flag")

	flag.IntVar(&mqttQos, "q", mqttQos, "Set MQTT qos flag")
	flag.IntVar(&mqttQos, "qos", mqttQos, "Set MQTT qos flag")

	flag.IntVar(&messageBatch, "batch", messageBatch, "Messages per interval")
	flag.IntVar(&messageBatch, "b", messageBatch, "Messages per interval")

	flag.DurationVar(&sendInterval, "i", sendInterval, "Interval between send operations")
	flag.DurationVar(&sendInterval, "interval", sendInterval, "Interval between send operations")

	flag.StringVar(&ftlAppName, "application", ftlAppName, "FTL Application Name")
	flag.StringVar(&ftlAppName, "a", ftlAppName, "FTL Application Name")

	flag.StringVar(&ftlEndpointName, "endpoint", ftlEndpointName, "Endpoint Name")
	flag.StringVar(&ftlEndpointName, "e", ftlEndpointName, "Endpoint Name")

	flag.StringVar(&traceLevel, "trace", traceLevel, "where <level> can be: off, severe, warn, info, debug, verbose")

	flag.Parse()

	realmServerAddress = realmHost + ":" + realmPort

	return nil
}

// ftl-emd-publisher main function
func main() {

	fmt.Println(" ")
	fmt.Println("Starting " + filepath.Base(os.Args[0]) + ":")
	fmt.Println("   Sample FTL (FTL->MQTT) Publisher Application")

	fmt.Println("   Parsing Command Line Arguments")
	fmt.Println(" ")

	// Call parseArgs to parse command line arguments
	parseArgs()

	// Set FTL log level if traceLevel is not empty
	if traceLevel != "" {
		ftl.SetLogLevel(traceLevel)
	}

	// Call send function to connect to FTL realm server, create the publisher and send messages
	send()

	fmt.Println(" ")
	fmt.Println("Stopping " + filepath.Base(os.Args[0]) + ":")
	fmt.Println("   Sent " + strconv.Itoa(messageNumber-1) +
		" messages in " +  strconv.Itoa(messageNumber/messageBatch) +
		" batches, " + strconv.Itoa(messageBatch) + " messages per batch")
	fmt.Println(" ")
}

// ftl-emd-publisher initMessage function walks the msgBlock object clearing messages and setting the
// appropriate mqtt fields.  This is call each time a back of messages is sent for higher performance
// this should be pre-allocated and only the fields that need to be changed should be touched.
func initMessage() {

	// Walk the msgBlock object, clear and set each messages contents
	for _, msg := range msgBlock {
		// Clear the message
		msg.Clear()

		// Create the content object as an ftl.MsgContent object
		content := make(ftl.MsgContent)

		// Set the _mqtt_topic field to mqttTopic
		content["_mqtt_topic"] = mqttTopic

		// Set the _mqtt_ret to 0
		content["_mqtt_ret"] = int64(0)

		// Check if mqttRetainedFlag is set, if it is set _mqtt_ret field to 1
		if mqttRetainedFlag {
			content["_mqtt_ret"] = int64(1)
		}

		// Set the _mqtt_qos field to mqttQos
		content["_mqtt_qos"] = mqttQos

		// Set the _mqtt_pay field to messagePayload + messageNumber
		content["_mqtt_pay"] = messagePayload + strconv.Itoa(messageNumber)

		// Increment messageNumber
		messageNumber++

		// Set the message to content
		msg.Set(content)
	}
}


// ftl-emd-publisher send function does the actual work of setting up realm server and sending messages
func send() int {

	// Set up properties for the realm server connection
	realmProps := make(ftl.Props)

	// Check if realmUser has been set, if it has been set realmProps
	// Username and Userpassword to realmUser and realmPassword
	if realmUser != "" {
		realmProps[ftl.RealmPropertyStringUsername] = realmUser
		realmProps[ftl.RealmPropertyStringUserpassword] = realmPassword
	}

	// Check if the realmCertificateFile is set, if it has been set realmProps LongTrustType to UseSpecifiedTrustFile
	// and set StringTrustFile equal to the passed realmCertificateFile.  If realmCertificateFile has not been set
	// check if realmTrustAll was set and set LongTrustType to ConnectionTrustEveryone if it has been set.
	if realmCertificateFile != "" {
		realmProps[ftl.RealmPropertyLongTrustType] = ftl.RealmHTTPSConnectionUseSpecifiedTrustFile
		realmProps[ftl.RealmPropertyStringTrustFile] = realmCertificateFile
	} else if realmTrustAll {
		realmProps[ftl.RealmPropertyLongTrustType] = ftl.RealmHTTPSConnectionTrustEveryone
	}

	// Check if ftlClientLabel is empty, if not set realmProps StringClientLabel to ftlClientLabel
	if ftlClientLabel != "" {
		realmProps[ftl.RealmPropertyStringClientLabel] = ftlClientLabel
	}

	// Connect to the FTL realm server via realmServerAddress passing the ftlAppName and the realmProps set above.
	realm, err := ftl.Connect(realmServerAddress, ftlAppName, realmProps)

	// If ftl.connect fails call log.Fatal with err
	if err != nil {
		log.Fatal(ftl.ErrStack(err))
	}

	// FTL realm object was created successfully  defer Close() operation
	defer realm.Close()

	// Create a FTL MsgBlock object to store the message objects to send
	msgBlock = make(ftl.MsgBlock, 0)

	// Populate the msgBlock object with new messages.  Append the msg to the msgBlock based on the messageBatch size
	for i := 0; i < messageBatch; i++ {
		msg, err = realm.NewMessage(ftlFormatName)

		// If NewMessage fails log call log.Fatal with err
		if err != nil {
			log.Fatal(ftl.ErrStack(err))
		}

		// Append the newly created message to msgBlock
		msgBlock = append(msgBlock, msg)
	}


	// Create a FTL publisher using ftlEndpointName, publisher properties is set to nil
	ftlPublisher, err := realm.NewPublisher(ftlEndpointName, nil)

	// If NewPublisher fails call log.Fatal with err
	if err != nil {
		log.Fatal(ftl.ErrStack(err))
	}

	// FTL publisher object was created successfully defer Close() operation
	defer ftlPublisher.Close()


	// Create a new timer object that uses teh sendInterval as the Ticker
	timer := time.NewTicker(sendInterval)

	// Create a new waitGroup object to track go func operation and only exit once complete.
	// Set waitGroup.Add delta to 1 to track only a single go func
	waitGroup := &sync.WaitGroup{}
	waitGroup.Add(1)

	// Run goroutine for send operation
	go func() {
		// Loop until doneSending and messageSent equals messageCount
		for {
			// Select operation on the time ticker channel, fires every tick (based on sendInterval)
			select {

			case <-timer.C:

				// Check to see if we have sent the correct number of messages and are done sending
				if (messageCount > 0 && messagesSent >= messageCount) || doneSending {
					waitGroup.Done()
					return
				}

				// Log the planned attempt to send the batch of messages for this interval.
				log.Printf("Sending %d messages every %s", messageBatch, sendInterval)

				// Call initMessage to initialize the dynamic message format used and
				// populate the correct fields for mqtt
				initMessage()

				switch {

				// Check that messageBatch is not less than 1 if it is call log.Fatal
				case messageBatch < 1:
					log.Fatal("Message batch is required to be greater than 0, " +
						"please set batch option to be greater than 0")
					break

				// If messageBatch equals 1, send the first msg in msgBlock using ftlPublisher.Send()
				case messageBatch == 1:

					// Increment messagesSent
					messagesSent++

					// Call ftlPublisher.Send passing msgBlock sub 0
					err = ftlPublisher.Send(msgBlock[0])

					// If err is not nil print the error message but continue
					if err != nil {
						log.Printf("ftlPublisher.Send(msgBlock[0]) failed: ", err)
					}

					break

				// If messageBatch greater than 1, send the whole msgBlock using ftlPublisher.SendMessages()
				default:
					// Increment messagesSent by the number of messages in the messageBatch
					messagesSent += messageBatch

					// Call ftlPublisher.Send passing the whole msgBlock
					err = ftlPublisher.SendMessages(msgBlock)

					// If err is not nil print the error message but continue
					if err != nil {
						log.Printf("ftlPublisher.SendMessages(msgBlock) failed: ", err)
					}
					break
				}
			}
		}
	}()

	// wait long enough for send to be done
	waitGroup.Wait()

	// Clean up
	msg.Destroy()

	return 0
}
