package main

import (
	"fmt"
	"flag"
	"log"

	"tibco.com/ftl"
	"time"
	"os"
)

var (
	received       int64   = 0
	appName        string  = "tibrequest"
	realmServer    string  = "http://localhost:8080"
	endpointName   string  = "tibrequest-endpoint"
	trcLevel       string  = ""
	username       string  = "guest"
	password       string  = "guest-pw"
	identifier     string  = ""
	secondary      string  = ""
	clientLabel    string  = ""
	formatName     string  = "format-name"
	inboxFieldName string  = "My-Inbox"
	pingInterval   float64 = 1.0
	pongTimer      *time.Ticker
	payloadSize    int
	timer          *time.Ticker
	replyReceived  bool = false

	pub      ftl.Publisher
	replyMsg ftl.Message
	realm    ftl.Realm
	queue    ftl.Queue
	sub      ftl.Subscriber
	inbox    ftl.Inbox
	help     bool = false
)

var Usage = func() {
	fmt.Fprintf(os.Stderr, "Usage:  tibRequestEx [options] <realmserver_url>\n")
	fmt.Fprintf(os.Stderr, "Default url is %s\n", realmServer)
	fmt.Fprintf(os.Stderr, "   -a, --application <name>\n"+
		"       --client-label <label>\n"+
		"   -e, --endpoint <name>\n"+
		"   -h, --help\n"+
		"   -id,--identifier\n"+
		"   -p, --password <string>\n"+
		"   -s, --secondary <string>\n"+
		"   -t, --trace <level>\n"+
		"          where <level> can be:\n"+
		"                off, severe, warn, info, debug, verbose\n"+
		"   -u, --user <string>\n")
}

func Parse() error {
	flag.StringVar(&appName, "a", "tibrequest", "Application Name")
	flag.StringVar(&appName, "application", "tibrequest", "Application Name")

	flag.StringVar(&clientLabel, "cl", "", "clientLabel")
	flag.StringVar(&clientLabel, "clientLabel", "", "clientLabel")

	flag.StringVar(&endpointName, "e", "tibrequest-endpoint", "Endpoint Name")
	flag.StringVar(&endpointName, "endpoint", "tibrequest-endpoint", "Endpoint Name")

	flag.StringVar(&identifier, "id", "", "Identifier")
	flag.StringVar(&identifier, "identifier", "", "Identifier")

	flag.StringVar(&password, "p", "", "Password for realm server")
	flag.StringVar(&password, "password", "", "Password for realm server")

	flag.StringVar(&secondary, "s", "", "Secondary")
	flag.StringVar(&secondary, "secondary", "", "Secondary")

	flag.StringVar(&trcLevel, "t", "", "Trace level")
	flag.StringVar(&trcLevel, "trace", "", "Trace level")

	flag.StringVar(&username, "u", "", "User name for realm server")
	flag.StringVar(&username, "user", "", "User name for realm server")

	flag.BoolVar(&help, "help", false, "print usage")
	flag.BoolVar(&help, "h", false, "print usage")

	flag.Parse()

	if help {
		Usage()
		os.Exit(0)
	}

	if flag.NArg() > 0 {
		realmServer = flag.Arg(0)
	}

	return nil
}

func messagesReceived(msg ftl.Message) {
	fmt.Printf("Received reply message:\n" + msg.String())
	replyReceived = true
	return
}

func main() {
	// print the banner
	log.Println("--------------------------------------------------------------")
	log.Println("TibRequest")
	log.Println(ftl.Version())
	log.Println("--------------------------------------------------------------")
	Parse()
	request()
}

func initMessage() {
	if trcLevel != "" {
		replyReceived = true
		log.Println("resending initial message\n")
	}
}

func request() int {
	if trcLevel != "" {
		ftl.SetLogLevel(trcLevel)
	}

	realmProps := make(ftl.Props)
	if username != "" {
		realmProps[ftl.RealmPropertyStringUsername] = username
		realmProps[ftl.RealmPropertyStringUserpassword] = password
	}
	if identifier != "" {
		realmProps[ftl.RealmPropertyStringAppinstanceIdentifier] = identifier
	}
	if secondary != "" {
		realmProps[ftl.RealmPropertyStringClientLabel] = secondary
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

	errorchan := make(chan error)
	go func() {
		for problem := range errorchan {
			log.Fatal(ftl.ErrStack(problem))
			break
		}
	}()

	queue, err = realm.NewQueue(errorchan, nil)
	if err != nil {
		log.Fatal(ftl.ErrStack(err))
	}
	defer queue.Destroy()

	msgPipe := make(chan ftl.Message)
	sub, err = queue.SubscribeInbox(endpointName, nil, msgPipe)
	if err != nil {
		log.Println(ftl.ErrStack(err))
	}

	defer queue.CloseSubscription(sub)
	defer sub.Close()

	inbox, err := sub.Inbox()
	if err != nil {
		log.Println(ftl.ErrStack(err))
	}

	pub, err := realm.NewPublisher(endpointName, nil)
	if err != nil {
		log.Fatal(ftl.ErrStack(err))
	}
	defer pub.Close()

	msg, _ := realm.NewMessage(formatName)
	content := make(ftl.MsgContent)
	content["My-String"] = "request"
	content["My-Long"] = 10
	opaqueBytes := []byte("payload")
	content["My-Opaque"] = opaqueBytes
	content["My-Inbox"] = inbox
	msg.Set(content)

	defer msg.Destroy()

	if pingInterval != 0 {
		timer := time.NewTicker(time.Duration(pingInterval) * time.Millisecond)
		for {
			select {
				case <-timer.C:
					initMessage()
					fmt.Printf("Sending message\n")
					fmt.Println(msg.String())
					
					err := pub.Send(msg)
					if err != nil {
						log.Fatal(ftl.ErrStack(err))
					}

					fmt.Println("Receiving reply")
					for reply := range msgPipe {
						messagesReceived(reply)
						if replyReceived == true {
							break
						}
					}
			}
			break
		}
	}
	
	return 0
}
