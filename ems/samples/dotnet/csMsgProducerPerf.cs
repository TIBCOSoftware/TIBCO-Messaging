/* 
 * Copyright (c) 2001-$Date: 2016-12-13 17:00:37 -0600 (Tue, 13 Dec 2016) $ TIBCO Software Inc. 
 * Licensed under a BSD-style license. Refer to [LICENSE]
 * For more information, please contact:
 * TIBCO Software Inc., Palo Alto, California, USA
 * 
 * $Id: csMsgProducerPerf.cs 90180 2016-12-13 23:00:37Z olivierh $
 * 
 */

/// <summary>
/// This is a simple csMsgProducer performance sample.
///
/// This samples publishes specified message(s) on a specified destination and
/// quits. 
///
/// Notice that the specified destination should exist in your configuration
/// or your topics/queues configuration file should allow creation of the
/// specified topic or queue. Sample configuration supplied with the TIBCO
/// EMS distribution allows creation of any destination.
///
/// If this sample is used to publish messages into csMsgConsumer sample, the
/// csMsgConsumer sample must be started first.
///
/// If -topic is not specified this sample will use a topic named
/// "topic.sample".
///
/// Usage:  csMsgProducerPerf  [options]
///
///  where options are:
///
///   -server       <server-url>  Server URL.
///                               If not specified this sample assumes a
///                               serverUrl of "tcp://localhost:7222"
///   -user         <user-name>   User name. Default is null.
///   -password     <password>    User password. Default is null.
///   -topic        <topic-name>  Topic name. Default value is "topic.sample"
///   -queue        <queue-name>  Queue name. No default
///   -count        <number msgs> Number of messages to send, 10K default.
///   -threads      <number thrs> Number of threads to use for message production
///   -connections  <number cons> Number of connections to use.
///   -size         <msgsize>     Size of each message to be sent
///   -delivery     <delivery>    Specifies deliveryMode
///   -txnsize      <count>       Number of messages per transaction
///   -uniquedests                Each producer uses a different destination
///   -compression                Enable compression while sending messages
/// </summary>

using System;
using System.IO;
using System.Text;
using System.Collections;
using System.Threading;
using TIBCO.EMS;

public class csMsgProducerPerf
{
    String serverUrl = null;
    String userName = null;
    String password = null;
    String name = "topic.sample";
    bool useTopic = true;
    bool useTxn = false;
    bool uniqueDests = false;
    String payloadFile = null;
    bool compression = false;
    int msgRate = 0;
    bool stopNow = false;
    int txnSize = 0;
    int count = 10000;
    int runTime = 0;
    int msgSize = 0;
    int threads = 1;
    int connections = 1;
    int connIter = 0;
    int destIter = 1;
    int delMode = DeliveryMode.NON_PERSISTENT;
    int sentCount = 0;
    long startTime = 0;
    long endTime = 0;
    long completionTime = 0;
    ArrayList connsVector = null;
    long elapsed;
    private bool async = false;

    private class EMSCompletionListener : ICompletionListener
    {
        private int     completionCount = 0;
        private long    completionTime = 0;
        private bool    finished        = false;
        private Object  finishedLock    = new Object();
        private int     finishCount     = 0;
        
        internal void WaitUntilFinished()
        {
            lock (finishedLock)
            {
                while (!finished)
                {
                    try
                    {
                        Monitor.Wait(finishedLock);
                    }
                    catch (ThreadInterruptedException)
                    {
                        finished = true;
                    }
                }
            }
        }
        
        internal int FinishCount
        {
            set
            {
                lock (finishedLock)
                {
                    finishCount = value;
                    if (completionCount >= finishCount)
                    {
                        if (completionTime == 0)
                            completionTime = DateTime.Now.Ticks;

                        finished = true;
                    }
                }
            }
        }

        internal long CompletionTime
        {
            get { return completionTime; }
        }
        
