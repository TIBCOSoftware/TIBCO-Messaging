/*
 * Copyright (c) 2013-$Date: 2018-11-10 19:06:28 -0600 (Sat, 10 Nov 2018) $ TIBCO Software Inc.
 * Licensed under a BSD-style license. Refer to [LICENSE]
 * For more information, please contact:
 * TIBCO Software Inc., Palo Alto, California, USA
 */

 This directory contains the Go client for TIBCO eFTL and examples.

 Examples located in this directory are simple examples of basic
 TIBCO eFTL functionality.


 Documentation
 ---------------------------------------------

 Godoc can be used to serve up the Go client for TIBCO eFTL
 documentation:

 1. Add the directory containing the Go client for TIBCO eFTL
    and examples to the GOPATH environment variable.

 2. Run godoc as a web server, for example:

      $ godoc -http=:6060

 3. Navigate your browser to the Go client for TIBCO eFTL
    package:

      http://localhost:6060/pkg/tibco.com/eftl


 Building 
 ---------------------------------------------

 To build the Go client for TIBCO eFTL: 

 1. The Go client for TIBCO eFTL is dependent on a WebSocket package. 
    Download and install the package dependency:

      $ go get github.com/gorilla/websocket

 2. Build the Go client for TIBCO eFTL and examples:

      $ go install tibco.com/eftl...

