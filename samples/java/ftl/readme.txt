/*
 * Copyright (c) 2010-2017 TIBCO Software Inc.
 * Licensed under a BSD-style license. Refer to [LICENSE]
 * For more information, please contact:
 * TIBCO Software Inc., Palo Alto, California, USA
 */

 This directory contains Java client samples for TIBCO FTL.

 Samples located in this directory are simple examples of basic
 FTL functionality.


 Compiling and running samples.
 ---------------------------------------------

 In order to compile and run samples you need to execute
 setup.bat (Windows) or setup (UNIX) script located in
 this directory's parent directory.

 To compile and run samples, do the following steps:

 1. Make sure your computer has Java 1.7 installed.

 2. Open console window and change directory to the samples
    subdirectory of your TIBCO FTL installation.

 3. run "setup" script.

 4. Change directory to the samples/src/java subdirectory.

 5. execute:

    javac -d . *.java

    This command compiles the samples 

 6. Now you can run samples simply by executing:

    On Linux and Mac OS X
    ---------------------

    java -Djava.library.path=<FTL install directory>/lib com.tibco.ftl.samples.<sample name>

    NOTE: On Mac OS X El Capitan (10.11) and later, java ignores the
    DYLD_LIBRARY_PATH setting.  This means that you must supply
    a java.library.path property.

    Alternatively on Linux you can set the LD_LIBRARY_PATH to 
    include <FTL install directory>/lib and then execute:
   
    java com.tibco.ftl.samples.<sample name>

    on Windows
    ---------

    java com.tibco.ftl.samples.<sample name>


Samples description.
---------------------------------------------

TibSend

    Basic sender program that demonstrates use of publisher using the 'default' application definition.

TibRecv

    Basic receiver program that demonstrates use of subscriber, simple matcher
    and event queue objects using the 'default' application definition.

TibSendEx

    Basic sender program that demonstrates use of publisher, timer,
    event queue objects and secure connection to realmserver.

TibRecvEx

    Basic receiver program that demonstrates use of subscriber, matcher,
    event queue objects and secure connection to realmserver.

TibRequest

    Basic program that sends a request and receives a reply on an
    inbox.

TibReply

    Basic program that replies to the TibRequest program.

TibLatSend

    Performance send program that measures latency.

TibLatRecv

    Performance program that works with TibLatSend.

TibThruSend

    Performance send program that measures throughput.

TibThruRecv

    Performance program that works with TibThruSend.

TibAux TibAuxStatRecord

    Utility functions for other samples.

TibStat

    Statistics functions for other samples.

TibGroup

    Basic program that joins a group and displays changes to the
    group membership.

TibMapSet

    Basic program that demonstrates the TibMap API.

TibMapGet

    Basic program that demonstrates the TibMap API.

TibMonSub

    Example monitoring endpoint subscriber.
