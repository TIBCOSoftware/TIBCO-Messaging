// Copyright © 2018. TIBCO Software Inc.

package main

// Golang Imports
import (
	"encoding/json"
	"flag"
	"fmt"
	"io/ioutil"
	"log"
	"net/http"
	"os"
	"path/filepath"
	"strings"
	"time"

	"github.com/TIBCOSoftware/TIBCO-Messaging/eftl-golang-sdk"
	"os/signal"
)

//stock-publisher global variable list
var (
	clientID	= ""
	key			= ""
	url 		= ""
)

var quoteChan = make(chan bool, 1)

//stock-publisher quotes map structure : Map string[quote-symbol] quote
var quotes = map[string]quote{
	"aapl": {},
	"amzn": {},
	"goog": {},
	"ibm":  {},
	"msft": {},
}

//stock-publisher quote structure populated from the IEX RESTful interface.
type quote struct {
	Symbol 				string 		`json:"symbol"`
	CompanyName	 		string 		`json:"companyName"`
	PrimaryExchange			string 		`json:"primaryExchange"`
	Sector				string 		`json:"sector"`
	CalculationPrice		string 		`json:"calculationPrice"`
	Open				float32 	`json:"open"`
	OpenTime			int64 		`json:"openTime"`
	Close				float32		`json:"close"`
	CloseTime			int64		`json:"closeTime"`
	High				float32		`json:"high"`
	Low				float32		`json:"low"`

	LatestPrice			float32		`json:"latestPrice"`
	LatestSource			string		`json:"latestSource"`
	LatestTime			string 		`json:"latestTime"`
	LatestUpdate			int64		`json:"latestUpdate"`
	LatestVolume			int64		`json:"latestVolume"`

	IexRealtimePrice		float32		`json:"iexRealtimePrice"`
	IexRealtimeSize			int32		`json:"iexRealtimeSize"`
	IexRealtimeUpdated		int64		`json:"iexRealtimeUpdated"`

	DelayedPrice			float32	 	`json:"delayedPrice"`
	DelayedPriceTime		int64		`json:"delayedPriceTime"`

	ExtendedPrice			float32	 	`json:"extendedPrice"`
	ExtendedChange			float32	 	`json:"extendedChange"`
	ExtendedChangePercent		float32	 	`json:"extendedChangePercent"`
	ExtendedPriceTime		int64	 	`json:"extendedPriceTime"`

	PreviousClose			float32	 	`json:"previousClose"`
	Change				float32	 	`json:"change"`
	ChangePercent			float32	 	`json:"changePercent"`

	IexMarketPercent		float32	 	`json:"iexMarketPercent"`

	IexVolume			int32	 	`json:"iexVolume"`
	AvgTotalVolume			int64	 	`json:"avgTotalVolume"`

	IexBidPrice			float32	 	`json:"iexBidPrice"`
	IexBidSize			int32	 	`json:"iexBidSize"`

	IexAskPrice			float32	 	`json:"iexAskPrice"`
	IexAskSize			int32	 	`json:"iexAskSize"`

	MarketCap			int64	 	`json:"marketCap"`
	PeRatio				float32	 	`json:"peRatio"`
	Week52High			float32	 	`json:"week52High"`
	Week52Low			float32	 	`json:"week52Low"`
	YtdChange			float32	 	`json:"ytdChange"`
}

// parseArgs parses the commandline arguments using the golang flag operations
func parseArgs() error {
	// Usage output function to display stock-publisher.go
	flag.Usage = func() {
		usage := filepath.Base(os.Args[0]) + " [options]\n\tA sample TIBCO Cloud Messaging publisher that pulls" +
			" stock prices from IEX Group Rest API and publishes that data to a TIBCO Cloud Messaging subscriber"
		fmt.Printf("\nUsage: %s\n\n", usage)
		fmt.Println("Options:")
		flag.PrintDefaults()
	}

	flag.StringVar(&clientID, "c", clientID, "Client ID used when connecting to TIBCO Cloud Messaging")
	flag.StringVar(&key, "k", key, "TIBCO CLoud Messaging authentication key")
	flag.StringVar(&url, "u", url, "TIBCO Cloud Messaging url provided as part of your " +
		"TIBCO Cloud Messaging subscription")

	flag.Parse()

	return nil
}

