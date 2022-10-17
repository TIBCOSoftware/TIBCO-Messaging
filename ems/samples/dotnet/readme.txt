 =================================================================
 Copyright (c) 2001-$Date: 2013-08-21 15:28:23 -0500 (Wed, 21 Aug 2013) $ by TIBCO Software Inc.
 ALL RIGHTS RESERVED

 $Id: readme.txt 68846 2013-08-21 20:28:23Z rkutter $
 =================================================================

 This directory contains .NET client samples for TIBCO EMS.
 In order to build and run .NET samples you need to install Microsoft .NET
 development environment, including the 'csc' compiler.

 Samples located in this directory are simple examples of basic
 EMS functionality.


 Compiling and running samples.
 ---------------------------------------------

 Requirements:
 To compile and run samples, the following requirements must be met:
 
 1. Microsoft .NET is installed.
 
 2. The 'csc' compiler is installed and is placed in your PATH.
    The command-line compiler csc.exe is included in the Microsoft .NET
    Framework.  The compiler executable can be usually found in:
    <Window directory>\Microsoft.NET\Framework\v<version>\
    The Microsft .NET Framework SDK can be downloaded free
    from: http://msdn.microsoft.com/downloads/
    

 To compile and run samples, do the following steps:

 1. Verify the setting of TIBEMS_ROOT variable inside the Makefile file,
    Set TIBEMS_ROOT to the root of your installation of TIBCO Enterprise 
    Message Service software.

 2. Open a console window and change directory to the samples/cs
    subdirectory of your TIBCO EMS installation.

 3. execute:

    nmake

 4. Make sure TIBCO EMS server (tibemsd) is running.

 5. Now you can run samples simply by executing their name such as:

    csMsgProducer -topic sample TEXT

    If tibemsd server is running on a different computer, use
    the -server parameter when running samples; for example, if the
    tibemsd server is running on a computer named mainhost, then run
    the samples using the following command:

    csMsgProducer -server mainhost:7222 -topic sample TEXT


 Shared Assembly:
 ----------------
 The current Makefile copies the TIBCO EMS .NET client library into the
 sample executables directory.  This is required for the .NET samples
 to run properly.  In fact, since the library is not registered in the
 Global Assembly Cache (GAC), it must be in the same directory as any
 executable that uses it. However, if you prefer to make the library a
 shared assembly (available for all applications on the local machine)
 you can do so by registering the TIBCO.EMS.dll in the GAC.  Once a
 library is installed in the GAC, it is available for all
 applications on the machine.  

 Two files (register.bat and unregister.bat) have been provided in
 this sample directory to perform the installation/uninstallation of
 the assembly into/from GAC.  Users should verify that "gacutil.exe"
 is installed on their computer and the batch files contain the correct
 path to gacutil.exe before running. The gacutil.exe is usually located
 in "C:\Program Files\Microsoft Visual Studio .NET\FrameworkSDK\bin\".

 To install the assembly in the GAC using the batch file provided:

 . Open a console window in the current directory, execute:
   
   register.bat

 To uninstall the assembly from GAC using the batch file provided:

 . Open a console window in the current directory, execute:
   
   unregister.bat

 To install the assembly into GAC manually:

 1. Verify that "gacutil.exe" is installed.  It is usually located
    in C:\Program Files\Microsoft Visual Studio .NET\FrameworkSDK\bin\

 2. Open a console window, execute:

    <directory containing gacutils.exe>\gacutil.exe /i <TIBCO EMS
    library directory>\TIBCO.EMS.dll

 To uninstall the assembly into GAC manually:

 1. Verify that "gacutil.exe" is installed.  It is usually located
    in C:\Program Files\Microsoft Visual Studio .NET\FrameworkSDK\bin

 2. Open a console window, execute:

    <directory containing gacutils.exe>\gacutil.exe /u TIBCO.EMS


 Samples Description:
 --------------------
 All samples accept the '-server' command line parameter which specifies
 the url of a running instance of the TIBCO EMS server (tibemsd).
 By default, all samples try to connect to the server running on the
 local computer with default port of 7222. The server url is usually
 specified in the form '-server "tcp://hostname:port"'.

 All samples accept the parameters -user and -password. You may need to
 use these parameters when running samples against a server with
 authorization enabled.
