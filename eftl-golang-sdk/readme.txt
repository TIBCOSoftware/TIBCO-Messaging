/*
 * Copyright (c) 2013-$Date: 2018-11-10 19:06:28 -0600 (Sat, 10 Nov 2018) $ Cloud Software Group, Inc.
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

   NOTE: Before running godoc make sure be in <TIBCO eFTL Install Directory>/samples/golang/src/tibco.com/eftl directory

 3. Navigate your browser to the Go client for TIBCO eFTL
    package:

      http://localhost:6060/pkg/tibco.com/eftl


 Building 
 ---------------------------------------------

 To build the Go client for TIBCO eFTL and examples:
 
 1. Navigate to the eFTL samples directory
    (e.g. on linux, 'cd <eftl_install_dir>/samples/golang')

 2. Set GOBIN 
    (e.g. on linux,  'export GOBIN=<eftl_install_dir>/samples/golang')


 3. Navigate to the eftl samples golang src directory and 
    build the Go client for TIBCO eFTL and examples:
    (e.g. on linux, 'cd <eftl_install_dir>/samples/golang/src/tibco.com/eftl')

      $ go install ./...

 4. Run the samples in the GOBIN directory simply by executing: 

      $ subscriber
      $ publisher


 Examples
---------------------------------------------

 Once built the examples can be found in the /bin directory.

 publisher

        Basic publisher program that demonstrates the use of 
        publishing eFTL messages.

 subscriber

        Basic subscriber program that demonstrates the use of 
        subscribing to eFTL messages.

 stopSubscriber

        Basic subscriber program that demonstrates the use of
        stopping and starting a subscription.

 sharedSubscriber

        Basic subscriber program that demonstrates the use of 
        shared durable subscriptions.

 lastValueSubscriber

        Basic subscriber program that demonstrates the use of 
        last-value durable subscriptions.

 kvset

        Basic key-value program that demonstrates the use of
        setting a key-value pair in a map.

 kvget

        Basic key-value program that demonstrates the use of
        getting a key-value pair from a map.

 kvremove

        Basic key-value program that demonstrates the use of
        removing a key-value pair from a map.
