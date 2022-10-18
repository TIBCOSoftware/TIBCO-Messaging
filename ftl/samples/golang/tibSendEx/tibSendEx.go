package main

import (
	"flag"
	"fmt"
	"log"
	"os"
	"time"

	"tibco.com/ftl"
)

var (
	delay        int64  = 1000
	defaultUrl   string = "http://localhost:8080"
	user         string = "guest"
	password     string = "guest-pw"
	appName      string = "tibsend"
	realmServer  string = ""
	endpointName string = "tibsend-endpoint"
	identifier   string = ""
	count        int64  = 1
	sent         int64  = 0
	index        int64  = 0
	trcLevel     string = ""
	trustAll     bool   = false
	trustFile    string = ""
	clientLabel  string = ""
	secondary    string = ""
	keyedOpaque  bool   = false
	formatName   string = "Format-1"
	doneSending  bool   = false
	myData       string = "_data"
	msg          ftl.Message
	realm        ftl.Realm
	timer        *time.Ticker
	help         bool = false
)

var Usage = func() {
	fmt.Fprintf(os.Stderr, "Usage:  tibSendEx [options] <realmserver_url>\n")
	fmt.Fprintf(os.Stderr, "Default url is %s\n", defaultUrl)
	fmt.Fprintf(os.Stderr, "   -a, --application <name>\n"+
		"        --client-label <label>\n"+
		"    -c, --count <int>\n"+
		"    -d, --delay <int> (in millis)\n"+
		"    -e, --endpoint <name>\n"+
		"    -f, --format <name>\n"+
		"    -h, --help\n"+
		"    -id, --identifier\n"+
		"    -ko, --keyedopaque\n"+
		"    -p, --password <string>\n"+
		"    -s, --secondary <string>\n"+
		"    -t, --trace <level>\n"+
		"           where <level> can be:\n"+
		"                 off, severe, warn, info, debug, verbose\n"+
		"        --trustall\n"+
		"        --trustfile <file>\n"+
		"    -u, --user <string>\n")
}

func parseArgs() error {
	flag.StringVar(&user, "user", "", "Username")
	flag.StringVar(&user, "u", "", "Username")

	flag.StringVar(&password, "password", "", "Password")
	flag.StringVar(&password, "p", "", "Password")

	flag.StringVar(&identifier, "identifier", "", "Identifier")
	flag.StringVar(&identifier, "id", "", "Identifier")

	flag.StringVar(&appName, "application", "tibsend", "App Name")
	flag.StringVar(&appName, "a", "tibsend", "App Name")

	flag.StringVar(&endpointName, "endpoint", "tibsend-endpoint", "Endpoint Name")
	flag.StringVar(&endpointName, "e", "tibsend-endpoint", "Endpoint Name")

	flag.Int64Var(&count, "count", 10, "Number of messages to send")
	flag.Int64Var(&count, "c", 10, "Number of messages to send")

	flag.StringVar(&clientLabel, "client-label", "", "client label")
	flag.StringVar(&clientLabel, "cl", "", "client label")

	flag.BoolVar(&trustAll, "trustall", false, "trustAll")

	flag.StringVar(&trustFile, "trustfile", "", "trust file")

	flag.StringVar(&secondary, "secondary", "", "Secondary Realmserver URL")
	flag.StringVar(&secondary, "s", "", "Secondary Realmserver URL")

	flag.StringVar(&trcLevel, "trace", "", "where <level> can be: off, severe, warn, info, debug, verbose")
	flag.StringVar(&trcLevel, "t", "", "where <level> can be: off, severe, warn, info, debug, verbose")

	flag.Int64Var(&delay, "delay", 1000, "Delay between sends")
	flag.Int64Var(&delay, "d", 1000, "Delay between sends")

	flag.StringVar(&formatName, "format", "Format-1", "Format Name")
	flag.StringVar(&formatName, "f", "Format-1", "Format Name")

	flag.BoolVar(&keyedOpaque, "keyedopaque", false, "send keyedOpaque msgs")
	flag.BoolVar(&keyedOpaque, "ko", false, "send keyedOpaque msgs")

	flag.BoolVar(&help, "help", false, "print usage")
	flag.BoolVar(&help, "h", false, "print usage")

	flag.Parse()

	if help {
		Usage()
		os.Exit(0)
	}

	if flag.NArg() > 0 {
		realmServer = flag.Arg(0)
	} else {
		realmServer = defaultUrl
	}

	return nil
}

