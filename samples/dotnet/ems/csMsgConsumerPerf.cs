/* 
 * Copyright (c) 2001-$Date: 2016-12-13 17:00:37 -0600 (Tue, 13 Dec 2016) $ TIBCO Software Inc. 
 * Licensed under a BSD-style license. Refer to [LICENSE]
 * For more information, please contact:
 * TIBCO Software Inc., Palo Alto, California, USA
 * 
 * $Id: csMsgConsumerPerf.cs 90180 2016-12-13 23:00:37Z olivierh $
 * 
 */

/// <summary>
/// This is a sample of a basic csMsgConsumer.
///
/// This sample subscribes to specified destination and receives and prints all
/// received messages. 
///
/// Notice that the specified destination should exist in your configuration
/// or your topics/queues configuration file should allow creation of the
/// specified destination.
///
/// If this sample is used to receive messages published by csMsgProducer
/// sample, it must be started prior to running the csMsgProducer sample.
///
/// Usage:  csMsgConsumer [options]
///
///    where options are:
///
///    -server       <server-url>  Server URL.
///                                If not specified this sample assumes a
///                                serverUrl of "tcp://localhost:7222"
///    -user         <user-name>   User name. Default is null.
///    -password     <password>    User password. Default is null.
///    -topic        <topic-name>  Topic name. Default value is "topic.sample"
///    -queue        <queue-name>  Queue name. No default
///    -count        <number msgs> Number of messages to send, 10K default.
///    -durable      <name>        Durable name. No default.
///    -threads      <number thrs> Number of threads to use for message consumption
///    -connections  <number cons> Number of connections to use.
///    -ackmode      <ack mode>    Acknowledgement mode. Default is AUTO.
///    -txnsize      <count>       Number of messages per transaction
///    -uniquedests                Each consumer uses a different destination
///    
/// </summary>

using System;
using System.Collections;
using System.Threading;
using TIBCO.EMS;

public class csMsgConsumerPerf
{
    String serverUrl = null;
    String userName = null;
    String password = null;
    String name = "topic.sample";
    String durableName = null;
    bool useTopic = true;
    bool useTxn = false;
    bool uniqueDests = false;
    bool stopNow = false;
    int txnSize = 0;
    int count = 10000;
    int runTime = 0;
    int threads = 1;
    int connections = 1;
    int connIter = 0;
    int destIter = 1;
    int nameIter = 1;
    int recvCount = 0;
    int ackMode = Session.AUTO_ACKNOWLEDGE;
    ArrayList startTime = null;
    ArrayList endTime = null;
    ArrayList connsVector = null;
    long elapsed = 0;
    
    public csMsgConsumerPerf(String[] args) 
    {
        ParseArgs(args);
        
        Console.WriteLine("\n------------------------------------------------------------------------");
        Console.WriteLine("csMsgConsumer SAMPLE");
        Console.WriteLine("------------------------------------------------------------------------");
        Console.WriteLine("Server....................... " + ((serverUrl != null)?serverUrl:"localhost"));
        Console.WriteLine("User......................... " + ((userName != null)?userName:"(null)"));
        Console.WriteLine("Destination.................. " + name);
        Console.WriteLine("Consumer Threads............. " + threads);
        Console.WriteLine("Consumer Connections......... " + connections);
        Console.WriteLine("Ack Mode..................... " + AcknowledgeModeName(ackMode));
        if (useTxn)
            Console.WriteLine("Transaction Size............. " + txnSize);
        Console.WriteLine("------------------------------------------------------------------------\n");
        
        try
        {
            Console.WriteLine("Subscribing to destination '" + name + "'\n");
            
            ConnectionFactory factory = new TIBCO.EMS.ConnectionFactory(serverUrl);
            
            connsVector = new ArrayList(connections);
            
            for (int i = 0; i < connections; i++)
            {
                Connection conn = factory.CreateConnection(userName, password);
                conn.Start();
                connsVector.Add(conn);
            }

            startTime = ArrayList.Synchronized(new ArrayList(threads));
            endTime = ArrayList.Synchronized(new ArrayList(threads));
            
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
                for (int i = 0; i < threads; i++)
                {
                    Thread t = (Thread) tv[i];
                    t.Interrupt();
                }
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
            Console.WriteLine(Performance);
            
        }
        catch (EMSException e)
        {
            Console.Error.WriteLine("Exception in csMsgConsumer: " +
                                    e.Message);
            Console.Error.WriteLine(e.StackTrace);
            Environment.Exit(-1);
        }
    }
    
