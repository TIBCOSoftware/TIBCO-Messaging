 =================================================================
 Copyright (c) 2017-$Date: 2017-03-27 16:01:48 -0500 (Mon, 27 Mar 2017) $ TIBCO Software Inc. 
 ALL RIGHTS RESERVED

 $Id: readme.txt 92619 2017-03-27 21:01:48Z olivierh $
 =================================================================

 This directory contains .NET administration samples for TIBCO EMS.
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

    csServer 

    If tibemsd server is running on a different computer, use
    the -server parameter when running samples; for example, if the
    tibemsd server is running on a computer named mainhost, then run
    the samples using the following command:

    csServer -server mainhost:7222 


 Shared Assemblies:
 ----------------
 The current Makefile copies the TIBCO EMS .NET admin and client
 libraries into the sample executables directory.  This is required for
 the .NET admin samples to run properly.  In fact, since the libraries
 are not registered in the Global Assembly Cache (GAC), they must be in
 the same directory as any executable that uses them. However, if you
 prefer to make the libraries shared assemblies (available for all
 applications on the local machine) you can do so by registering
 TIBCO.EMS.ADMIN.dll and TIBCO.EMS.dll in the GAC.  Once libraries are
 installed in the GAC, they are available for all applications on the
 machine.  

 Two files (register.bat and unregister.bat) have been provided in
 this sample directory to perform the installation/uninstallation of
 the assemblies into/from GAC. Users should verify that "gacutil.exe"
 is installed on their computer and the batch files contain the correct
 path to gacutil.exe before running. The gacutil.exe is usually located
 in "C:\Program Files\Microsoft Visual Studio .NET\FrameworkSDK\bin\".

 To install the assemblies in the GAC using the batch file provided:

 . Open a console window in the current directory, execute:
   
   register.bat

 To uninstall the assemblies from GAC using the batch file provided:

 . Open a console window in the current directory, execute:
   
   unregister.bat

 To install the assemblies into GAC manually:

 1. Verify that "gacutil.exe" is installed.  It is usually located
    in C:\Program Files\Microsoft Visual Studio .NET\FrameworkSDK\bin\

 2. Open a console window, execute:

    <directory containing gacutils.exe>\gacutil.exe /i <TIBCO EMS
    library directory>\TIBCO.EMS.dll
    <directory containing gacutils.exe>\gacutil.exe /i <TIBCO EMS
    library directory>\TIBCO.EMS.ADMIN.dll

 To uninstall the assemblies into GAC manually:

 1. Verify that "gacutil.exe" is installed.  It is usually located
    in C:\Program Files\Microsoft Visual Studio .NET\FrameworkSDK\bin

 2. Open a console window, execute:

    <directory containing gacutils.exe>\gacutil.exe /u TIBCO.EMS
    <directory containing gacutils.exe>\gacutil.exe /u TIBCO.EMS.ADMIN


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
