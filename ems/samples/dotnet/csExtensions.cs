/* 
 * Copyright (c) 2001-$Date: 2016-12-13 17:00:37 -0600 (Tue, 13 Dec 2016) $ TIBCO Software Inc. 
 * Licensed under a BSD-style license. Refer to [LICENSE]
 * For more information, please contact:
 * TIBCO Software Inc., Palo Alto, California, USA
 * 
 * $Id: csExtensions.cs 90180 2016-12-13 23:00:37Z olivierh $
 * 
 */

/// <summary>
/// Sample of how delivery and acknowledgement modes can be used.
///
/// This sample publishes a specified number of messages on a topic
/// and receives all of those messages in a different session. After
/// all messages have been received by the subsciber, it publishes
/// back a confirmation message to ensure all messages have been
/// delivered.
///
/// This test is performed 4 times using four combinations of delivery mode
/// and acknowledge mode:
///
///   Publisher mode        Subscriber mode
///  ----------------------------------------------
///   NON_PERSISTENT        CLIENT_ACKNOWLEDGE
///   NON_PERSISTENT        AUTO_ACKNOWLEDGE
///   NON_PERSISTENT        DUPS_OK_ACKNOWLEDGE
///  *RELIABLE_DELIVERY    *NO_ACKNOWLEDGE
///
/// Depending on your hardware you may need to change the number of
/// messages being published in a single run. The run should persist for
/// at least 3 seconds to give reliable results.
/// For best results make sure it takes no less than 10 seconds to complete.
/// The default value in this sample is 10,000 messages. This may not be large
/// enough if you run this sample on fast hardware.
///
/// Under normal circumstances you should see each run to perform
/// faster than the previous and you can compare the results. The run
/// which uses proprietary values of RELIABLE_DELIVERY and NO_ACKNOWLEDGE
/// should perform much faster than other runs, although performance
/// will depend on a number of external parameters.
///
/// It is *strongly* recommended to run this sample on a computer not
/// performing any other calculational or networking tasks.
///
/// Notice that increased performance is achieved by the client and the
/// server omitting certain confirmation messages which in turn increases
/// the chances of messages being lost during a provider failure. Please
/// refer to the TIBCO EMS documentation which explains these issues in
/// detail. This sample should not be used to calculate the performance of
/// TIBCO EMS, it exists solely to compare relative performance of topic
/// delivery utilizing different delivery and acknowledgment modes.
///
/// Also notice this sample only demonstrates relative performance in a
/// particular situation when the publisher and the subscriber run in the
/// same application on a dedicated computer.
/// The difference in performance may change greatly depending on particular
/// usage. This includes a possibility that the RELIABLE delivery and
/// NO_ACKNOWLEDGE acknowledge modes may deliver even greater performance
/// advantage compared to other modes if the publisher and subscribers
/// run on different computers.
///
/// Usage:  csExtensions  [options]
///
///    where options are:
///
///    -server    <server-url>  Server URL.
///                             If not specified this sample assumes a
///                             serverUrl of "tcp://localhost:7222"
///    -user      <user-name>   User name. Default is null.
///    -password  <password>    User password. Default is null.
///    -topic     <topic-name>  Topic name. Default value is "topic.sample"
///    -count     <msg count>   Number of messages to use in each run.
///                             Default is 20000.
///
/// </summary>

using System;
using System.Threading;
using TIBCO.EMS;

public class csExtensions
{
    String serverUrl = null;
    String userName = null;
    String password = null;
    
    int count = 20000;
    
    Connection connection = null;
    
    // these used to publish and receive all messages
    Topic topic = null;
    Session publishSession;
    Session subscribeSession;
    MessageProducer publisher;
    MessageConsumer subscriber;
    
    // these used to send final confirmation when the subscriber receives all
    // messages it supposed to receive
    Topic checkTopic = null;
    MessageProducer checkPublisher;
    MessageConsumer checkSubscriber;
    
    // simple helper to print the performance nummbers
    public String GetPerformance(long startTime, long endTime, int count) {
    if (startTime >= endTime)
        return Int32.MaxValue.ToString();
    int seconds = (int) ((endTime - startTime) / 1000);
    int millis = (int) ((endTime - startTime) % 1000);
    double d = ((double) count) / (((double) (endTime - startTime)) / 1000.0);
    int iperf = (int) d;
    return ("" + count + " times took " + seconds + "." + millis +
        " seconds, performance is " + iperf + " messages/second"); 
    }
    
    // returns printable delivery mode name
    public static String DeliveryModeName(int delMode) {
    switch (delMode)
    {
        case DeliveryMode.PERSISTENT: 
        return "PERSISTENT";
            
        case DeliveryMode.NON_PERSISTENT: 
        return "NON_PERSISTENT";
            
        case DeliveryMode.RELIABLE_DELIVERY: 
        return "RELIABLE";
            
        default: 
        return "???";
    }
    }
    
