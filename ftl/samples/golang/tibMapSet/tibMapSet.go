package main

import (
	"flag"
	"fmt"
	"log"
	"strings"
	"os"

	"tibco.com/ftl"
)

var (
	appName          string = "tibmapset"
	realmServer      string = "http://localhost:8080"
	mapEndpoint      string = "tibmapset-endpoint"
	formatName       string = "Format-1"
	trcLevel         string
	user             string = "guest"
	password         string = "guest-pw"
	identifier       string
	secondary        string
	mapName          string = "tibmap"
	defaultUrl       string = "http://localhost:8080"
	defaultSecureUrl string = "https://localhost:8080"
	keyList          string = "key1,key2,key3"
	clientLabel      string
	lockName         string = "lock-tibMapSet"

	done  bool = false
	realm ftl.Realm
	tmap  ftl.Map
	msg   ftl.Message

	count     int    = 0
	trustAll  bool   = false
	trustFile string = ""
	help      bool   = false
)

var Usage = func() {
	fmt.Fprintf(os.Stderr, "Usage:  tibMapSet [options] <realmserver_url>\n")
	fmt.Fprintf(os.Stderr, "Default url is %s\n", realmServer)
	fmt.Fprintf(os.Stderr, "    -a,  --application <name>\n"+
		"    -c,  --client-label <label>\n"+
		"    -e,  --endpoint <name>\n"+
		"    -f,  --format <name>\n"+
		"    -h,  --help\n"+
		"    -id, --identifier <name>\n"+
		"    -l,  --keyList <string>[,<string>]\n"+
		"    -n,  --mapname <name>\n"+
		"    -p,  --password <string>\n"+
		"    -u,  --user <string>\n"+
		"    -s,  --secondary <string>\n"+
		"    -t,  --trace <level>\n"+
		"            where <level> can be:\n"+
		"                  off, severe, warn, info, debug, verbose\n"+
		"         --trustall\n"+
		"         --trustfile <file>\n")
}

func parseArgs() {
	flag.StringVar(&appName, "a", "tibmapset", "Application Name")
	flag.StringVar(&appName, "application", "tibmapset", "Application Name")

	flag.StringVar(&clientLabel, "client-label", "", "client label")
	flag.StringVar(&clientLabel, "cl", "", "client label")

	flag.IntVar(&count, "count", 0, "count")
	flag.IntVar(&count, "c", 0, "count")

	flag.StringVar(&mapEndpoint, "endpoint", "tibmapset-endpoint", "Endpoint Name")
	flag.StringVar(&mapEndpoint, "e", "tibmapset-endpoint", "Endpoint Name")

	flag.StringVar(&formatName, "format", "Format-1", "Format Name")
	flag.StringVar(&formatName, "f", "Format-1", "Format Name")

	flag.StringVar(&identifier, "id", "", "Identifier")
	flag.StringVar(&identifier, "identifier", "", "Identifier")

	flag.StringVar(&keyList, "keyList", "key1,key2,key3", "Key List")
	flag.StringVar(&keyList, "l", "key1,key2,key3", "Key List")

	flag.StringVar(&mapName, "mapname", "tibmap", "Map Name")
	flag.StringVar(&mapName, "n", "tibmap", "Map Name")

	flag.StringVar(&password, "p", "", "Password for realm server")
	flag.StringVar(&password, "password", "", "Password for realm server")

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
	} else {
		realmServer = defaultUrl
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

	tmap, _ := realm.NewMap(mapEndpoint, mapName, nil)
	if err != nil {
		log.Fatal(ftl.ErrStack(err))
	}
	defer tmap.Close()

	lock, err := realm.NewLock(lockName, nil)
	if err != nil {
		log.Fatal(ftl.ErrStack(err))
	}

	msg, err = realm.NewMessage(formatName)
	if err != nil {
		log.Fatal(ftl.ErrStack(err))
	}

	if keyList != "" {
		keys := strings.Split(keyList, ",")
		for _, key := range keys {
			fmt.Println(key)
			go initMessage(key)
			err := tmap.Set(key, msg, lock)
			if err != nil {
				log.Fatal(ftl.ErrStack(err))
			}
			tmap.Set(key, msg, lock)
		}
	}

	return 0
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

func initMessage(key string) {
	content := make(ftl.MsgContent)
	count = count + 1
	content["My-Long"] = count
	msg.Set(content)
}

func main() {
	// print the banner
	log.Println("--------------------------------------------------------------")
	log.Println("TibMapSet")
	log.Println(ftl.Version())
	log.Println("--------------------------------------------------------------")
	parseArgs()
	work()
}
