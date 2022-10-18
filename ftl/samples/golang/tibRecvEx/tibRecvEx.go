package main

import (
	"flag"
	"fmt"
	"log"
	"os"

	"tibco.com/ftl"
)

var (
	count            int64  = 1
	appName          string = "tibrecv"
	clientLabel      string = ""
	durableName      string = ""
	subEndpoint      string = "tibrecv-endpoint"
	identifier       string = ""
	user             string = "guest"
	password         string = "guest-pw"
	trustFile        string = ""
	trustAll         bool   = false
	match            string = ""
	secondary        string = ""
	trcLevel         string = ""
	doneReceiving    bool   = false
	received         int64  = 0
	explicitAck      bool   = false
	ackableMsgs      []ftl.Message
	defaultUrl       string = "http://localhost:8080"
	realmServer      string = ""
	discardPolicy    int    = ftl.EventQueueDiscardPolicyNew
	maxEvents        int    = 1000
	discardPolicyStr string = "none"
	ackMode          string = "auto"
	discardAmount    int    = 1
	inline           bool   = false
	advisoryType     string = ""
	advisoryQueue    string = "advisoryQueue"
	unsubscribe      bool   = false
	noAdvisory       bool
	realm            ftl.Realm
	sub              ftl.Subscriber
	advSub           ftl.Subscriber
	queue            ftl.Queue
	advPipe          chan ftl.Message
	size             int  = -1
	help             bool = false
)

var Usage = func() {
	fmt.Fprintf(os.Stderr, "Usage: tibRecvEx [options] <realmserver_url>  ('-' or '--' may be used with both the short and long option names)\n\n")
	fmt.Fprintf(os.Stderr, "     Default url is %s\n", defaultUrl)
	fmt.Fprintf(os.Stderr, "\n"+
		"    -a,  --application <name>\n"+
		"    -d,  --durablename <name>\n"+
		"    -c,  --count <int>\n"+
		"         --client-label <label>\n"+
		"    -e,  --endpoint <name>\n"+
		"    -h,  --help\n"+
		"    -id, --identifier <name>\n"+
		"    -m,  --match <content match>\n"+
		"            where <content match> is of the form:\n"+
		"                 '{\"fieldname\":<value>}'\n"+
		"            where <value> is:\n"+
		"                 \"string\" for a string field\n"+
		"                 a number such as 2147483647 for an integer field\n"+
		"    -p,  --password <string>\n"+
		"    -u,  --user <string>\n"+
		"    -s,  --secondary <string>\n"+
		"    -t,  --trace <level>\n"+
		"            where <level> can be:\n"+
		"                  off, severe, warn, info, debug, verbose\n"+
		"         --trustall\n"+
		"         --trustfile <file>\n"+
		"    -us  --unsubscribe \n"+
		"            the durable name needs to be specified as well\n"+
		"    -x,  --explicitAck \n")
}

