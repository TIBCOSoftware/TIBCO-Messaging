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
	"strings"
	"time"

	"github.com/TIBCOSoftware/TIBCO-Messaging/eftl-golang-sdk"
)

//stock-subscriber global variable list
var (
	url 		= ""
	clientID	= ""
	key		= ""
	symbol		= ""
)

//stock-subscriber quotes map structure : Map string[quote-symbol] quote
var quotes = map[string]*quote{
	"aapl": {},
	"amzn": {},
	"goog": {},
	"ibm":  {},
	"msft": {},
}

//stock-subscriber quote structure populated from the eFTL message.
type quote struct {
	Symbol 				string
	CompanyName 			string
	PrimaryExchange			string
	Sector				string
	CalculationPrice		string
	Open				float64
	OpenTime			int64
	Close				float64
	CloseTime			int64
	High				float64
	Low				float64

	LatestPrice			float64
	LatestSource			string
	LatestTime			string
	LatestUpdate			int64
	LatestVolume			int64

	IexRealtimePrice		float64
	IexRealtimeSize			int64
	IexRealtimeUpdated		int64
	DelayedPrice			float64
	DelayedPriceTime		int64

	ExtendedPrice			float64
	ExtendedChange			float64
	ExtendedChangePercent		float64
	ExtendedPriceTime		int64

	PreviousClose			float64
	Change				float64
	ChangePercent			float64

	IexMarketPercent		float64

	IexVolume			int64
	AvgTotalVolume			int64

	IexBidPrice			float64
	IexBidSize			int64

	IexAskPrice			float64
	IexAskSize			int64

	MarketCap			int64
	PeRatio				float64
	Week52High			float64
	Week52Low			float64
	YtdChange			float64
}


// parseArgs parses the commandline arguments using the golang flag operations
func parseArgs() error {
	// Usage output function to display stock-subscriber.go
	flag.Usage = func() {
		usage := filepath.Base(os.Args[0]) + " [options]\n\tA sample TIBCO Cloud Messaging subscriber that connects" +
			" to TIBCO Cloud Messaging and subscribes to stock quotes"
		fmt.Printf("\nUsage: %s\n\n", usage)
		fmt.Println("Options:")
		flag.PrintDefaults()
	}

	flag.StringVar(&url, "u", url, "TIBCO Cloud Messaging url provided as part of your " +
		"TIBCO Cloud Messaging subscription")
	flag.StringVar(&clientID, "c", clientID, "Client ID used when connecting to TIBCO Cloud Messaging")
	flag.StringVar(&key, "k", key, "TIBCO CLoud Messaging authentication key")
	flag.StringVar(&symbol, "s", symbol, "Symbol to use in TIBCO eFTL Matcher to filter message data")

	flag.Parse()

	return nil
}