        public void OnCompletion(Message msg)
        {
            completionCount++;

            lock (finishedLock)
            {
                if (finished)
                    return;
                
                if (finishCount > 0 && completionCount >= finishCount)
                {
                    finished = true;
                    completionTime = DateTime.Now.Ticks;
                    Monitor.Pulse(finishedLock);
                }
            }
        }

        public void OnException(Message msg, Exception ex)
        {
            System.Console.WriteLine("Error:  Exception with sending message.");
            System.Console.WriteLine("  Message: " + ex.Message);
            System.Console.WriteLine("  " + ex.StackTrace);
        }
    }

    /**
     * Class used to control the producer's send rate.
     */
    private class MsgRateChecker
    {
        int  rate;
        long sampleStart;
        long sampleTime;
        int  sampleCount;
        const long SAMPLE_TIME_ADJUST = 10 * TimeSpan.TicksPerMillisecond;
        const long SAMPLE_TIME_CHECK_LOW = 300 * TimeSpan.TicksPerMillisecond;
        const long SAMPLE_TIME_CHECK_HIGH = 20 * TimeSpan.TicksPerMillisecond;

        internal MsgRateChecker(int rate)
        {
            this.rate = rate;
            this.sampleTime = 10 * TimeSpan.TicksPerMillisecond;
        }

        internal void checkMsgRate(int count)
        {
            if (rate < 100)
            {
                if (count % 10 == 0)
                {
                    try
                    {
                        int sleepTime = (int)((10.0 / (double)rate) * 1000);
                        Thread.Sleep((int)sleepTime);
                    }
                    catch (ThreadInterruptedException) { }
                }
            }
            else if (sampleStart == 0)
            {
                sampleStart = DateTime.Now.Ticks;
            }
            else
            {
                long elapsed = DateTime.Now.Ticks - sampleStart;
                if (elapsed >= sampleTime)
                {
                    int actualMsgs = count - sampleCount;
                    int expectedMsgs = (int)((elapsed/TimeSpan.TicksPerMillisecond) * ((double)rate / 1000.0));
                    if (actualMsgs > expectedMsgs)
                    {
                        int sleepTime = (int)((double)(actualMsgs - expectedMsgs) / ((double)rate / 1000.0));
                        try
                        {
                            Thread.Sleep(sleepTime);
                        }
                        catch (ThreadInterruptedException) { }

                        if (sampleTime > SAMPLE_TIME_CHECK_HIGH)
                            sampleTime -= SAMPLE_TIME_ADJUST;
                    }
                    else
                    {
                        if (sampleTime < SAMPLE_TIME_CHECK_LOW)
                            sampleTime += SAMPLE_TIME_ADJUST;
                    }
                    sampleStart = DateTime.Now.Ticks;
                    sampleCount = count;
                }
            }
        }
    }


    public Connection MyConnection 
    {
        get 
        {
            lock (this) 
            {
                Connection connection = (Connection) connsVector[connIter++];
                if (connIter == connections)
                    connIter = 0;
                return connection;
            }
        }        
    }

    public Destination CreateDestination(Session session)
    {
        Destination destination;
        lock (this)
        {
            if (useTopic)
            {
                if (uniqueDests)
                {
                    destination = session.CreateTopic(name + '.' + destIter++);
                }
                else
                {
                    destination = session.CreateTopic(name);
                }
            }
            else
            {
                if (uniqueDests)
                {
                    destination = session.CreateQueue(name + '.' + destIter++);
                }
                else
                {
                    destination = session.CreateQueue(name);
                }
            }
        }
        return destination;
    }

