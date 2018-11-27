package main

import (
	"crypto/tls"
	"crypto/x509"
	"flag"
	"fmt"
	"io/ioutil"
	"log"
	"net/http"
	"os"
	"path/filepath"
	"strconv"
	"sync"
	"time"

	_ "net/http/pprof"

	mqtt "github.com/eclipse/paho.mqtt.golang"
)

// Varible list used by mqtt-emd-subscriber
var (
	mqttUser          		string          = ""
	mqttPassword      		string          = ""
	mqttHost          		string          = "http://localhost"
	mqttPort        	    string          = "1883"
	mqttBrokerAddress 		string          = mqttHost + ":" + mqttPort
	mqttTopic         		string          = "test"

	messageCount         	int             = 1

	brokerCertificateFile   string          = ""

	clientCount   			int             = 1
	clientGroup   			*sync.WaitGroup = &sync.WaitGroup{}
	readyGroup    			*sync.WaitGroup = &sync.WaitGroup{}

	pprof         			string          = ""
	outputQuiet         	bool            = false
)

// parseArgs parses the commandline arguments using the golang flag operations
func parseArgs() error {

	// Usage output function to display mqtt-emd-subscriber
	flag.Usage = func() {
		usage := filepath.Base(os.Args[0]) + " [options]\n\tA sample MQTT subscriber that subscribes to a FTL publisher"
		fmt.Printf("\nUsage: %s\n\n", usage)
		fmt.Println("Options:")
		flag.PrintDefaults()
	}

	flag.StringVar(&mqttUser, "u", mqttUser, "MQTT Username")
	flag.StringVar(&mqttUser, "user", mqttUser, "MQTT Username")

	flag.StringVar(&mqttPassword, "pw", mqttPassword, "MQTT Password")
	flag.StringVar(&mqttPassword, "password", mqttPassword, "MQTT Password")

	flag.StringVar(&mqttHost, "h", mqttHost, "MQTT Broker host")
	flag.StringVar(&mqttHost, "host", mqttHost, "MQTT Broker host")

	flag.StringVar(&mqttPort, "p", mqttPort, "MQTT Broker port")
	flag.StringVar(&mqttPort, "port", mqttPort, "MQTT Broker port")

	flag.IntVar(&messageCount, "c", messageCount, "Total number of messages to receive. " +
		"0 means endless.")
	flag.IntVar(&messageCount, "count", messageCount, "Total number of messages to receive. " +
		"0 means endless.")

	flag.StringVar(&mqttTopic, "t", mqttTopic, "Set MQTT topic")
	flag.StringVar(&mqttTopic, "topic", mqttTopic, "Set MQTT topic")

	flag.IntVar(&clientCount, "C", clientCount, "Number of MQTT clients to create")
	flag.IntVar(&clientCount, "clients", clientCount, "Number of MQTT clients to create")

	flag.StringVar(&brokerCertificateFile, "cafile", brokerCertificateFile,
		"MQTT Broker certificate file")

	flag.BoolVar(&outputQuiet, "q", outputQuiet,
		"Only print the number of messages received when exiting.")
	flag.BoolVar(&outputQuiet, "quiet", outputQuiet,
		"Only print the number of messages received when exiting.")

	flag.Parse()

	pprof = os.Getenv("TIB_BKR_PROFILING")

	mqttBrokerAddress = mqttHost + ":" + mqttPort

	return nil
}

// mqtt-emd-subscriber main function
func main() {

	fmt.Println(" ")
	fmt.Println("Starting " + filepath.Base(os.Args[0]) + ":")
	fmt.Println("   Sample MQTT (FTL->MQTT) Subscriber Application")

	fmt.Println("   Parsing Command Line Arguments")
	fmt.Println(" ")

	parseArgs()

	// Create the seperate client groups and spin a thread to have the client receive call receive
	for i := 0; i < clientCount; i++ {
		// Add the clientGroup
		clientGroup.Add(1)

		// Add the readyGroup
		readyGroup.Add(1)

		// Call receive
		go receive(i)
	}

	// Wait for all the readyGroups to return before running pprof function
	readyGroup.Wait()

	// If pprof is not empty run pprof
	if pprof != "" {
		go func() {
			http.ListenAndServe(pprof, nil)
		}()
	}

	// Wait for all clientGroups to return before exiting
	clientGroup.Wait()

	fmt.Println(" ")
	fmt.Println("Stopping " + filepath.Base(os.Args[0]) + ":")
	fmt.Println("   Received " + strconv.Itoa(messageNumber-1) + " messages")
	fmt.Println(" ")
}

