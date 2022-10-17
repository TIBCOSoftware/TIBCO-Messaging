/*
 * @COPYRIGHT_BANNER@
 */

 This directory contains Java client samples for TIBCO eFTL.

 Samples located in this directory are simple examples of basic
 TIBCO eFTL functionality.


 Compiling and Running the Samples.
 ---------------------------------------------

 In order to compile and run samples you need to execute
 setup.bat (Windows) or setup (UNIX) script located in
 this directory's parent directory.

 To compile and run samples, do the following steps:

 1. Make sure your computer has Java 1.8 installed.

 2. Open a console window and change directory to the samples
    subdirectory of your TIBCO eFTL installation.

 3. Run the "setup" script.

 4. Change directory to the samples/java subdirectory.

 5. Execute:

      javac -d . src/com/tibco/eftl/samples/*.java

    This command compiles the samples 

 6. Now run the samples simply by executing:

      java com.tibco.eftl.samples.Subscriber
      java com.tibco.eftl.samples.Publisher


 Samples Description
 ---------------------------------------------

 com.tibco.eftl.samples.Publisher

    Basic publisher program that demonstrates the use of 
    publishing eFTL messages.

 com.tibco.eftl.samples.Subscriber

    Basic subscriber program that demonstrates the use of 
    subscribing to eFTL messages.

 com.tibco.eftl.samples.SharedDurable

    Basic subscriber program that demonstrates the use of 
    shared durable subscriptions.

 com.tibco.eftl.samples.LastValueDurable

    Basic subscriber program that demonstrates the use of 
    last-value durable subscriptions.

 com.tibco.eftl.samples.Request

    Basic request program that demonstrates the use of
    sending a request and receiving a reply.

 com.tibco.eftl.samples.Reply

    Basic reply program that demonstrates the use of
    sending a reply in response to a request.

 com.tibco.eftl.samples.KVSet

    Basic key-value program that demonstrates the use of
    setting a key-value pair in a map.

 com.tibco.eftl.samples.KVGet

    Basic key-value program that demonstrates the use of
    getting a key-value pair from a map.

 com.tibco.eftl.samples.KVRemove

    Basic key-value program that demonstrates the use of
    removing a key-value pair from a map.