    public csMsgProducerPerf(String[] args)    
    {

        ParseArgs(args);

        Console.WriteLine("\n------------------------------------------------------------------------");
        Console.WriteLine("csMsgProducerPerf SAMPLE");
        Console.WriteLine("------------------------------------------------------------------------");
        Console.WriteLine("Server....................... " + ((serverUrl != null)?serverUrl:"localhost"));
        Console.WriteLine("User......................... " + ((userName != null)?userName:"(null)"));
        Console.WriteLine("Destination.................. " + name);
        Console.WriteLine("Message Size................. " + (payloadFile != null ? payloadFile : msgSize.ToString()));
        Console.WriteLine("Count........................ " + count);
        Console.WriteLine("Production Threads........... " + threads);
        Console.WriteLine("Production Connections....... " + connections);
        Console.WriteLine("DeliveryMode................. " + DeliveryModeName(delMode));
        Console.WriteLine("Compression.................. " + compression);
        Console.WriteLine("Asynchronous Sending......... " + async);
        if (msgRate > 0)
            Console.WriteLine("Message Rate................. " + msgRate);
        if (useTxn)
            Console.WriteLine("Transaction Size............. " + txnSize);
        Console.WriteLine("------------------------------------------------------------------------\n");
        
        try
        {
            Console.WriteLine("Publishing to destination '" + name + "'\n");
            
            ConnectionFactory factory = new TIBCO.EMS.ConnectionFactory(serverUrl);
            
            connsVector = new ArrayList(connections);
            
            for (int i = 0; i < connections; i++)
            {
                Connection conn = factory.CreateConnection(userName, password);
                connsVector.Add(conn);
            }
            
            ArrayList tv = new ArrayList(threads);
            
            for (int i = 0; i < threads; i++)
            {
                Thread t = new Thread(new ThreadStart(this.Run));
                tv.Add(t);
                t.Start();
            }
            
            if (runTime > 0)
            {
                Thread.Sleep(runTime * 1000);
                stopNow = true;
            }

            for (int i = 0; i < threads; i++)
            {
                Thread t = (Thread) tv[i];
                try
                {
                    t.Join();
                }
                catch (ThreadInterruptedException e)
                {
                    Console.Error.WriteLine("Exception in csMsgProducerPerf: "
                        + e.Message);
                    Console.Error.WriteLine(e.StackTrace);
                    Environment.Exit(0);
                }
            }
            
            for (int i = 0; i < connections; i++)
            {
                Connection conn = (Connection) connsVector[i];
                // close the connection
                conn.Close();
            }
            
            // Print performance
            PrintPerformance();
        }
        catch (EMSException e)
        {
            Console.Error.WriteLine("Exception in csMsgProducerPerf: " + e.Message);
            Console.Error.WriteLine(e.StackTrace);
            Environment.Exit(- 1);
        }
    }
    
    public void CountSends(int count)
    {
        lock (this)
        {
            sentCount += count;
        }
    }

    public void Run() 
    {
        int msgCount = 0;
        MsgRateChecker msgRateChecker = null;
        EMSCompletionListener cl = null;
        int numMsgsToPublish;

        try
        {
            Thread.Sleep(500);
        }
        catch (ThreadInterruptedException){}
        
        try
        {
            // create the session
            Connection connection = this.MyConnection;
            Session session = connection.CreateSession(useTxn, Session.AUTO_ACKNOWLEDGE);
            
            // create the destination 
            Destination destination = CreateDestination(session);

            // create the producer
            MessageProducer msgProducer = session.CreateProducer(null);

            if (async)
                cl = new EMSCompletionListener();

            // set parameters on producer
            msgProducer.DeliveryMode = delMode;
            // Specific for performance
            msgProducer.DisableMessageID = true;
            msgProducer.DisableMessageTimestamp = true;
            
            // create the message
            Message msg = CreateMessage(session);
            
            if (uniqueDests || useTopic)
                numMsgsToPublish = count;
            else
                numMsgsToPublish = count / threads;

            if (compression)
                msg.SetBooleanProperty("JMS_TIBCO_COMPRESS", true);

            // initialize message rate checking
            if (msgRate > 0)
                msgRateChecker = new MsgRateChecker(msgRate);

            startTiming();

            while ((count == 0 || msgCount < numMsgsToPublish) && !stopNow)
            {
                // publish message
                if (async)
                    msgProducer.Send(destination, msg, cl);
                else
                    msgProducer.Send(destination, msg);

                msgCount++;

                // commit messages
                if (useTxn && (msgCount % txnSize) == (txnSize - 1))
                    session.Commit();

                if (msgRate > 0)
                    msgRateChecker.checkMsgRate(msgCount);
            }
        
            // commit remaining messages
            if (useTxn)
                session.Commit();

            stopTiming(cl, numMsgsToPublish);

            CountSends(msgCount);
        }
        catch (EMSException e)
        {
            Console.Error.WriteLine("Exception in csMsgProducerPerf: " +
                e.Message);
            Console.Error.WriteLine(e.StackTrace);

            if (e.LinkedException != null)
            {
                Console.Error.WriteLine("Linked Exception: " + 
                e.LinkedException.Message);
                Console.Error.WriteLine(e.LinkedException.StackTrace);
            }

            Environment.Exit(-1);
        }
    }
    