// quoteTicker function that pulls quotes from the IEX via REST.
func quoteTicker() error {

	for {
		// Get the stock quotes for AAPL, IBM, GOOG, AMZN, and MSFT

		for key := range quotes {

			quoteURL := "https://api.iextrading.com/1.0/stock/" + key + "/quote"

			response, err := http.Get(quoteURL)

			if err != nil {
				fmt.Printf("[Error] - Failure to connect to IEX Group: %s\n", err)
				return err
			}

			data, _ := ioutil.ReadAll(response.Body)

			var tempQuote quote

			err = json.Unmarshal(data, &tempQuote)

			if err != nil {
				fmt.Printf("[Error] - Failure to unMarshal JSON data: %s\n", err)
				return err
			}

			quotes[key] = tempQuote

			// Sleep 200 milliseconds between gathering each quote
			time.Sleep(200 * time.Millisecond)
		}

		quoteChan <- true

		// Sleep 4 seconds between iterations
		time.Sleep(4000 * time.Millisecond)
	}

	return nil
}

// stock-publisher.go main function that drives quoteTicker and publishing of data via TCM/eFTL
func main() {

	// Print Application Banner that includes the eFTL version
	fmt.Println(" ")
	fmt.Println("Starting " + filepath.Base(os.Args[0]) + ":")
	fmt.Println("   Sample TCM stock publisher aaplication")
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

	// Set eFTL connection options for TCM connection
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

	// Spin quoteTicker function
	go quoteTicker()

	/*
		Initialize the quote data, hold off publishing via TCM/eFTL until data is initialized
	 */
	log.Printf("[TCM Info] - TIBCO Cloud Messaging Initializing Quote Data \n")

	quoteKeys := make([]string, 0, len(quotes))

	for key := range quotes {
		quoteKeys = append(quoteKeys, key)
	}

	log.Printf("[TCM Info] - Quote List [%s]\n", strings.Join(quoteKeys,", "))

	// Sleep 5 Seconds to allow for Qoutes to be primed
	time.Sleep(5000 * time.Millisecond)



	// Create a message and populate its contents with the Quote JSON data.
	//
	// Message fields can be of type string, long, double, date, and message, along with arrays
	// of each of the types.
	//
	// Subscribers will create matchers that match on one or more of the message fields. Only
	// string and long fields are supported by matchers.
	//

	for ready := range quoteChan {
		if ready {
			for _, value := range quotes {

				tcmMessage := eftl.Message{
					"symbol":                value.Symbol,
					"companyName":           value.CompanyName,
					"primaryExchange":       value.PrimaryExchange,
					"sector":                value.Sector,
					"calculationPrice":      value.CalculationPrice,
					"open":                  value.Open,
					"openTime":              value.OpenTime,
					"close":                 value.Close,
					"closeTime":             value.CloseTime,
					"high":                  value.High,
					"low":                   value.Low,
					"latestPrice":           value.LatestPrice,
					"latestSource":          value.LatestSource,
					"latestTime":            value.LatestTime,
					"latestUpdate":          value.LatestUpdate,
					"latestVolume":          value.LatestVolume,
					"iexRealtimePrice":      value.IexRealtimePrice,
					"iexRealtimeSize":       value.IexRealtimeSize,
					"iexRealtimeUpdated":    value.IexRealtimeUpdated,
					"delayedPrice":          value.DelayedPrice,
					"delayedPriceTime":      value.DelayedPriceTime,
					"extendedPrice":         value.ExtendedPrice,
					"extendedChange":        value.ExtendedChange,
					"extendedChangePercent": value.ExtendedChangePercent,
					"extendedPriceTime":     value.ExtendedPriceTime,
					"previousClose":         value.PreviousClose,
					"change":                value.Change,
					"changePercent":         value.ChangePercent,
					"iexMarketPercent":      value.IexMarketPercent,
					"iexVolume":             value.IexVolume,
					"avgTotalVolume":        value.AvgTotalVolume,
					"iexBidPrice":           value.IexBidPrice,
					"iexBidSize":            value.IexBidSize,
					"iexAskPrice":           value.IexAskPrice,
					"iexAskSize":            value.IexAskSize,
					"marketCap":             value.MarketCap,
					"peRatio":               value.PeRatio,
					"week52High":            value.Week52High,
					"week52Low":             value.Week52Low,
					"ytdChange":             value.YtdChange,
				}

				// Publish the message to TIBCO Cloud Messaging.
				err := connection.Publish(tcmMessage)
				if err != nil {
					log.Println("[TCM Error] - TIBCO Cloud Messaging publish failed:", err)
				}

				log.Printf("[TCM Info] - TIBCO Cloud Messaging publishing Quote for [%s]\n", value.Symbol)
			}

			log.Printf("[TCM Info] - TIBCO Cloud Messaging published All Quotes")

			quoteChan <- false
		}

	}
}
