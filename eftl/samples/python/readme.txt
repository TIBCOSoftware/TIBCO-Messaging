/*
 * Copyright 2013-2021 Cloud Software Group Inc.
 * All Rights Reserved. Confidential and Proprietary.
 */

 This directory contains python client samples for TIBCO eFTL.

 Samples located in this directory are simple examples of basic
 eFTL functionality.


 Running samples.
 ---------------------------------------------

 In order to run samples you need to execute
 the setup (setup.bat for windows) script located 
 in this directory's parent directory.

 To run samples, do the following steps:

 1. Open a console window and change directory to the samples
    subdirectory of your TIBCO eFTL installation.

 2. Run the "setup" script. (setup.bat for windows)

 3. Change directory to the samples/python subdirectory.

 4. Run the samples by executing
   
    python3 subscriber.py (in a terminal or command propmt (on windows), on windows it's python.exe (please make sure it's python 3.6 or greater)) 
    python3 publisher.py  (in a terminal or command prompt (on windows), on windows it's python.exe (please make sure it's python 3.6 or greater))
   
NOTE: Prior to running the samples you will have to install the 'eftl-1.0.0-py3-none-any.whl'

      pip3 install eftl-1.0.0-py3-none-any.whl

      eftl-1.0.0-py3-none-any.whl is located in sdks/python3 directory of the 
      eFTL installation.
     
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