    // returns printable delivery mode name
    public static String DeliveryModeName(int delMode) 
    {
        switch (delMode)
        {
            case DeliveryMode.PERSISTENT: 
                return "PERSISTENT";
            
            case DeliveryMode.NON_PERSISTENT: 
                return "NON_PERSISTENT";
            
            case DeliveryMode.RELIABLE_DELIVERY: 
                return "RELIABLE";
            
            default: 
                return "Unknown";
        }
    }

    /**
     * Create the message.
     */
    private Message CreateMessage(Session session)
    {
        String payload = null;
        
        // create the message
        BytesMessage msg = session.CreateBytesMessage();

        // add the payload
        if (payloadFile != null)
        {
            try
            {
                using (StreamReader sr = new StreamReader(payloadFile))
                {
                    payload = sr.ReadToEnd();
                }
            }
            catch (Exception e)
            {
                System.Console.WriteLine("Error: unable to load payload file - " + e.Message);
            }
        }
        else if (msgSize > 0)
        {
            StringBuilder sb = new StringBuilder();
            char c = 'A';
            for (int i = 0; i < msgSize; i++)
            {
                sb.Append(c++);
                if (c > 'z')
                    c = 'A';
            }

            char[] buf = new char[sb.Length];
            sb.CopyTo(0, buf, 0, sb.Length);
            payload = new String(buf);
        }
        
        if (payload != null)
        {
            // add the payload to the message
            msg.WriteBytes(System.Text.Encoding.ASCII.GetBytes(payload));
        }
        
        return msg;
    }

    private void startTiming()
    {
        lock (this)
        {
            if (startTime == 0)
                startTime = DateTime.Now.Ticks;
        }
    }
    
    private void stopTiming(EMSCompletionListener cl, int count)
    {
        long et = DateTime.Now.Ticks;
        long ct = 0;

        if (cl != null)
        {
            cl.FinishCount = count;
            cl.WaitUntilFinished();
            ct = cl.CompletionTime;
        }

        lock (this)
        {
            if (et > endTime)
                endTime = et;

            if (ct > completionTime)
                completionTime = ct;
        }
    }

    private void PrintPerformance()
    {
        if (endTime > startTime)
        {
            elapsed = (endTime - startTime)/TimeSpan.TicksPerMillisecond;
            int seconds = (int)(elapsed / 1000);
            int millis = (int)(elapsed % 1000);
            double d = ((double)sentCount) / (((double)elapsed) / 1000.0);
            int iperf = (int)d;
            System.Console.WriteLine(sentCount + " times took " + seconds + "." + millis +
                " seconds, performance is " + iperf + " messages/second");
        }
        else
        {
            System.Console.WriteLine("interval too short to calculate a message rate");
        }

        if (async)
        {
            if (completionTime > startTime)
            {
                long completionElapsed = (completionTime - startTime)/TimeSpan.TicksPerMillisecond;
                double seconds = completionElapsed / 1000.0;
                int perf = (int)((sentCount * 1000.0) / completionElapsed);
                System.Console.WriteLine(sentCount +
                    " completion listener calls took " + seconds +
                    " seconds, performance is " + perf + " messages/second");
            }
            else
            {
                System.Console.WriteLine("interval too short to calculate a completion listener rate");
            }
        }
    }
        
