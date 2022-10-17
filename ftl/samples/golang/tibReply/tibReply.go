package main

import (
	"flag"
	"fmt"
	"log"
	"os"

	"tibco.com/ftl"
)

var (
	count          int64  = 1
	received       int64  = 0
	appName        string = "tibreply"
	realmServer    string = "http://localhost:8080"
	endpointName   string = "tibreply-endpoint"
	trcLevel       string = ""
	username       string = "guest"
	password       string = "guest-pw"
	identifier     string = ""
	secondary      string = ""
	clientLabel    string = ""
	formatName     string = "format-name"
	inboxFieldName string = "inbox-name"
	doneReceiving  bool   = false

	pub      ftl.Publisher
	replyMsg ftl.Message
	realm    ftl.Realm
	queue    ftl.Queue
	sub      ftl.Subscriber
	help     bool = false
)

const (
	FieldNameOpaque = "opaque-name"
	FieldNameLong   = "long-name"
	FieldNameDouble = "double-name"
	FieldNameString = "string-name"
	FieldNameInbox  = "inbox-name"
)

var Usage = func() {
	fmt.Fprintf(os.Stderr, "Usage:  tibReplyEx [options] <realmserver_url>\n")
	fmt.Fprintf(os.Stderr, "Default url is %s\n", realmServer)
	fmt.Fprintf(os.Stderr, "   -a, --application <name>\n"+
		"       --client-label <label>\n"+
		"   -c, --count <int>\n"+
		"   -e, --endpoint <name>\n"+
		"   -h, --help\n"+
		"   -id, --identifier\n"+
		"   -p, --password <string>\n"+
		"   -s, --secondary <string>\n"+
		"   -t, --trace <level>\n"+
		"  where --trace can be:\n"+
		"    off, severe, warn, info, debug, verbose\n" +
		"   -u, --user <string>\n")

}

func Parse() error {
	flag.StringVar(&appName, "a", "tibreply", "Application Name")
	flag.StringVar(&appName, "application", "tibreply", "Application Name")

	flag.StringVar(&clientLabel, "cl", "", "clientLabel")
	flag.StringVar(&clientLabel, "clientLabel", "", "clientLabel")

	flag.Int64Var(&count, "c", 1, "count")
	flag.Int64Var(&count, "count", 1, "count")

	flag.StringVar(&endpointName, "e", "tibreply-endpoint", "Endpoint Name")
	flag.StringVar(&endpointName, "endpoint", "tibreply-endpoint", "Endpoint Name")

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

func main() {
	// print the banner
	log.Println("--------------------------------------------------------------")
	log.Println("TibReply")
	log.Println(ftl.Version())
	log.Println("--------------------------------------------------------------")
	// parse configuration
	Parse()
	reply()
}

func reply() int {
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
	queue, _ = realm.NewQueue(errorchan, nil)
	if err != nil {
		log.Fatal(ftl.ErrStack(err))
	}
	defer queue.Destroy()

	subPipe := make(chan ftl.Message)
	sub, err = queue.Subscribe(endpointName, "", nil, subPipe)
	if err != nil {
		log.Fatal(ftl.ErrStack(err))
	}
	defer queue.CloseSubscription(sub)
	defer sub.Close()

	pub, err = realm.NewPublisher(endpointName, nil)
	if err != nil {
		log.Fatal(ftl.ErrStack(err))
	}
	defer pub.Close()

	replyMsg, _ = realm.NewMessage(formatName)
	defer replyMsg.Destroy()

	fmt.Printf("Waiting for request(s)\n")

	for msg := range subPipe {
		messagesReceived(msg) 
		if (received == count) {
			break
		}
	}

	return 0
}

func messagesReceived(msg ftl.Message) {
	var sendInbox ftl.Inbox
	var ct ftl.MsgContent

	received++
	fmt.Printf("Received message [%d]: ", received)
	fmt.Println(msg)

	content, err := msg.Get()
	if err == nil {
		for fieldName, value := range content {
			fmt.Printf("Name: %s\n", fieldName)
			fmt.Printf("    Value: %v\n", value)  
		}
		sendInbox, _ = content["My-Inbox"].(ftl.Inbox)
	} else {
		fmt.Println(ftl.ErrStack(err))
		fmt.Println(err.Error())
	}

	ct, _ = replyMsg.Get()
	ct["My-String"] = "reply"
	replyMsg.Set(ct)

	fmt.Printf("Sending Reply [%d]: \n", received)
	err = pub.SendToInbox(sendInbox, replyMsg)
	if err != nil {
		log.Println(ftl.ErrStack(err))
		log.Println(err.Error())
	}
	return
}
