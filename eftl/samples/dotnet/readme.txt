/*
 * @COPYRIGHT_BANNER@
 */

 This directory contains C# client samples for TIBCO eFTL.

 Samples located in this directory are simple examples of basic
 eFTL functionality.


 Compiling and running samples.
 ---------------------------------------------

 In order to compile and run samples you need to execute
 the setup.bat script located in this directory's parent directory.

 To compile and run samples, do the following steps:

 1. Open a console window and change directory to the samples
    subdirectory of your TIBCO eFTL installation.

 2. Run the "setup" script.

 3. Change directory to the samples/dotnet subdirectory.

 4. Execute:

      Windows:
         nmake -f Makefile.Windows (builds .NET framework and .NET core apps)
      Linux:
         make -f Makefile.Linux  (builds only .NET core apps)
      MacOS
         make -f Makefile.Darwin (builds only .NET core apps)

    This command compiles the samples. 

.NET Framework (Windows Only)
--------------
 5. Now run the samples build for .NET framework by executing:

      net472\Subscriber.exe
      net472\Publisher.exe


.NET core (Linux, Windows, MacOS)
------------

 6. In order to the run the samples build for .NET core execute:

      Linux and Mac:
        dotnet netcoreapp6.0/Subscriber.dll
        dotnet netcoreapp6.0/Publisher.dll
      Windows:
        dotnet netcoreapp6.0\Subscriber.dll
        dotnet netcoreapp6.0\Publisher.dll
      
 NOTE: Connecting to a secure tibeftlServer (via the wss:// protocol) requires 
 that the .NET client install the server trust certificate in the Microsoft 
 Certificate store under "Trusted Root Certification Authorities".

 7. In order to build individual apps execute:
      
     Linux/Mac/Windows: 
     -----------------
      
     dotnet build --configuration Release -f netcoreapp6.0 Publisher.csproj (to build Publisher app for .NET core)
     
     Windows only:
     ------------

     dotnet build --configuration Release -f net472 Publisher.csproj (to build Publisher app for .NET Framework)
     
Samples description.
---------------------------------------------

 Publisher

      Basic publisher program that demonstrates the use of
      publishing eFTL messages.

 Subscriber

      Basic subscriber program that demonstrates the use of
      subscribing to eFTL messages.

 StopSubscriber

      Basic subscriber program that demonstrates the use of
      stopping and starting a subscription.

 SharedDurable

      Basic subscriber program that demonstrates the use of
      shared durable subscriptions.

 LastValueDurable

      Basic subscriber program that demonstrates the use of
      last-value durable subscriptions.

 Request

      Basic request program that demonstrates the use of
      sending a request and receiving a reply.

 Reply

      Basic reply program that demonstrates the use of
      sending a reply in response to a request.

 KVSet

      Basic key-value program that demonstrates the use of
      setting a key-value pair in a map.

 KVGet

      Basic key-value program that demonstrates the use of
      getting a key-value pair from a map.

 KVRemove

      Basic key-value program that demonstrates the use of
      removing a key-value pair from a map.