// stock-subscriber.go main function that drives connection and subscription to TCM/eFTL
func main() {

	// Print Application Banner that includes the eFTL version
	fmt.Println(" ")
	fmt.Println("Starting " + filepath.Base(os.Args[0]) + ":")
	fmt.Println("   Sample TCM stock subscriber application")
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

	// set eFTL connection options for TCM
	opts := &eftl.Options{
		ClientID:  clientID,
		Password:  key,
	}

	// Create error channel for receiving eFTL connection errors
	errChan := make(chan error, 1)

	// Connect to TIBCO Cloud Messaging calling eftl.Connect if err does not equal nil log err and exit
	connection, err := eftl.Connect(url, opts, errChan)
	if err != nil {
		log.Println("[TCM Error] - Failure to Connect to TIBCO Cloud Messaging: ", err)
		return
	}

	// Connection succesful, defer connection Disconnect to disconnect from TCM/eFTL at application close
	defer connection.Disconnect()

	// Listen for eFTL/TCM connection errors.
	go func() {
		for err := range errChan {
			log.Println("[TCM Error] - TIBCO Cloud Messaging connection error:", err)

			for !connection.IsConnected() {
				// Connection Error encountered try to reconnect
				connection.Reconnect()
				time.Sleep(5000 * time.Millisecond)
			}
		}
	}()

	// Create message channel for recieving eFTL messages
	messageChannel := make(chan eftl.Message, 100)

	// Create a subscription in TIBCO Cloud Messaging
	//
	// This subscription matches all published messages that contain a
	// field named "symbol" with a value of "APPL"
	//
	// This subscription also sets the durable name to "APPL" which allows
	// the client to receive messages that were published while disconnected
	//

	// Set up matcher to filter on, by default matcher is empty to receive all messages on TCM channel
	matcher := ""

	if symbol != "" {
		matcher = "{" + "\"symbol\"" + ":"+ "\"" + strings.ToUpper(symbol) + "\"" + "}"
	}

	// Create the subscriber
	subscriber, err := connection.Subscribe(matcher, "", messageChannel)

	if err != nil {
		log.Println("[TCM Error] - TIBCO Cloud Messaging subscribe failed: ", err)
		return
	}

	// unsubscribe from messages when done
	defer connection.Unsubscribe(subscriber)

	// listen for messages on messageChannel
	for message := range messageChannel {

		quoteKey := message["symbol"].(string)

		switch quoteKey {
		case "AAPL":
			quoteKey = "aapl"
		case "IBM":
			quoteKey = "ibm"
		case "GOOG":
			quoteKey = "goog"
		case "AMZN":
			quoteKey = "amzn"
		case "MSFT":
			quoteKey = "msft"
		default:
			log.Printf("[TCM Error] - Unknown Quote Symbol - %s - Ignoring Quote\n", quoteKey)
			quoteKey = ""
		}

		if quoteKey != "" {
			for messageFieldKey, messageFieldValue := range message {
				switch messageFieldKey {
				case "symbol":
					quotes[quoteKey].Symbol = messageFieldValue.(string)
				case "companyName":
					quotes[quoteKey].CompanyName = messageFieldValue.(string)
				case "primaryExchange":
					quotes[quoteKey].PrimaryExchange = messageFieldValue.(string)
				case "sector":
					quotes[quoteKey].Sector = messageFieldValue.(string)
				case "calculationPrice":
					quotes[quoteKey].CalculationPrice = messageFieldValue.(string)
				case "open":
					quotes[quoteKey].Open = messageFieldValue.(float64)
				case "openTime":
					quotes[quoteKey].OpenTime = messageFieldValue.(int64)
				case "close":
					quotes[quoteKey].Close = messageFieldValue.(float64)
				case "closeTime":
					quotes[quoteKey].CloseTime = messageFieldValue.(int64)
				case "high":
					quotes[quoteKey].High = messageFieldValue.(float64)
				case "low":
					quotes[quoteKey].Low = messageFieldValue.(float64)
				case "latestPrice":
					quotes[quoteKey].LatestPrice = messageFieldValue.(float64)
				case "latestSource":
					quotes[quoteKey].LatestSource = messageFieldValue.(string)
				case "latestTime":
					quotes[quoteKey].LatestTime = messageFieldValue.(string)
				case "latestUpdate":
					quotes[quoteKey].LatestUpdate = messageFieldValue.(int64)
				case "latestVolume":
					quotes[quoteKey].LatestVolume = messageFieldValue.(int64)
				case "iexRealtimePrice":
					quotes[quoteKey].IexRealtimePrice = messageFieldValue.(float64)
				case "iexRealtimeSize":
					quotes[quoteKey].IexRealtimeSize = messageFieldValue.(int64)
				case "iexRealtimeUpdated":
					quotes[quoteKey].IexRealtimeUpdated = messageFieldValue.(int64)
				case "delayedPrice":
					quotes[quoteKey].DelayedPrice = messageFieldValue.(float64)
				case "delayedPriceTime":
					quotes[quoteKey].DelayedPriceTime = messageFieldValue.(int64)
				case "extendedPrice":
					quotes[quoteKey].ExtendedPrice = messageFieldValue.(float64)
				case "extendedChange":
					quotes[quoteKey].ExtendedChange = messageFieldValue.(float64)
				case "extendedChangePercent":
					quotes[quoteKey].ExtendedChangePercent = messageFieldValue.(float64)
				case "extendedPriceTime":
					quotes[quoteKey].ExtendedPriceTime = messageFieldValue.(int64)
				case "previousClose":
					quotes[quoteKey].PreviousClose = messageFieldValue.(float64)
				case "change":
					quotes[quoteKey].Change = messageFieldValue.(float64)
				case "changePercent":
					quotes[quoteKey].ChangePercent = messageFieldValue.(float64)
				case "iexMarketPercent":
					quotes[quoteKey].IexMarketPercent = messageFieldValue.(float64)
				case "iexVolume":
					quotes[quoteKey].IexVolume = messageFieldValue.(int64)
				case "avgTotalVolume":
					quotes[quoteKey].AvgTotalVolume = messageFieldValue.(int64)
				case "iexBidPrice":
					quotes[quoteKey].IexBidPrice = messageFieldValue.(float64)
				case "iexBidSize":
					quotes[quoteKey].IexBidSize = messageFieldValue.(int64)
				case "iexAskPrice":
					quotes[quoteKey].IexAskPrice = messageFieldValue.(float64)
				case "iexAskSize":
					quotes[quoteKey].IexAskSize = messageFieldValue.(int64)
				case "marketCap":
					quotes[quoteKey].MarketCap = messageFieldValue.(int64)
				case "peRatio":
					quotes[quoteKey].PeRatio = messageFieldValue.(float64)
				case "week52High":
					quotes[quoteKey].Week52High = messageFieldValue.(float64)
				case "week52Low":
					quotes[quoteKey].Week52Low = messageFieldValue.(float64)
				case "ytdChange":
					quotes[quoteKey].YtdChange = messageFieldValue.(float64)
				default:
					log.Printf("[TCM Error] - Unknown Message Field - %s - Ingnoring Field\n", messageFieldKey)
				}

			}

			log.Printf("[TCM Stock Quote for %s Received]: ", quotes[quoteKey].Symbol)
			log.Printf("\tCompany Name: (%s)\n", quotes[quoteKey].CompanyName)
			log.Printf("\t\tPrice: %.2f, Volume: %d, High: %.2f, Low: %.2f\n",
				quotes[quoteKey].LatestPrice, quotes[quoteKey].LatestVolume,
				quotes[quoteKey].High, quotes[quoteKey].Low)
		}
	}

}