    private void Usage() 
    {
        Console.WriteLine("\nUsage: csMsgProducerPerf [options]");
        Console.WriteLine("\n");
        Console.WriteLine("   where options are:");
        Console.WriteLine("");
        Console.WriteLine("   -server       <server URL>  - Server URL, default is local server");
        Console.WriteLine("   -user         <user name>   - user name, default is null");
        Console.WriteLine("   -password     <password>    - password, default is null");
        Console.WriteLine("   -topic        <topic-name>  - topic name, default is \"topic.sample\"");
        Console.WriteLine("   -queue        <queue-name>  - queue name, no default");
        Console.WriteLine("   -size         <nnnn>        - Message payload in bytes");
        Console.WriteLine("   -count        <nnnn>        - Number of messages to send, default 10k");
        Console.WriteLine("   -time         <seconds>     - Number of seconds to run");
        Console.WriteLine("   -threads      <nnnn>        - Number of threads to use for sends");
        Console.WriteLine("   -connections  <nnnn>        - Number of connections to use for sends");
        Console.WriteLine("   -delivery     <nnnn>        - DeliveryMode, default NON_PERSISTENT");
        Console.WriteLine("   -txnsize      <count>       - Number of messages per transaction");
        Console.WriteLine("   -rate         <msgs/sec>    - Message rate for each producer thread");
        Console.WriteLine("   -payload      <file name>   - File containing message payload.");
        Console.WriteLine("   -uniquedests                - Each producer uses a different destination");
        Console.WriteLine("   -compression                - Enable compression while sending messages");
        Console.WriteLine("   -async                      - Use asynchronous sends");
        
        Environment.Exit(0);
    }
    