    // returns printable acknowledge mode name
    public static String AcknowledgeModeName(int ackMode) {
    switch (ackMode)
    {
        case Session.CLIENT_ACKNOWLEDGE: 
        return "CLIENT_ACKNOWLEDGE";
            
        case Session.AUTO_ACKNOWLEDGE: 
        return "AUTO_ACKNOWLEDGE";
            
        case Session.DUPS_OK_ACKNOWLEDGE: 
        return "DUPS_OK_ACKNOWLEDGE";
            
        case Session.NO_ACKNOWLEDGE: 
        return "NO_ACKNOWLEDGE";
            
        default: 
        return "???";
    }
    }
    
    // a thread which keeps printing dots on the screen
    public void Run() {
    try
    {
        while (true)
        {
        Thread.Sleep(1000);
        Console.Write(".");
        }
    }
    catch (ThreadInterruptedException)
    {
        // return
    }
    }
    
    // A listener for the asynchronous consumer receiving messages published
    // on a topic. It does pretty much nothing but receives and counts
    // messages delivered to it by the subscriber session. When all messages
    // have been received it publishes a confirmation message to the
    // publishing session.
    class MyMessageListener : IMessageListener
    {
    private csExtensions enclosingInstance;
    private int receivedCount = 0;
    private int acknowledgeMode = 0;
    private bool warmupMode = false;
        
    public MyMessageListener(csExtensions enclosingInstance,
        int acknowledgeMode, bool warmupMode) {
        this.enclosingInstance = enclosingInstance;
        this.acknowledgeMode = acknowledgeMode;
        this.warmupMode = warmupMode;
        receivedCount = 0;
    }
        
    public void OnMessage(Message message) {
        receivedCount++;
            
        try
        {
        // acknowledge if CLIENT_ACKNOWLEDGE
        if (acknowledgeMode == Session.CLIENT_ACKNOWLEDGE)
            message.Acknowledge();
                
        // if it was the last message, publish a confirmation that we
        // have received all messages
        if (receivedCount == enclosingInstance.count)
            enclosingInstance.checkPublisher.Send(message);
        }
        catch (EMSException e)
        {
        Console.Error.WriteLine("Exception in csExtensions: " + e.Message);
        Console.Error.WriteLine(e.StackTrace);
        Environment.Exit(0);
        }
    }
    }
    
    // Main method which actually publishes all messages, checks when they all
    // have been received by the subscriber, then calculates and prints the
    // performance numbers. The delivery mode and acknowledge mode are
    // parameters to this method.
    public void DoRun(int deliveryMode, int acknowledgeMode,
    bool warmupMode) {

    // we need two sessions
    publishSession = connection.CreateSession(false, acknowledgeMode);
    subscribeSession = connection.CreateSession(false, acknowledgeMode);
        
    // little thread printing dots on the screen while nothing else happens
    Thread tracker = null;
        
    // if not yet created, create main topic and the check topic used to
    // send confirmations back from the subscriber to the publisher
    if (topic == null)
    {
        topic = publishSession.CreateTopic("test.extensions.topic");
        checkTopic = publishSession.CreateTopic("test.extensions.check.topic");
    }
        
    // create the publisher and set its delivery mode
    publisher = publishSession.CreateProducer(topic);
    publisher.DeliveryMode = deliveryMode;
        
    // create the subscriber and set its listener
    subscriber = subscribeSession.CreateConsumer(topic);
    subscriber.MessageListener = new MyMessageListener(this, acknowledgeMode, warmupMode);
        
    // create publisher and subscriber to deal with the final confirmation
    // message
    checkPublisher = subscribeSession.CreateProducer(checkTopic);
    checkSubscriber = publishSession.CreateConsumer(checkTopic);
        
    // will publish virtually empty message
    Message message = publishSession.CreateMessage();
        
    if (!warmupMode)
    {
        Console.WriteLine("");
        Console.WriteLine("Sending and Receiving " + count + " messages");
        Console.WriteLine("        delivery mode:    " + DeliveryModeName(deliveryMode));
        Console.WriteLine("        acknowledge mode: " + AcknowledgeModeName(acknowledgeMode));
    }
        
    // start printing dots while we run the main loop
    tracker = new Thread(new ThreadStart(this.Run));
    tracker.Start();
        
    long startTime = (DateTime.Now.Ticks - 621355968000000000) / 10000;
        
    // now publish all messages
    for (int i = 0; i < count; i++)
        publisher.Send(message);
        
    // and wait for the subscriber to receive all messages and publish the
    // confirmation message which we'll receive here. If anything goes
    // wrong we will get stuck here, but let's hope everything will be
    // fine
    checkSubscriber.Receive();
        
    // print performance, close all and we are done
    long endTime = (DateTime.Now.Ticks - 621355968000000000) / 10000;
        
    tracker.Interrupt();
        
    if (!warmupMode)
    {
        Console.WriteLine(" completed");
            
        if ((endTime - startTime) < 3000)
        {
        Console.WriteLine("**Warning**: elapsed time is too small to calculate reliable performance");
        Console.WriteLine("             numbers. You need to re-run this test with greater number");
        Console.WriteLine("             of messages. Use '-count' command-line parameter to specify");
        Console.WriteLine("             greater message count.");
        }
            
        Console.WriteLine(GetPerformance(startTime, endTime, count));
    }
        
    publishSession.Close();
    subscribeSession.Close();
    }
    
