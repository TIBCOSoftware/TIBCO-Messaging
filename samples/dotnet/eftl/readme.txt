/*
 * Copyright (c) 2009-$Date: 2018-09-04 17:57:51 -0500 (Tue, 04 Sep 2018) $ TIBCO Software Inc.
 * Licensed under a BSD-style license. Refer to [LICENSE]
 * For more information, please contact:
 * TIBCO Software Inc., Palo Alto, California, USA
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

      nmake

    This command compiles the samples. 

 5. Now run the samples by executing:

      Subscriber.exe
      Publisher.exe
      
 NOTE: Connecting to a secure tibeftlServer (via the wss:// protocol) requires 
 that the .NET client install the server trust certificate in the Microsoft 
 Certificate store under "Trusted Root Certification Authorities".


Samples description.
---------------------------------------------

 Publisher

      Basic publisher program that demonstrates the use of
      publishing eFTL messages.

 Subscriber

      Basic subscriber program that demonstrates the use of
      subscribing to eFTL messages.

 SharedDurable

      Basic subscriber program that demonstrates the use of
      shared durable subscriptions.

 LastValueDurable

      Basic subscriber program that demonstrates the use of
      last-value durable subscriptions.

 KVSet

      Basic key-value program that demonstrates the use of
      setting a key-value pair in a map.

 KVGet

      Basic key-value program that demonstrates the use of
      getting a key-value pair from a map.

 KVRemove

      Basic key-value program that demonstrates the use of
      removing a key-value pair from a map.