    private void Usage() 
    {
        Console.WriteLine("\nUsage: csMsgConsumer [options]");
        Console.WriteLine("");
        Console.WriteLine("   where options are:");
        Console.WriteLine("");
        Console.WriteLine("   -server       <server URL> - Server URL, default is local server");
        Console.WriteLine("   -user         <user name>  - user name, default is null");
        Console.WriteLine("   -password     <password>   - password, default is null");
        Console.WriteLine("   -topic        <topic-name> - topic name, default is \"topic.sample\"");
        Console.WriteLine("   -queue        <queue-name> - queue name, no default");
        Console.WriteLine("   -durable      <name>       - durable subscriber name");
        Console.WriteLine("   -count        <nnnn>       - Number of messages to consume, default 10k");
        Console.WriteLine("   -time         <seconds>    - Number of seconds to run");
        Console.WriteLine("   -threads      <nnnn>       - Number of threads to use for consumers");
        Console.WriteLine("   -connections  <nnnn>       - Number of connections to use for consumers");
        Console.WriteLine("   -ackmode      <nnnn>       - Acknowledge Mode, default AUTO");
        Console.WriteLine("   -txnsize      <count>      - Number of messages per transaction");
        Console.WriteLine("   -uniquedests               - Each consumer uses a different destination");
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
            else if (args[i].CompareTo("-ackmode") == 0) 
            {
                if ((i + 1) >= args.Length)
                    Usage();
                String am = args[i+1];
                i += 2;
                if (am.CompareTo("DUPS_OK") == 0)
                    ackMode = Session.DUPS_OK_ACKNOWLEDGE;
                else if (am.CompareTo("AUTO") == 0)
                    ackMode = Session.AUTO_ACKNOWLEDGE;
                else if (am.CompareTo("CLIENT") == 0)
                    ackMode = Session.CLIENT_ACKNOWLEDGE;
                else if (am.CompareTo("EXPLICIT_CLIENT") == 0)
                    ackMode = Session.EXPLICIT_CLIENT_ACKNOWLEDGE;
                else if (am.CompareTo("EXPLICIT_CLIENT_DUPS_OK") == 0)
                    ackMode = Session.EXPLICIT_CLIENT_DUPS_OK_ACKNOWLEDGE;
                else if (am.CompareTo("NO") == 0)
                    ackMode = Session.NO_ACKNOWLEDGE;
                else
                {
                    Console.Error.WriteLine("Error: invalid value of -ackmode parameter");
                    Console.Error.WriteLine("Error: values are: DUPS_OK, AUTO, CLIENT, EXPLICIT_CLIENT, EXPLICIT_CLIENT_DUPS_OK, NO");
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
            else if (args[i].CompareTo("-durable") == 0) 
            {
                if ((i + 1) >= args.Length)
                    Usage();
                durableName = args[i+1];
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
            else if (args[i].CompareTo("-uniquedests") == 0) 
            {
                uniqueDests = true;
                i += 1;
            } 
            else if (args[i].CompareTo("-help") == 0) 
            {
                Usage();
            } 
            else 
            {
                Console.Error.WriteLine("Unrecognized parameter: " + args[i]);
                Usage();
            }
        }
    }

    public void CountReceives(int count)
    {
        lock (this)
        {
            recvCount += count;
        }
    }

    private void Run()    
    {
        int i = 0;
        bool first = true;
        Message msg;
        String uniqueName = null;
        int numMsgsToReceive;
        MessageConsumer msgConsumer = null;
        Session session = null;

        try
        {
            Thread.Sleep(new TimeSpan(10000 * 250));
        }
        catch (ThreadInterruptedException)
        {
        }
        
        try
        {
            // create the session
            Connection connection = this.MyConnection;
            session = connection.CreateSession(useTxn, ackMode);
            
            // create the destination 
            Destination destination = CreateDestination(session);
        
            // create the consumer
            if (durableName == null)
            {
                msgConsumer = session.CreateConsumer(destination);
            }
            else 
            {
                uniqueName = this.MyUniqueName;
                msgConsumer = session.CreateDurableSubscriber((Topic) destination, uniqueName);
            }
                        
            if (Tibems.IsConsumerMulticast(msgConsumer))
                Tibems.MulticastExceptionHandler += new EMSMulticastExceptionHandler(HandleMulticastException);

            if (uniqueDests || useTopic)
                numMsgsToReceive = count;
            else
                numMsgsToReceive = (count/threads);

            // read messages
            for (i = 0; i < numMsgsToReceive; i++)
            {
                // receive the message
                msg = msgConsumer.Receive();

                if (first)
                {
                    startTime.Add((DateTime.Now.Ticks - 621355968000000000) / 10000);
                    first = false;
                }

                if (msg == null)
                    break;

                if (ackMode == Session.CLIENT_ACKNOWLEDGE ||
                    ackMode == Session.EXPLICIT_CLIENT_ACKNOWLEDGE ||
                    ackMode == Session.EXPLICIT_CLIENT_DUPS_OK_ACKNOWLEDGE)
                    msg.Acknowledge();

                if (useTxn && (i % txnSize) == (txnSize - 1))
                    session.Commit();
            }

            if (useTxn)
                session.Commit();
        } 
        catch (EMSException e)
        {
            if (!stopNow)
            {
                Console.Error.WriteLine("Exception in csMsgConsumerPerf: " +
                                        e.Message);
                Console.Error.WriteLine(e.StackTrace);
                
                if (e.LinkedException != null)
                {
                    Console.Error.WriteLine("Linked Exception: " + 
                                            e.LinkedException.Message);
                    Console.Error.WriteLine(e.LinkedException.StackTrace);
                }
                
                Environment.Exit(- 1);
            }
        }

        if (!first)
        {
            endTime.Add((DateTime.Now.Ticks - 621355968000000000) / 10000);
            CountReceives(i);
        }

        try 
        {
            msgConsumer.Close();
            
            if (durableName != null)
                session.Unsubscribe(uniqueName);

            session.Close();
        }
        catch (EMSException)
        {
        }
    }

    public void HandleMulticastException(object sender, 
                                         EMSMulticastExceptionEventArgs arg)
    {
        Console.WriteLine(arg.Exception.Message);

        try
        {
            arg.Session.Close();
        }
        catch (EMSException)
        {
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

    public String MyUniqueName
    {
        get
        {
            lock (this)
            {
                return durableName + nameIter++;
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

    public long ElapsedTime
    {
        get
        {
            return elapsed;
        }
    }

    public int ReceiveCount
    {
        get
        {
            return recvCount;
        }
    }

    public String Performance 
    {
        get 
        {
            
            long st = Int64.MaxValue;
            long et = 0;
            
            for (int i = 0; i < startTime.Count; i++)
            {
                long sti = (long) ((Int64) startTime[i]);
                if (sti < st)
                    st = sti;
            }
            
            for (int i = 0; i < endTime.Count; i++)
            {
                long eti = (long) ((Int64) endTime[i]);
                if (eti > et)
                    et = eti;
            }
            
            elapsed = et - st;
            int seconds = (int) (elapsed / 1000);
            int millis = (int) (elapsed % 1000);
            double d = ((double) recvCount) / (((double) elapsed) / 1000.0);
            int iperf = (int) d;
            return ("" + recvCount + " times took " + seconds + "." + millis +
                    " seconds, performance is " + iperf + " messages/second"); 
        }
    }

    // returns printable acknowledge mode name
    public static String AcknowledgeModeName(int ackMode) 
    {
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

    public static void Main(String[] args) 
    {
        csMsgConsumerPerf t = new csMsgConsumerPerf(args);
    }
}
