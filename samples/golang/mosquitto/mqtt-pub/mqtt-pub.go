package main

import (
	"crypto/tls"
	"crypto/x509"
	"flag"
	"fmt"
	"io/ioutil"
	"log"
	"os"
	"path/filepath"
	"strconv"
	"sync"
	"time"

	mqtt "github.com/eclipse/paho.mqtt.golang"
)

var (
	user          string          = ""
	password      string          = ""
	host          string          = "localhost"
	port          string          = "1883"
	brokerAddress string          = host + ":" + port
	count         int             = 1
	interval      time.Duration   = 1 * time.Second
	topic         string          = "test"
	retained      bool            = false
	qos           int             = 0
	message       string          = ""
	cafile        string          = ""
	clientCount   int             = 1
	clientGroup   *sync.WaitGroup = &sync.WaitGroup{}
	readyGroup    *sync.WaitGroup = &sync.WaitGroup{}
)

func parseArgs() error {
	flag.Usage = func() {
		usage := filepath.Base(os.Args[0]) + " [options]\n\ta sample MQTT publisher"
		fmt.Printf("\nUsage: %s\n\n", usage)
		fmt.Println("Options:")
		flag.PrintDefaults()
	}

	flag.StringVar(&user, "u", user, "Username")
	flag.StringVar(&user, "user", user, "Username")
	flag.StringVar(&password, "pw", password, "Password")
	flag.StringVar(&password, "password", password, "Password")
	flag.StringVar(&host, "h", host, "Broker host")
	flag.StringVar(&host, "host", host, "Broker host")
	flag.StringVar(&port, "p", port, "Broker port")
	flag.StringVar(&port, "port", port, "Broker port")
	flag.IntVar(&count, "c", count, "Total number of messages to send. 0 means endless.")
	flag.IntVar(&count, "count", count, "Total number of messages to send. 0 means endless.")
	flag.StringVar(&topic, "t", topic, "set MQTT topic")
	flag.StringVar(&topic, "topic", topic, "set MQTT topic")
	flag.IntVar(&clientCount, "C", clientCount, "Number of clients to create")
	flag.IntVar(&clientCount, "clients", clientCount, "Number of clients to create")
	flag.StringVar(&cafile, "cafile", cafile, "Broker certificate file")
	flag.StringVar(&message, "m", message, "set MQTT message")
	flag.StringVar(&message, "message", message, "set MQTT message")
	flag.BoolVar(&retained, "r", retained, "set MQTT retained flag")
	flag.BoolVar(&retained, "retained", retained, "set MQTT retained flag")
	flag.IntVar(&qos, "q", qos, "set MQTT qos flag")
	flag.IntVar(&qos, "qos", qos, "set MQTT qos flag")
	flag.DurationVar(&interval, "i", interval, "Interval between sends")
	flag.DurationVar(&interval, "interval", interval, "Interval between sends")

	flag.Parse()

	brokerAddress = host + ":" + port
	return nil
}

func main() {
	// print the banner
	parseArgs()
	for i := 0; i < clientCount; i++ {
		clientGroup.Add(1)
		readyGroup.Add(1)
		go send(i)
	}
	readyGroup.Wait()
	clientGroup.Wait()
}

func send(id int) {
	defer clientGroup.Done()
	cid := "mqtt-pub-" + strconv.Itoa(id) + "-" + strconv.Itoa(os.Getpid())
	opts := mqtt.NewClientOptions().
		AddBroker(brokerAddress).
		SetAutoReconnect(false).
		SetClientID(cid).
		SetMaxReconnectInterval(1 * time.Minute).
		SetUsername(user).
		SetPassword(password)
	if cafile != "" {
		tlsConfig := &tls.Config{}
		tlsCerts := x509.NewCertPool()
		if pemCert, err := ioutil.ReadFile(cafile); err != nil {
			log.Fatalf("failed to read trust file: %s\n", err)
		} else {
			tlsCerts.AppendCertsFromPEM(pemCert)
			tlsConfig.RootCAs = tlsCerts
		}
		opts.SetTLSConfig(tlsConfig)
	}
	c := mqtt.NewClient(opts)
	token := c.Connect()
	token.Wait()
	if token.Error() != nil {
		log.Fatalf("failed to connect to MQTT broker: %s\n", token.Error())
	}
	readyGroup.Done()
	sent := 0
	timer := time.NewTicker(interval)
	sub := topic
	if id > 0 {
		sub = topic + "/" + strconv.Itoa(id)
	}
	for {
		select {
		case <-timer.C:
			token := c.Publish(sub, byte(qos), retained, message)
			sent++
			if !token.WaitTimeout(interval) {
				log.Fatalf("timed out publishing message #%d", sent)
			} else if err := token.Error(); err != nil {
				log.Fatalf("failed to send MQTT message #%d: %s\n", sent, err)
			}
			if count > 0 && sent >= count {
				return
			}
		}
	}
}
