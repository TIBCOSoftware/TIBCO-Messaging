 =================================================================
 TIBCO Enterprise Message Service
 Copyright (c) 2001-$Date: 2017-03-27 16:01:48 -0500 (Mon, 27 Mar 2017) $ TIBCO Software Inc.
 =================================================================

 This directory contains simple examples of basic TIBCO Enterprise Message
 Service functionality.

 The main directory contains EMS client samples.

 The 'tibrv' subdirectory contains examples of how EMS clients using the
 TIBCO Enterprise Message Service implementation can interoperate with
 TIBCO Rendezvous applications.

 The 'JNDI' subdirectory contains client JNDI examples.

 The 'admin' subdirectory contains administration examples.


 Compiling and running samples.
 ---------------------------------------------

 In order to compile and run samples, you need to execute the
 setup.bat (Windows) or setup.sh (UNIX) script located in
 this directory. You may need to change the script to reflect
 your installation of the TIBCO Enterprise Message Service software.
 Please read the comments inside the script file. Normally, you
 don't need to change it if you installed TIBCO Enterprise Message Service
 into the default directory.

 To compile and run samples, proceed with the following steps:

 1. Verify the setting of TIBEMS_ROOT environment variable inside
    the setup.bat or setup.sh script file.

 2. Open a console window and change directory to the samples/java
    subdirectory of your TIBCO Enterprise Message Service installation.

 3. run "setup" script.

 4. execute:

    javac -d . *.java

    This command compiles the samples, except for those located in
    subdirectories. For subdirectories, execute the same command there.

 5. Make sure the TIBCO Enterprise Message Service server (tibemsd) is running

 6. Now you can run samples simply by executing:

    java <sample name>

    Some samples require mandatory parameters.

    If the tibemsd server is running on a different computer you should use
    the -server parameter when running samples. For example, if the
    tibemsd server is running on computer 'mainhost', you should run
    the samples using the following command:

    java tibjmsMsgConsumer -server mainhost:7222 -topic "test.topic"


 In order to run samples in the 'tibrv' sudirectory, you should perform
 the following:

 1. Make sure TIBCO Rendezvous software is installed on your computer.

 2. Stop your tibemsd server.

 3. Change the tibemsd.conf sample configuration to enable TIBCO Rendezvous
    transports.

 4. Restart your tibemsd server

 5. Navigate into samples/java directory

 6. Run the setup script

 7. execute: javac -d . tibrv/*.java
       - this assumes the TIBCO Rendezvous jar file tibrvj.jar
         exists in your CLASSPATH if you have installed
         TIBCO Rendezvous software.

 8. Run samples as any other sample in the java directory.


 All samples, except for those in the 'JNDI' subdirectory,
 accept the '-server' command line parameter which specifies the url
 of the running instance of the TIBCO Enterprise Message Service server.
 By default, all samples try to connect to the server running on the local
 computer with default port of 7222. The server url is usually specified in
 the form '-server "tcp://hostname:port"'

 All samples, except for those in the 'tibrv' subdirectory and
 JNDI/tibjmsJNDIRead, accept parameters -user and -password. You may need
 to use these parameters when running samples against a server with
 authorization enabled.
 
 Note that some examples, in particular examples in the 'tibrv'
 subdirectory, may require changes to your TIBCO Enterprise Message Service
 server configuration. Please read the comment section in the sample
 programs.

 Examples located in the 'tibrv' subdirectory require that you have
 TIBCO Rendezvous software installed on your computer and you
 must enable TIBCO Rendezvous transports in TIBCO Enterprise Message Service
 server configuration file. Also, these examples require configuring topic
 entries as exported and imported. Please read comments inside the samples
 for more information. Example of topics configuration file located in
 samples/config directory defines topics required to run these samples.

 Many samples in the main directory and in the 'tibrv' subdirectory
 require the use of sample configuration files distributed
 with TIBCO Enterprise Message Service software. TIBCO Rendezvous transports
 also need to be enabled within the main server configuration file
 for samples in the 'tibrv' directory to work correctly.

 All JNDI samples, with the exception of JNDI/tibjmsJNDIRead,
 accept the '-provider' command line parameter, which specifies the url of the
 JNDI provider of the running instance of TIBCO EMS server. By default,
 all JNDI samples try to connect to the JNDI provider running on the local
 computer with default port of 7222. The provider url is usually specified in
 the form '-provider "tibjmsnaming://hostname:port"'
