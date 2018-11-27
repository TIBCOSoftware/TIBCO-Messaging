/* 
 * Copyright (c) 2001-$Date: 2016-12-13 17:00:37 -0600 (Tue, 13 Dec 2016) $ TIBCO Software Inc. 
 * Licensed under a BSD-style license. Refer to [LICENSE]
 * For more information, please contact:
 * TIBCO Software Inc., Palo Alto, California, USA
 * 
 * $Id: csAsyncMsgConsumerUsingDelegate.cs 90180 2016-12-13 23:00:37Z olivierh $
 * 
 */

/// <summary>
/// This is a simple sample of a basic asynchronous csMsgConsumer.
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
/// Usage:  csAsyncMsgConsumerUsingDelegate [options]
///
///    where options are:
///
///    -server    <server-url/>  Server URL.
///                             If not specified this sample assumes a
///                             serverUrl of "tcp://localhost:7222"
///    -user      <user-name/>   User name. Default is null.
///    -password  <password/>    User password. Default is null.
///    -topic     <topic-name/>  Topic name. Default value is "topic.sample"
///    -queue     <queue-name/>  Queue name. No default
///
/// </summary>

using System;
using TIBCO.EMS;

public class csAsyncMsgConsumerUsingDelegate 
{
    String  serverUrl = null;
    String  userName  = null;
    String  password  = null;
    String  name      = "topic.sample";
    bool    useTopic  = true;

    Connection      connection  = null;
    Session         session     = null;
    MessageConsumer msgConsumer = null;
    Destination     destination = null; 

    object stateLock = new object();
    bool stop = false;

    public csAsyncMsgConsumerUsingDelegate(String[] args) 
    {
        if (ParseArgs(args) < 0)
            return;

        try {
            tibemsUtilities.initSSLParams(serverUrl,args);
        }
        catch (Exception e)
        {
            System.Console.WriteLine("Exception: "+e.Message);
            System.Console.WriteLine(e.StackTrace);
            System.Environment.Exit(-1);
        }

        Console.WriteLine("------------------------------------------------------------------------");
        Console.WriteLine("csAsyncMsgConsumerUsingDelegate SAMPLE");
        Console.WriteLine("------------------------------------------------------------------------");
        Console.WriteLine("Server....................... " + ((serverUrl != null)?serverUrl:"localhost"));
        Console.WriteLine("User......................... " + ((userName != null)?userName:"(null)"));
        Console.WriteLine("Destination.................. " + name);
        Console.WriteLine("------------------------------------------------------------------------\n");

        try {
            ConnectionFactory factory = new TIBCO.EMS.ConnectionFactory(serverUrl);

            // create the connection
            connection = factory.CreateConnection(userName, password);

            // create the session
            session = connection.CreateSession(false, Session.AUTO_ACKNOWLEDGE);

            // add the exception listener
            connection.ExceptionHandler += new EMSExceptionHandler(_HandleException);

            // create the destination
            if (useTopic)
                destination = session.CreateTopic(name);
            else
                destination = session.CreateQueue(name);

            Console.WriteLine("Subscribing to destination: " + name);

            // create the consumer
            msgConsumer = session.CreateConsumer(destination);

            // add the message listener
            msgConsumer.MessageHandler += new EMSMessageHandler(_HandleMessage);

            // start the connection
            connection.Start();

            // Note: when message callback is used, the session
            // creates the dispatcher thread which is not a daemon
            // thread by default. Thus we can quit this method however
            // the application will keep running. It is possible to
            // specify that all session dispatchers are daemon threads.
            while (true) {
                lock(stateLock) {
                    if (stop) {
                        break;
                    }
                    System.Threading.Thread.Sleep(5000);
                }
            }
        } catch (Exception e) {
            Console.WriteLine("Exception in AsyncMsgConsumer: " +
                              e.Message);
            Console.WriteLine(e.StackTrace);
        }
    }

    private void Usage() {
        Console.WriteLine("\nUsage: csAsyncMsgConsumer [options]");
        Console.WriteLine("");
        Console.WriteLine("   where options are:");
        Console.WriteLine("");
        Console.WriteLine("   -server   <server URL> - Server URL, default is local server");
        Console.WriteLine("   -user     <user name>  - user name, default is null");
        Console.WriteLine("   -password <password>   - password, default is null");
        Console.WriteLine("   -topic    <topic-name> - topic name, default is \"topic.sample\"");
        Console.WriteLine("   -queue    <queue-name> - queue name, no default");
        Console.WriteLine("   -help-ssl              - help on ssl parameters");
        Environment.Exit(0);
    }

    private int ParseArgs(String[] args)
    {
        int i = 0;

        while (i < args.Length) {
            if (args[i].CompareTo("-server") == 0) 
            {
                if ((i + 1) >= args.Length) 
                {
                    Usage();
                    return -1;
                }
                serverUrl = args[i+1];
                i += 2;
            }
            else
            if (args[i].CompareTo("-topic") == 0) 
            {
                if ((i + 1) >= args.Length) 
                {
                    Usage();
                    return -1;
                }
                name = args[i+1];
                i += 2;
            } 
            else 
            if (args[i].CompareTo("-queue") == 0) 
            {
                if ((i + 1) >= args.Length) 
                {
                    Usage();
                    return -1;
                }
                name = args[i+1];
                i += 2;
                useTopic = false;
            } 
            else 
            if (args[i].CompareTo("-user") == 0) 
            {
                if ((i + 1) >= args.Length) 
                {
                    Usage();
                    return -1;
                }
                userName = args[i+1];
                i += 2;
            } 
            else 
            if (args[i].CompareTo("-password") == 0) 
            {
                if ((i + 1) >= args.Length) 
                {
                    Usage();
                    return -1;
                }
                password = args[i+1];
                i += 2;
            } 
            else 
            if (args[i].CompareTo("-help") == 0) 
            {
                Usage();
                return -1;
            }
            else 
            if (args[i].CompareTo("-help-ssl")==0)
            {
                tibemsUtilities.sslUsage();
            }
            else
            if(args[i].StartsWith("-ssl"))
            {
                i += 2;
            }         
            else {
                Console.Error.WriteLine("Unrecognized parameter: " + args[i]);
                Usage();
                return -1;
            }
        }
        return 0;
    }

    public void _HandleException(object sender, EMSExceptionEventArgs arg) {
        EMSException e = arg.Exception;
        // print the connection exception status
        Console.WriteLine("Connection Exception: " + e.Message);
        Console.WriteLine(e.StackTrace);
        lock(stateLock) {
            stop = true;
        }
    }

    public void _HandleMessage(object sender, EMSMessageEventArgs arg) {
        try 
        {
            Message msg = arg.Message;
            Console.WriteLine("Received message: " + msg);
        } catch (Exception e) {
            Console.WriteLine("Unexpected exception message callback!");
            Console.WriteLine(e.Message);
        }
    }

    public static void Main(String[] args) {
        csAsyncMsgConsumerUsingDelegate generatedAux = new csAsyncMsgConsumerUsingDelegate(args);
    }
}
