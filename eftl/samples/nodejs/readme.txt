/*
 * @COPYRIGHT_BANNER@
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

 stop_subscriber.js

        Basic subscriber program that demonstrates the use of
        stopping and starting a subscription.

 shared_durable.js

        Basic subscriber program that demonstrates the use of
        shared durable subscriptions.

 last_value_durable.js

        Basic subscriber program that demonstrates the use of
        last-value durable subscriptions.

 request.js

        Basic subscriber program that demonstrates the use of
        sending a request and receiving a reply.

 reply.js

        Basic subscriber program that demonstrates the use of
        sending a reply in response to a request.

 kvset.js

        Basic key-value program that demonstrates the use of
        setting a key-value pair in a map.

 kvget.js

        Basic key-value program that demonstrates the use of
        getting a key-value pair from a map.

 kvremove.js

        Basic key-value program that demonstrates the use of
        removing a key-value pair from a map.