// mqtt-emd-subscriber receive function does the actual work of setting up realm server and sending messages
func receive(id int) {
	// defer clientGroup.Done until receive function returns
	defer clientGroup.Done()

	// Set clientId to the application name - group id - pid
	clientId := "mqtt-emd-subscriber-" + strconv.Itoa(id) + "-" + strconv.Itoa(os.Getpid())

	// Create a new clientOptions struct setting BrokerAddress, ClientId, MaxReconnectInterval, Username and Password,
	// also disable AutoReconnect
	opts := mqtt.NewClientOptions()
	opts.AddBroker(mqttBrokerAddress)
	opts.SetAutoReconnect(false)
	opts.SetClientID(clientId)
	opts.SetMaxReconnectInterval(1 * time.Minute)
	opts.SetUsername(mqttUser)
	opts.SetPassword(mqttPassword)

	// Check if the brokerCertificateFile is set, if it has been set, create the tlsConfig and tlsCerts pool reading
	// in the brokerCertificateFile and appending the pemCert from the brokerCertificatefile and setting the
	// tlsConfig.RootCAs to the tlsCerts and then set the TLSConfig in the ClientOptions structure.
	if brokerCertificateFile != "" {
		tlsConfig := &tls.Config{}
		tlsCerts := x509.NewCertPool()
		if pemCert, err := ioutil.ReadFile(brokerCertificateFile); err != nil {
			log.Fatalf("failed to read trust file: %s\n", err)
		} else {
			tlsCerts.AppendCertsFromPEM(pemCert)
			tlsConfig.RootCAs = tlsCerts
		}
		opts.SetTLSConfig(tlsConfig)
	}

	// Create the new MQTT client passing the set ClientOptions structure
	mqttClient := mqtt.NewClient(opts)

	// Call connect, wait and validate there was no error on connecting to the MQTT Broker
	mqttDeliverytoken := mqttClient.Connect()
	mqttDeliverytoken.Wait()
	if mqttDeliverytoken.Error() != nil {
		log.Fatalf("[Error] - Failed to connect to MQTT broker: %s\n", mqttDeliverytoken.Error())
	}


	// Create a new waitGroup object to track go func operation and only exit once complete.
	// Set waitGroup.Add delta to 1 to track only a single go func
	waitGroup := &sync.WaitGroup{}
	waitGroup.Add(1)

	// Set received to 0 to start
	received := 0

	// Set subscribeTopic to mqttTopic
	subscribeTopic := mqttTopic

	// If multiple clients are defined and mqttTopic does not equal # set subscribeTopic to mqttTopic/id
	if id > 0 && mqttTopic != "#" {
		subscribeTopic = mqttTopic + "/" + strconv.Itoa(id)
	}

	// Call Subscribe passing subscribeTopic, qos 0 and the callback function to process the received message
	mqttDeliverytoken = mqttClient.Subscribe(subscribeTopic, 0, func(client mqtt.Client, message mqtt.Message) {
		// Increment received
		received++

		// If not Quiet print the message payload
		if !outputQuiet {
			fmt.Println(" [Received Message " + strconv.Itoa(received) + "]: Payload = " +
				string(message.Payload()) + "")
		}

		// Check if messageCount is not 0 and if received is greater than messageCount end subscribe loop and report
		// message received versus requested.
		if messageCount > 0 && received >= messageCount {
			fmt.Println("")
			fmt.Println("Received " + strconv.Itoa(received) + " : Requested " +
				strconv.Itoa(messageCount) + "Messages.")
			fmt.Println("")
			waitGroup.Done()
		}

	})

	// If Deliverytoken returns and DeliveryToken.Error is not nil log error and exit
	if mqttDeliverytoken.Wait() && mqttDeliverytoken.Error() != nil {
		log.Fatalf("[Error] - failed to setup mqtt subscription for [%s, %s]: %s\n",
			clientId, subscribeTopic, mqttDeliverytoken.Error())
	}

	// Set readyGroup.Done, this is done for multiple clients as one client might finish before others so only exit
	// this thread
	readyGroup.Done()

	// wait long enough for all messages to be received to be done
	waitGroup.Wait()

	// disconnect the mqttClient form the MQTT server
	mqttClient.Disconnect(0)
}
