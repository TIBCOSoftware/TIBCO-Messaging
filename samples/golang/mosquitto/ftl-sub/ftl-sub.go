package main

import (
	"encoding/json"
	"flag"
	"fmt"
	"log"
	"net/http"
	"os"
	"path/filepath"
	"strconv"

	_ "net/http/pprof"

	"tibco.com/ftl"
)

var (
	user          string = "guest"
	password      string = "guest-pw"
	host          string = "http://localhost"
	port          string = "8080"
	serverAddress string = host + ":" + port
	count         int    = 1
	clientCount   int    = 1
	cafile        string = ""
	trustall      bool   = false

	appName       string = "tibfmbridge"
	endpointName  string = "tibfmbridge_client_endpoint"
	clientLabel   string = "ftl-sub"
	trcLevel      string = ""
	formatName    string = ""
	match         string = ""
	durableName   string = ""
	explicitAck   bool   = false
	ackableMsgs   []ftl.Message
	discardPolicy int = ftl.EventQueueDiscardPolicyNew
	maxEvents     int = 1000
	realm         ftl.Realm
	received      int    = 0
	doneReceiving bool   = false
	unsubscribe   bool   = false
	size          int    = -1
	help          bool   = false
	pprof         string = ""
	quiet         bool   = false
)

func parseArgs() error {
	flag.Usage = func() {
		usage := filepath.Base(os.Args[0]) + " [options]\n\ta sample FTL subscriber"
		fmt.Printf("\nUsage: %s\n\n", usage)
		fmt.Println("Options:")
		flag.PrintDefaults()
	}

	flag.StringVar(&user, "u", user, "Username")
	flag.StringVar(&user, "user", user, "Username")
	flag.StringVar(&password, "pw", password, "Password")
	flag.StringVar(&password, "password", password, "Password")
	flag.StringVar(&host, "h", host, "Realmserver host")
	flag.StringVar(&host, "host", host, "Realmserver host")
	flag.StringVar(&port, "p", port, "Realmserver port")
	flag.StringVar(&port, "port", port, "Realmserver port")
	flag.IntVar(&count, "c", count, "Total number of messages to receive. 0 means endless.")
	flag.IntVar(&count, "count", count, "Total number of messages to receive. 0 means endless.")
	flag.StringVar(&cafile, "cafile", cafile, "Realmserver certificate file")
	flag.BoolVar(&trustall, "trustall", trustall, "Trust all")

	flag.StringVar(&appName, "application", appName, "App Name")
	flag.StringVar(&appName, "a", appName, "App Name")
	flag.StringVar(&endpointName, "endpoint", endpointName, "Endpoint Name")
	flag.StringVar(&endpointName, "e", endpointName, "Endpoint Name")
	flag.StringVar(&match, "match", "", "where <content match> is of the form: \n"+
		"		'{\"fieldname\":<value>}'\n"+
		"	where <value> is:\n"+
		"		\"string\" for a string field\n"+
		" 		a number such as 2147483647 for an integer field")

	flag.BoolVar(&quiet, "q", quiet, "Only print the number of messages received when exiting.")
	flag.BoolVar(&quiet, "quiet", quiet, "Only print the number of messages received when exiting.")
	flag.StringVar(&trcLevel, "trace", trcLevel, "where <level> can be: off, severe, warn, info, debug, verbose")

	flag.Parse()

	pprof = os.Getenv("TIB_BKR_PROFILING")
	serverAddress = host + ":" + port
	return nil
}

func main() {
	parseArgs()
	if trcLevel != "" {
		ftl.SetLogLevel(trcLevel)
	}
	recv()
}

func recv() int {

	realmProps := make(ftl.Props)
	if user != "" {
		realmProps[ftl.RealmPropertyStringUsername] = user
		realmProps[ftl.RealmPropertyStringUserpassword] = password
	}

	if cafile != "" {
		realmProps[ftl.RealmPropertyLongTrustType] = ftl.RealmHTTPSConnectionUseSpecifiedTrustFile
		realmProps[ftl.RealmPropertyStringTrustFile] = cafile
	} else if trustall {
		realmProps[ftl.RealmPropertyLongTrustType] = ftl.RealmHTTPSConnectionTrustEveryone
	}

	if clientLabel != "" {
		realmProps[ftl.RealmPropertyStringClientLabel] = clientLabel
	}

	// Connect to the realm server
	realm, err := ftl.Connect(serverAddress, appName, realmProps)
	if err != nil {
		log.Fatal(ftl.ErrStack(err))
	}
	defer realm.Close()

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
	queue, err := realm.NewQueue(errorchan, queueProps)
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
	_, err = queue.SubscribeBlocks(endpointName, match, subProps, subPipe)
	if err != nil {
		log.Println(err.Error())
	}

	if pprof != "" {
		go func() {
			http.ListenAndServe(pprof, nil)
			os.Exit(1)
		}()
	}

	fmt.Println("Waiting for message(s)")
	for msgs := range subPipe {
		numMsgs := len(msgs)
		if explicitAck {
			ackableMsgs = make([]ftl.Message, 0, numMsgs)
		}
		for i := 0; i < numMsgs; i++ {
			received++
			if explicitAck {
				ackableMsgs = append(ackableMsgs, msgs[i])
			}
			if !quiet {
				content, _ := msgs[i].Get()
				strcontent := make(map[string]interface{})
				for k, v := range content {
					if k == "_mqtt_pay" {
						data, _ := v.([]byte)
						strcontent[k] = string(data)
					} else {
						strcontent[k] = v
					}
				}
				data, _ := json.Marshal(strcontent)
				fmt.Println(string(data))
			}
		}
		for _, ack := range ackableMsgs {
			ack.Acknowledge()
			ack.Destroy()
		}
		if received >= count {
			fmt.Println(strconv.Itoa(received))
			break
		}
	}
	if durableName != "" && unsubscribe {
		realm.Unsubscribe(endpointName, durableName)
	}
	return 0
}
