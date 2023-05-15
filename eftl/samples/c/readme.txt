/*
 * Copyright (c) 2010-2023 $ Cloud Software Group Inc.
 * Licensed under a BSD-style license. Refer to [LICENSE]
 */

 This directory contains C client source and C client samples
 for TIBCO eFTL.

 Samples located in this directory are simple examples of basic
 TIBCO eFTL functionality.


 Compiling and running samples.
 ---------------------------------------------

 In order to compile and run samples you need to execute
 the setup.bat (Windows) or setup (Unix) script located in
 this directory's parent directory.

 To compile and run samples, do the following steps:

 1. Open a console window and change directory to the samples
    subdirectory of your TIBCO eFTL installation.

 2. Run the "setup" script.

 3. Change directory to the samples/c subdirectory.

 4. To compile the samples, execute:

        Windows: nmake -f Makefile.Windows
        Linux:   make -f Makefile.Linux
        Mac OS:  make -f Makefile.Darwin

  5. Now run the samples by executing:

        tibeftlsub
        tibeftlpub


 Samples description.
 ---------------------------------------------

 tibeftlpub

    Basic publisher program that demonstrates the use of 
    publishing eFTL messages.

 tibeftlsub

    Basic subscriber program that demonstrates the use of 
    subscribing to eFTL messages.

 tibeftlstopsub

    Basic subscriber program that demonstrates the use of
    stopping and starting a subscription.

 tibeftlrequest

    Basic request program that demonstrates the use of
    sending a request and waiting for a reply. 

    This program can receive replies from other eFTL
    clients, from FTL clients, and from EMS clients
    using the JMS ReplyTo and CorrelationID headers.

 tibeftlreply

    Basic reply program that demonstrates the use of
    sending a reply in response to a request.

    This program can send replies to other eFTL
    clients, to FTL clients, and to EMS clients
    using the JMS ReplyTo and CorrelationID headers.

 tibeftlshared

    Basic subscriber program that demonstrates the use of
    shared durable subscriptions.

 tibeftllastvalue

    Basic subscriber program that demonstrates the use of
    last-value durable subscriptions.

 tibeftlkvset

    Basic key-value program that demonstrates the use of
    setting a key-value pair in a map.

 tibeftlkvget

    Basic key-value program that demonstrates the use of
    getting a key-value pair from a map.

 tibeftlkvremove

    Basic key-value program that demonstrates the use of
    removing a key-value pair from a map.