    public csExtensions(String[] args) {

    ParseArgs(args);

    // print parameters
    Console.WriteLine("\n------------------------------------------------------------------------");
    Console.WriteLine("csExtensions SAMPLE");
    Console.WriteLine("------------------------------------------------------------------------");
    Console.WriteLine("Server....................... " + ((serverUrl != null)?serverUrl:"localhost"));
    Console.WriteLine("User......................... " + ((userName != null)?userName:"(null)"));
    Console.WriteLine("------------------------------------------------------------------------\n");
        
    try
    {
        ConnectionFactory factory = new TIBCO.EMS.ConnectionFactory(serverUrl);
            
        connection = factory.CreateConnection(userName, password);
            
        // do not forget to start the connection...
        connection.Start();
            
        int n = count;
            
        // Warm up the server and HotSpot...
        Console.Write("Warming up before testing the performance. Please wait...");
        count = 3000;
        DoRun(DeliveryMode.NON_PERSISTENT, Session.AUTO_ACKNOWLEDGE, true);
        Console.WriteLine("");
            
        // let the messaging server quiesce
        try
        {
        Thread.Sleep(1000);
        }
        catch (ThreadInterruptedException)
        {
        }
            
        // recover actual count
        count = n;
            
        // Use PERSISTENT / CLIENT_ACKNOWLEDGE modes
        DoRun(DeliveryMode.PERSISTENT, Session.CLIENT_ACKNOWLEDGE, false);
            
        // Use PERSISTENT / AUTO_ACKNOWLEDGE modes
        DoRun(DeliveryMode.PERSISTENT, Session.AUTO_ACKNOWLEDGE, false);
            
        // Use PERSISTENT / DUPS_OK_ACKNOWLEDGE modes
        DoRun(DeliveryMode.PERSISTENT, Session.DUPS_OK_ACKNOWLEDGE, false);
            
        // Use NON_PERSISTENT / CLIENT_ACKNOWLEDGE modes
        DoRun(DeliveryMode.NON_PERSISTENT, Session.CLIENT_ACKNOWLEDGE, false);
            
        // Use NON_PERSISTENT / AUTO_ACKNOWLEDGE modes
        DoRun(DeliveryMode.NON_PERSISTENT, Session.AUTO_ACKNOWLEDGE, false);
            
        // Use NON_PERSISTENT / DUPS_OK_ACKNOWLEDGE modes
        DoRun(DeliveryMode.NON_PERSISTENT, Session.DUPS_OK_ACKNOWLEDGE, false);
            
        // Use proprietary RELIABLE_DELIVERY / NO_ACKNOWLEDGE modes
        DoRun(DeliveryMode.RELIABLE_DELIVERY, Session.NO_ACKNOWLEDGE, false);
            
        connection.Close();
    }
    catch (EMSException e)
    {
        Console.Error.WriteLine("Exception in csExtensions: " + e.Message);
        Console.Error.WriteLine(e.StackTrace);
        Environment.Exit(0);
    }
    }
    
    public static void  Main(String[] args) {

    csExtensions t = new csExtensions(args);
    }
    
    void Usage() {
    Console.WriteLine("\nUsage: csExtensions [options]");
    Console.WriteLine("");
    Console.WriteLine("  where options are:");
    Console.WriteLine("");
    Console.WriteLine("  -server    <server URL> - Server URL, default is local server");
    Console.WriteLine("  -user      <user name>  - user name, default is null");
    Console.WriteLine("  -password  <password>   - password, default is null");
    Console.WriteLine("  -count     <nnnn>       - number of messages, default is 20000");
    Environment.Exit(0);
    }
    
    void ParseArgs(String[] args) {
    int i = 0;
        
    while (i < args.Length)
    {
        if (args[i].CompareTo("-server") == 0) {
        if ((i + 1) >= args.Length)
            Usage();
        serverUrl = args[i+1];
        i += 2;
        } else if (args[i].CompareTo("-count") == 0) {
        if ((i + 1) >= args.Length)
            Usage();
        try
        {
            count = Int32.Parse(args[i+1]);
        }
        catch (FormatException)
        {
            Console.Error.WriteLine("Error: invalid value of -count parameter");
            Usage();
        }
        i += 2;
        } else if (args[i].CompareTo("-user") == 0) {
        if ((i + 1) >= args.Length)
            Usage();
        userName = args[i+1];
        i += 2;
        } else if (args[i].CompareTo("-password") == 0) {
        if ((i + 1) >= args.Length)
            Usage();
        password = args[i+1];
        i += 2;
        } else if (args[i].CompareTo("-help") == 0) {
        Usage();
        } else {
        Console.Error.WriteLine("Unrecognized parameter: " + args[i]);
        Usage();
        }
    }
    }
}
