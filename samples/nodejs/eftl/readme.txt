/*
 * Copyright (c) 2013-$Date: 2018-05-15 14:49:05 -0500 (Tue, 15 May 2018) $ TIBCO Software Inc.
 * Licensed under a BSD-style license. Refer to [LICENSE]
 * For more information, please contact:
 * TIBCO Software Inc., Palo Alto, California, USA
 */

 This directory contains Node.js client samples for TIBCO eFTL.

 Samples located in this directory are simple examples of basic
 TIBCO eFTL functionality.


 Running the Samples
 ---------------------------------------------

 When running with Node.js the JavaScript client for TIBCO eFTL
 requires that the ws package be installed:

      npm install ws

 Next copy the JavaScript client for TIBCO eFTL library, eftl.js,
 into the node_modules/ directory, and run:

      node subscriber.js
      node publisher.js


 Samples Description
 ---------------------------------------------

 publisher.js

        Basic publisher program that demonstrates the use of
        publishing eFTL messages.

 subscriber.js

        Basic subscriber program that demonstrates the use of
        subscribing to eFTL messages.

 shared_durable.js

        Basic subscriber program that demonstrates the use of
        shared durable subscriptions.

 last_value_durable.js

        Basic subscriber program that demonstrates the use of
        last-value durable subscriptions.

 kvset.js

        Basic key-value program that demonstrates the use of
        setting a key-value pair in a map.

 kvget.js

        Basic key-value program that demonstrates the use of
        getting a key-value pair from a map.

 kvremove.js

        Basic key-value program that demonstrates the use of
        removing a key-value pair from a map.

