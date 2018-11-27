package main

import (
	"flag"
	"fmt"
	"log"
	"os"
	"strings"

	"tibco.com/ftl"
)

var (
	appName            string = "tibmapget"
	realmServer        string = "http://localhost:8080"
	mapEndpoint        string = "tibmapget-endpoint"
	trcLevel           string
	user               string = "guest"
	password           string = "guest-pw"
	identifier         string
	secondary          string
	mapName            string = "tibmap"
	DEFAULT_URL        string = "http://localhost:8080"
	DEFAULT_SECURE_URL string = "https://localhost:8080"
	keyList            string
	clientLabel        string
	lockName           string = "lock-tibMapGet"

	done bool = false

	realm   ftl.Realm
	tibMap  ftl.Map
	msg     ftl.Message
	mapIter ftl.MapIterator

	count     int    = 0
	trustAll  bool   = false
	trustFile string = ""
	removeMap bool   = false
	help      bool   = false
)

var Usage = func() {
	fmt.Fprintf(os.Stderr, "Usage:  tibMapGet [options] <realmserver_url>\n")
	fmt.Fprintf(os.Stderr, "Default url is %s\n", realmServer)
	fmt.Fprintf(os.Stderr, "      -a,  --application <name>\n"+
		"         --client-label <label>\n"+
		"    -e,  --endpoint <name>\n"+
		"    -h,  --help\n"+
		"    -id, --identifier <name>\n"+
		"    -l,  --keyList <string>[,<string>]\n"+
		"            if not provided the entire key space will be iterated\n"+
		"    -n,  --mapname <name>\n"+
		"    -p,  --password <string>\n"+
		"    -u,  --user <string>\n"+
		"    -r   --removemap \n"+
		"            the mapname command line arg needs to be specified as well\n"+
		"    -s,  --secondary <string>\n"+
		"    -t,  --trace <level>\n"+
		"            where <level> can be:\n"+
		"                  off, severe, warn, info, debug, verbose\n"+
		"         --trustall\n"+
		"         --trustfile <file>\n")
}

func parseArgs() {
	flag.StringVar(&appName, "a", "tibmapget", "Application Name")
	flag.StringVar(&appName, "application", "tibmapget", "Application Name")

	flag.StringVar(&clientLabel, "client-label", "", "client label")
	flag.StringVar(&clientLabel, "cl", "", "client label")

	flag.StringVar(&mapEndpoint, "endpoint", "tibmapget-endpoint", "Endpoint Name")
	flag.StringVar(&mapEndpoint, "e", "tibmapget-endpoint", "Endpoint Name")

	flag.StringVar(&identifier, "id", "", "Identifier")
	flag.StringVar(&identifier, "identifier", "", "Identifier")

	flag.StringVar(&keyList, "keyList", "", "Key List")
	flag.StringVar(&keyList, "l", "", "Key List")

	flag.StringVar(&mapName, "mapname", "tibmap", "Map Name")
	flag.StringVar(&mapName, "n", "tibmap", "Map Name")

	flag.StringVar(&password, "p", "", "Password for realm server")
	flag.StringVar(&password, "password", "", "Password for realm server")

	flag.BoolVar(&removeMap, "removemap", false, "Remove Map")

	flag.StringVar(&secondary, "secondary", "", "Secondary")
	flag.StringVar(&secondary, "s", "", "Secondary")

	flag.StringVar(&trcLevel, "t", "", "Trace level")
	flag.StringVar(&trcLevel, "trace", "", "Trace level")

	flag.BoolVar(&trustAll, "trustall", false, "Trust all secure realm servers")

	flag.StringVar(&trustFile, "trustfile", "", "Trust realm servers with this certificate")

	flag.StringVar(&user, "u", "", "User name for realm server")
	flag.StringVar(&user, "user", "", "User name for realm server")

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
}

// go routine to handle notifications
func processNotifications(realm ftl.Realm) {
	notificationsPipe := make(chan ftl.Notification)
	realm.SetNotificatonChannel(notificationsPipe)

	for notification := range notificationsPipe {
		if notification.Notify == ftl.NotificationClientDisabled {
			log.Println("application administratively disabled: " + notification.Reason)
			done = true
		} else {
			log.Printf("Notification type is %d : %s", notification.Notify, notification.Reason)
			done = true
		}
	}
}

func work() int {
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

	go processNotifications(realm)

	tibMap, _ := realm.NewMap(mapEndpoint, mapName, nil)
	if err != nil {
		log.Fatal(ftl.ErrStack(err))
	}
	defer tibMap.Close()

	lock, err := realm.NewLock(lockName, nil)
	if err != nil {
		log.Fatal(ftl.ErrStack(err))
	}

	if keyList != "" {
		keys := strings.Split(keyList, ",")
		for _, key := range keys {
			fmt.Printf("getting value for key %s\n", key)
			msg, err = tibMap.Get(key, lock)
			if err != nil {
				log.Fatal(ftl.ErrStack(err))
			}
			msg.String()
			if msg.String() != "" {
				fmt.Printf("Key = %s, value = %s", key, msg.String())
			} else {
				fmt.Printf("Currently no key %s in the map", key)
			}
		}
	} else {
		fmt.Printf("Iterating keys is the map\n")

		mapIter, err := tibMap.NewIterator(lock, nil)
		if err != nil {
			log.Fatal(ftl.ErrStack(err))
		}
		defer mapIter.Destroy()

		for {
			iterNext, err := mapIter.Next()
			if err != nil {
				log.Fatal(ftl.ErrStack(err))
			}
			if iterNext == false {
				break
			}
			key, _ := mapIter.CurrentKey()
			msg, _ := mapIter.CurrentValue()
			if key != "" {
				fmt.Fprintf(os.Stderr, "Key: %s Value: %s\n", key, msg.String())
			}
		}
	}

	if mapName != "" && removeMap {
		fmt.Printf("Removing map %s", mapName)
		realm.RemoveMap(mapEndpoint, mapName, nil)
	}

	return 0
}

func main() {
	// print the banner
	log.Println("--------------------------------------------------------------")
	log.Println("TibMapGet")
	log.Println(ftl.Version())
	log.Println("--------------------------------------------------------------")
	parseArgs()
	work()
}