func main() {
	// print the banner
	log.Println("--------------------------------------------------------------")
	log.Println("TibSendEx")
	log.Println(ftl.Version())
	log.Println("--------------------------------------------------------------")
	send()
}

func initMessage() {

	// clear the content
	msg.Clear()
	if keyedOpaque {
		content := make(ftl.MsgContent)
		key := fmt.Sprintf("%d", index)
		index++
		content["ftl.BuiltinMsgFmtKeyFieldName"] = key
		opaque := []byte("opaque")
		content["_data"] = opaque
		msg.Set(content)
	} else {
		content := make(ftl.MsgContent)
		content["My-String"] = endpointName
		index = index + 1
		content["My-Long"] = index
		opaque := []byte("opaque")
		content["My-Opaque"] = opaque
		msg.Set(content)
	}
}

func processNotifications(realm ftl.Realm) {
	notificationsPipe := make(chan ftl.Notification)
	realm.SetNotificatonChannel(notificationsPipe)

	for notification := range notificationsPipe {
		if notification.Notify == ftl.NotificationClientDisabled {
			log.Println("application administratively disabled: " + notification.Reason)
			doneSending = true
		} else {
			log.Printf("Notification type %d : %s", notification.Notify, notification.Reason)
		}
	}
}

func send() int {
	parseArgs()

	if trcLevel != "" {
		ftl.SetLogLevel(trcLevel)
	}

	// Set up properties of realm connection
	realmProps := make(ftl.Props)
	if user != "" {
		realmProps[ftl.RealmPropertyStringUsername] = user
		realmProps[ftl.RealmPropertyStringUserpassword] = password
	}

	if identifier != "" {
		realmProps[ftl.RealmPropertyStringAppinstanceIdentifier] = identifier
	}

	if trustFile != "" {
		realmProps[ftl.RealmPropertyLongTrustType] = ftl.RealmHTTPSConnectionUseSpecifiedTrustFile
		realmProps[ftl.RealmPropertyStringTrustFile] = trustFile
	} else if trustAll {
		realmProps[ftl.RealmPropertyLongTrustType] = ftl.RealmHTTPSConnectionTrustEveryone
	}

	if secondary != "" {
		realmProps[ftl.RealmPropertyStringSecondaryServer] = secondary
	}

	if clientLabel != "" {
		realmProps[ftl.RealmPropertyStringClientLabel] = clientLabel
	}

	// Connect to the realm server
	realm, err := ftl.Connect(realmServer, appName, realmProps)
	if err != nil {
		log.Fatal(ftl.ErrStack(err))
	}
	defer realm.Close()

	// process notifications
	go processNotifications(realm)

	if keyedOpaque == true {
		msg, _ = realm.NewMessage(ftl.BuiltinMsgFmtKeyedOpaque)
	} else {
		msg, _ = realm.NewMessage(formatName)
	}

	pub, err := realm.NewPublisher(endpointName, nil)
	if err != nil {
		log.Fatal(ftl.ErrStack(err))
	}
	defer pub.Close()

	timeout := delay
	timer := time.NewTicker(time.Duration(timeout * int64(time.Millisecond)))
	go func() {
		for {
			select {
			case <-timer.C:
				if (sent == count) || doneSending {
					break
				}

				initMessage()
				log.Printf("Sending message [%d]:\n", index)
				log.Println(msg.String())
				err := pub.Send(msg)
				if err != nil {
					log.Fatal(ftl.ErrStack(err))
				}
				sent++

				if (sent == count) || doneSending {
					break
				}
			}
		}
		return
	}()

	// wait long enough for send to be done
	time.Sleep(time.Duration(delay * (count * 2) * int64(time.Millisecond)))
	// Clean up
	msg.Destroy()
	return 0
}
