/*
 * Copyright (c) 2010-2017 TIBCO Software Inc.
 * Licensed under a BSD-style license. Refer to [LICENSE]
 * For more information, please contact:
 * TIBCO Software Inc., Palo Alto, California, USA
 */

 This directory contains C client samples for TIBCO FTL.

 Samples located in this directory are simple examples of basic
 FTL functionality.


 Compiling and running samples.
 ---------------------------------------------

 The TIBFTL_DIR environment variable is set based on the assumption
 that you are building directly in the samples/src/c directory.
 If this is not true, please modify the Makefile setting
 of TIBFTL_DIR to reflect where the header files and libraries are
 located.

 To compile and run samples, do the following steps:

 1. Open a new terminal or console window.  Change directory to the
    samples/src/c subdirectory of your TIBCO FTL installation.

 2. execute:

      make   (or nmake on Windows platforms)

 3. Now you can run samples by executing their names, for example:

      tibsend

    Note that this depends on the shared libraries so you must put the
    lib directory in your "*PATH*" (specific to the platform) to find
    the shared libraries in the lib and samples/lib directories.


Samples description.
---------------------------------------------

tibsend

    Basic sender program that demonstrates use of publisher using the 'default' application definition.

tibrecv

    Basic receiver program that demonstrates use of subscriber, matcher
    and event queue objects using the 'default' application definition.

tibsendex

    Basic sender program that demonstrates use of publisher, timer,
    event queue objects and secure connection to realmserver.

tibrecvex

    Basic receiver program that demonstrates use of subscriber, matcher,
    event queue objects and secure connection to realmserver.

tibrequest

    Basic program that sends a request and receives a reply on an
    inbox.

tibreply

    Basic program that replies to the tibrequest program.

tiblatsend

    Performance send program that measures latency.

tiblatrecv

    Performance program that works with tiblatsend.

tibthrusend

    Performance send program that measures throughput.

tibthrurecv

    Performance program that works with tibthrusend.

tibthrusendd

    Performance send program that measures throughput, specialized for shared durables.

tibthrurecvd

    Performance program that works with tibthrusendd.

tibaux

    Utility functions for other samples.

tibgrp

    Basic program that joins a group and displays changes to the
    group membership.

tibmapset

    Basic program that demonstrates the TibMap API.

tibmapget

    Basic program that demonstrates the TibMap API.

directlatsend

    Performance send program that measures latency.

directlatrecv

    Performance program that works with directlatsend.

tibmonsub

    Example monitoring endpoint subscriber.