    private void ParseArgs(String[] args)
    {
        int i = 0;
        
        while (i < args.Length)
        {
            if (args[i].CompareTo("-server") == 0) 
            {
                if ((i + 1) >= args.Length)
                    Usage();
                serverUrl = args[i+1];
                i += 2;
            } 
            else if (args[i].CompareTo("-topic") == 0) 
            {
                if ((i + 1) >= args.Length)
                    Usage();
                name = args[i+1];
                i += 2;
            } 
            else if (args[i].CompareTo("-queue") == 0) 
            {
                if ((i + 1) >= args.Length)
                    Usage();
                name = args[i+1];
                i += 2;
                useTopic = false;
            } 
            else if (args[i].CompareTo("-user") == 0) 
            {
                if ((i + 1) >= args.Length)
                    Usage();
                userName = args[i+1];
                i += 2;
            } 
            else if (args[i].CompareTo("-password") == 0) 
            {
                if ((i + 1) >= args.Length)
                    Usage();
                password = args[i+1];
                i += 2;
            } 
            else if (args[i].CompareTo("-delivery") == 0) 
            {
                if ((i + 1) >= args.Length)
                    Usage();
                String dm = args[i+1];
                i += 2;
                if (dm.CompareTo("PERSISTENT") == 0)
                    delMode = DeliveryMode.PERSISTENT;
                else if (dm.CompareTo("NON_PERSISTENT") == 0)
                    delMode = DeliveryMode.NON_PERSISTENT;
                else if (dm.CompareTo("RELIABLE") == 0)
                    delMode = DeliveryMode.RELIABLE_DELIVERY;
                else
                {
                    Console.Error.WriteLine("Error: invalid value of -delivery parameter");
                    Console.Error.WriteLine("Error: values are: PERSISTENT, NON_PERSISTENT, RELIABLE");
                }
            } 
            else if (args[i].CompareTo("-count") == 0) 
            {
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
            } 
            else if (args[i].CompareTo("-time") == 0) 
            {
                if ((i + 1) >= args.Length)
                    Usage();
                try
                {
                    runTime = Int32.Parse(args[i+1]);
                }
                catch (FormatException)
                {
                    Console.Error.WriteLine("Error: invalid value of -time parameter");
                    Usage();
                }
                i += 2;
            } 
            else if (args[i].CompareTo("-threads") == 0) 
            {
                if ((i + 1) >= args.Length)
                    Usage();
                try
                {
                    threads = Int32.Parse(args[i+1]);
                }
                catch (FormatException)
                {
                    Console.Error.WriteLine("Error: invalid value of -threads parameter");
                    Usage();
                }
                if (threads < 1)
                {
                    Console.Error.WriteLine("Error: invalid value of -threads parameter, must be >= 1");
                    Usage();
                }
                i += 2;
            } 
            else if (args[i].CompareTo("-connections") == 0) 
            {
                if ((i + 1) >= args.Length)
                    Usage();
                try
                {
                    connections = Int32.Parse(args[i+1]);
                }
                catch (FormatException)
                {
                    Console.Error.WriteLine("Error: invalid value of -connections parameter");
                    Usage();
                }
                if (connections < 1)
                {
                    Console.Error.WriteLine("Error: invalid value of -connections parameter, must be >= 1");
                    Usage();
                }
                
                i += 2;
            } 
            else if (args[i].CompareTo("-size") == 0) 
            {
                if ((i + 1) >= args.Length)
                    Usage();
                try
                {
                    msgSize = Int32.Parse(args[i+1]);
                }
                catch (FormatException)
                {
                    Console.Error.WriteLine("Error: invalid value of -size parameter");
                    Usage();
                }
                i += 2;
            } 
            else if (args[i].CompareTo("-txnsize") == 0) 
            {
                if ((i + 1) >= args.Length)
                    Usage();
                try
                {
                    txnSize = Int32.Parse(args[i+1]);
                    if (txnSize < 1)
                    {
                        Console.Error.WriteLine("Error: invalid value of -txnsize parameter, must be >= 1");
                        Usage();
                    }
                    useTxn = true;
                }
                catch (FormatException)
                {
                    Console.Error.WriteLine("Error: invalid value of -size parameter");
                    Usage();
                }
                i += 2;
            }
            else if (args[i].CompareTo("-rate") == 0)
            {
                if ((i + 1) >= args.Length)
                    Usage();
                try
                {
                    msgRate = Int32.Parse(args[i + 1]);
                }
                catch (FormatException)
                {
                    System.Console.WriteLine("Error: invalid value of -rate parameter");
                    Usage();
                }
                if (msgRate < 1)
                {
                    System.Console.WriteLine("Error: invalid value of -rate parameter");
                    Usage();
                }
                i += 2;
            }
            else if (args[i].CompareTo("-payload") == 0)
            {
                if ((i + 1) >= args.Length)
                    Usage();

                payloadFile = args[i + 1];
                i += 2;
            }

            else if (args[i].CompareTo("-uniquedests") == 0) 
            {
                uniqueDests = true;
                i += 1;
            } 
            else if (args[i].CompareTo("-compression") == 0) 
            {
                compression = true;
                i += 1;
            }
            else if (args[i].CompareTo("-async") == 0)
            {
                async = true;
                i += 1;
            }
            else 
            {
                Usage();
            }
        }
    }
    
    public long ElapsedTime
    {
        get
        {
            return elapsed;
        }
    }

    public int SentCount
    {
        get
        {
            return sentCount;
        }
    }

    public static void Main(String[] args) 
    {
        new csMsgProducerPerf(args);
    }
}