func parseArgs() error {
	flag.StringVar(&user, "user", "", "Username")
	flag.StringVar(&user, "u", "", "Username")

	flag.StringVar(&password, "password", "", "Password")
	flag.StringVar(&password, "p", "", "Password")

	flag.StringVar(&identifier, "identifier", "", "Identifier")
	flag.StringVar(&identifier, "id", "", "Identifier")

	flag.StringVar(&appName, "application", "tibrecv", "App Name")
	flag.StringVar(&appName, "a", "tibrecv", "App Name")

	flag.StringVar(&subEndpoint, "tibrecv-endpoint", "tibrecv-endpoint", "Endpoint Name")
	flag.StringVar(&subEndpoint, "e", "tibrecv-endpoint", "Endpoint Name")

	flag.StringVar(&durableName, "durableName", "", "Durable Name")
	flag.StringVar(&durableName, "d", "", "Durable Name")

	flag.Int64Var(&count, "count", 10, "Send and Receive N messages")
	flag.Int64Var(&count, "c", 10, "Send and Receive N messages")

	flag.StringVar(&clientLabel, "clientLabel", "", "client label")

	flag.BoolVar(&trustAll, "trustall", false, "trustall")

	flag.StringVar(&trustFile, "trustfile", "", "trust file")

	flag.StringVar(&match, "match", "", "where <content match> is of the form: \n"+
		"		'{\"fieldname\":<value>}'\n"+
		"	where <value> is:\n"+
		"		\"string\" for a string field\n"+
		" 		a number such as 2147483647 for an integer field")
	flag.StringVar(&match, "m", "", "Content matcher")

	flag.StringVar(&secondary, "secondary", "", "Secondary Realmserver URL")
	flag.StringVar(&secondary, "s", "", "Secondary Realmserver URL")

	flag.StringVar(&trcLevel, "trace", "", "Trace level")
	flag.StringVar(&trcLevel, "t", "", "Trace level")

	flag.BoolVar(&explicitAck, "explicitAck", false, "Use explicit acks")
	flag.BoolVar(&explicitAck, "x", false, "Use explicit acks")

	flag.BoolVar(&unsubscribe, "unsubscribe", false, "unsubscribe the durable name needs to be specified as well")
	flag.BoolVar(&unsubscribe, "us", false, "unsubscribe the durable name needs to be specified as well")

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

func processNotifications(realm ftl.Realm) {
	notificationsPipe := make(chan ftl.Notification)
	realm.SetNotificatonChannel(notificationsPipe)

	for notification := range notificationsPipe {
		if notification.Notify == ftl.NotificationClientDisabled {
			log.Println("application administratively disabled: " + notification.Reason)
			doneReceiving = true
		} else {
			log.Printf("Notification type %d : %s", notification.Notify, notification.Reason)
			doneReceiving = true
		}
	}
}

func messagesReceived(msgs ftl.MsgBlock) {
	numMsgs := len(msgs)
	for i := 0; i < numMsgs; i++ {
		received++
		log.Printf("Received message [%d]: ", received)
		log.Println(msgs[i].String())
		if explicitAck {
			ackableMsgs = append(ackableMsgs, msgs[i])
		}
	}

	if received == count {
		doneReceiving = true
	}
}

func main() {
	// print the banner
	log.Println("--------------------------------------------------------------")
	log.Println("TibRecvEx")
	log.Println(ftl.Version())
	log.Println("--------------------------------------------------------------")
	recv()
}

func recv() int {
	parseArgs()

	if trcLevel != "" {
		ftl.SetLogLevel(trcLevel)
	}

	realmProps := make(ftl.Props)
	if user != "" {
		realmProps[ftl.RealmPropertyStringUsername] = user
		realmProps[ftl.RealmPropertyStringUserpassword] = password
	}
	if identifier != "" {
		realmProps[ftl.RealmPropertyStringAppinstanceIdentifier] = identifier
	}

	if secondary != "" {
		realmProps[ftl.RealmPropertyStringSecondaryServer] = secondary
	}

	if trustFile != "" {
		realmProps[ftl.RealmPropertyLongTrustType] = ftl.RealmHTTPSConnectionUseSpecifiedTrustFile
		realmProps[ftl.RealmPropertyStringTrustFile] = trustFile
	} else if trustAll {
		realmProps[ftl.RealmPropertyLongTrustType] = ftl.RealmHTTPSConnectionTrustEveryone
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

	queueProps := make(ftl.Props)
	queueProps[ftl.EventqueuePropertyIntDiscardPolicy] = discardPolicy
	queueProps[ftl.EventqueuePropertyIntDiscardPolicyMaxEvents] = maxEvents
	errorchan := make(chan error)
	go func() {
		for problem := range errorchan {
			log.Println(ftl.ErrStack(problem))
			break
		}
	}()
	queue, err = realm.NewQueue(errorchan, queueProps)
	if err != nil {
		log.Fatal(ftl.ErrStack(err))
	}
	defer queue.Destroy()

	subProps := make(ftl.Props)
	if durableName != "" {
		subProps[ftl.SubscriberPropertyStringDurableName] = durableName
	}
	if explicitAck {
		subProps[ftl.SubscriberPropertyBoolExplicitAck] = explicitAck
	}

	subPipe := make(chan ftl.MsgBlock)
	sub, err = queue.SubscribeBlocks(subEndpoint, match, subProps, subPipe)
	if err != nil {
		log.Println(err.Error())
	}
	defer queue.CloseSubscription(sub)

	if !noAdvisory {
		go watchAdvisories(queue)
	}

	fmt.Println("Waiting for message(s)\n")

	for msgs := range subPipe {
		if doneReceiving {
			break
		}
		messagesReceived(msgs)
		size := len(ackableMsgs)
		var i int = 0
		if size > 0 {
			ackableMsgs[i].Acknowledge()
			ackableMsgs[i].Destroy()
			i++
		}
		if doneReceiving {
			break
		}
	}

	if durableName != "" && unsubscribe {
		realm.Unsubscribe(subEndpoint, durableName)
	}

	return 0
}

func watchAdvisories(queue ftl.Queue) {
	advPipe = make(chan ftl.Message)
	am := "{\"" + ftl.AdvisoryFieldName + "\":\"" + ftl.AdvisoryNameDataloss + "\"}"
	advSub, err := queue.Subscribe(subEndpoint, am, nil, advPipe)
	if err != nil {
		log.Println(err.Error())
	}
	defer queue.CloseSubscription(advSub)
	defer advSub.Close()

	for advisory := range advPipe {
		if doneReceiving {
			break
		}
		fmt.Println(advisory)
		log.Println(ftl.AdvisoryFieldName)
		log.Println(ftl.AdvisoryFieldSeverity)
		log.Println(ftl.AdvisoryFieldModule)
		log.Println(ftl.AdvisoryFieldReason)
		log.Println(ftl.AdvisoryFieldTimestamp)
		log.Println(ftl.AdvisoryFieldAggregationCount)
		log.Println(ftl.AdvisoryFieldAggregationTime)
	}
	return
}
