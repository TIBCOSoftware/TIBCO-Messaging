 =================================================================
 TIBCO Enterprise Message Service
 Copyright (c) 2001-$Date: 2013-04-12 15:37:01 -0500 (Fri, 12 Apr 2013) $ TIBCO Software Inc.
 =================================================================

 This directory contains C client samples for TIBCO Enterprise Message Service.

 Samples located in this directory are simple examples of basic
 EMS functionality.


 Compiling and running samples.
 ---------------------------------------------

 The TIBEMS_DIR environment variable is set based on the assumption
 that you are building directly in the samples/c directory.
 If this is not true, please modify the Makefile setting
 of TIBEMS_DIR to reflect where the header files and libraries are
 located.

 To compile and run samples, do the following steps:

 1. Open a new terminal or console window.  Change directory to the
    samples/c subdirectory of your TIBCO Enterprise Message Service
    installation.

 2. execute:

      make   (or nmake on Windows platforms)

 3. Be sure that the TIBCO EMS server, tibemsd, is running.
    If not, start it by executing the command 'tibemsd' from the bin 
    directory. 

 4. Now you can run samples by executing their names, for example:

      tibemsMsgProducer    

    Note that this depends on the shared libraries so you must put the
    lib directory in your "*PATH*" (specific to the platform) to find
    the shared libraries.

    If tibemsd server is running on a different computer, use
    the -server parameter when running samples. For example, if the
    tibemsd server is running on a computer mainhost, then run
    the samples using the following command:

      tibemsMsgProducer -server mainhost:7222 -topic "test.topic"

 5. In order to build XA samples, you must supply xa.h, and build target
    'xa' with the command:

      make xa


 All samples accept the '-server' command line parameter, which
 specifies the url of the instance of TIBCO EMS server.

 By default, all samples try to connect to a local server running
 on the local computer using the default port number 7222. The server url
 is usually specified in the form '-server "tcp://hostname:port"'

 All sample programs accept the parameters '-user' and '-password'.
 You may need to use these parameters when running samples against a
 server on which authorization is enabled.

 Many samples in this directory require the use of sample configuration
 files distributed with TIBCO Enterprise Message Service software.
