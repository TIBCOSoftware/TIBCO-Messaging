/*
 * Copyright (c) 2010-2017 TIBCO Software Inc.
 * Licensed under a BSD-style license. Refer to [LICENSE]
 * For more information, please contact:
 * TIBCO Software Inc., Palo Alto, California, USA
 */

package main

import (
	"flag"
	"log"
	"net/http"
	"tibco.com/ftl-sample/security"
)

func main() {
	cert := flag.String("cert", "./tls.crt", "cert file")
	key := flag.String("key", "./tls.key", "key file")
	listen := flag.String("url", ":8080", "Port to listen on")

	file := flag.String("file", "./users.txt", "permissions file")
	auth := flag.String("auth-group", "tibffauthsvc", "group used for basic http auth")
	refresh := flag.Int("refresh", 30, "permissions file refresh interval in seconds")
	flag.Parse()

	log.SetPrefix("Warning: ")

	srv := &http.Server{Addr: *listen}

	var err error
	var quit chan struct{}
	if err, srv.Handler, quit = security.SetupAuthService(*file, *refresh, *auth); err == nil {
		err = srv.ListenAndServeTLS(*cert, *key)
		close(quit)
	}
	log.Fatal(err)
}
