/* 
 * Copyright (c) 2001-$Date: 2016-12-13 17:00:37 -0600 (Tue, 13 Dec 2016) $ TIBCO Software Inc. 
 * Licensed under a BSD-style license. Refer to [LICENSE]
 * For more information, please contact:
 * TIBCO Software Inc., Palo Alto, California, USA
 * 
 * $Id: csUFOAsyncMsgConsumer.cs 90180 2016-12-13 23:00:37Z olivierh $
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
/// Usage:  csUFOAsyncMsgConsumer [options]
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
using TIBCO.EMS.UFO;
using EMSException = TIBCO.EMS.EMSException;
using IExceptionListener = TIBCO.EMS.IExceptionListener;

public class csUFOAsyncMsgConsumer : IExceptionListener, IMessageListener 
{
    String  serverUrl  = null;
    String  userName   = null;
    String  password   = null;
    String  name       = "topic.sample";
    bool    useTopic   = true;

    ConnectionFactory factory     = null;
    Connection        connection  = null;
    Session           session     = null;
    MessageConsumer   msgConsumer = null;
    Destination       destination = null; 

    public csUFOAsyncMsgConsumer(String[] args) 
    {
        ParseArgs(args);

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
        Console.WriteLine("csUFOAsyncMsgConsumer SAMPLE");
        Console.WriteLine("------------------------------------------------------------------------");
        Console.WriteLine("Server....................... " + ((serverUrl != null)?serverUrl:"localhost"));
        Console.WriteLine("User......................... " + ((userName != null)?userName:"(null)"));
        Console.WriteLine("Destination.................. " + name);
        Console.WriteLine("------------------------------------------------------------------------\n");

        try {
            factory = new ConnectionFactory(serverUrl);

            // create the connection
            connection = factory.CreateConnection(userName, password);

            // create the session
            session = connection.CreateSession(false, Session.AUTO_ACKNOWLEDGE);

            // set the exception listener
            connection.ExceptionListener = this;

            // create the destination
            if (useTopic)
                destination = session.CreateTopic(name);
            else
                destination = session.CreateQueue(name);

            Console.WriteLine("Subscribing to destination: " + name);

            // create the consumer
            msgConsumer = session.CreateConsumer(destination);

            // set the message listener
            msgConsumer.MessageListener = this;

            // start the connection
            connection.Start();

            // Note: the connection exception listener is executed 
            // in a daemon thread. So we need to make sure the main
            // thread is alive.
            while (true)
            {
                try { System.Threading.Thread.Sleep(1000); }
                catch (System.Threading.ThreadInterruptedException) { };
            }
        } 
        catch (Exception e) 
        {
            Console.Error.WriteLine("Exception in AsyncMsgConsumer: " +
                                    e.Message);
            Console.Error.WriteLine(e.StackTrace);
        }
    }

    private void  Usage() 
    {
        Console.WriteLine("\nUsage: csUFOAsyncMsgConsumer [options]");
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

    private void  ParseArgs(String[] args) 
    {
        int i = 0;

        while (i < args.Length) {
            if (args[i].CompareTo("-server") == 0) 
            {
                if ((i + 1) >= args.Length)
                    Usage();
                serverUrl = args[i+1];
                i += 2;
            } 
            else 
            if (args[i].CompareTo("-topic") == 0) 
            {
                if ((i + 1) >= args.Length)
                    Usage();
                name = args[i+1];
                i += 2;
            } 
            else 
            if (args[i].CompareTo("-queue") == 0) 
            {
                if ((i + 1) >= args.Length)
                    Usage();
                name = args[i+1];
                i += 2;
                useTopic = false;
            } 
            else 
            if (args[i].CompareTo("-user") == 0) 
            {
                if ((i + 1) >= args.Length)
                    Usage();
                userName = args[i+1];
                i += 2;
            } 
            else 
            if (args[i].CompareTo("-password") == 0) 
            {
                if ((i + 1) >= args.Length)
                    Usage();
                password = args[i+1];
                i += 2;
            } 
            else 
            if (args[i].CompareTo("-help") == 0) 
            {
                Usage();
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
            else 
            {
                Console.Error.WriteLine("Unrecognized parameter: " + args[i]);
                Usage();
            }
        }
    }

    public void OnException(EMSException e) 
    {
        try
        {
            // print the connection exception message
            Console.Error.WriteLine("CONNECTION EXCEPTION: " + e.Message);
            factory.RecoverConnection(connection);
        }
        catch (EMSException ex)
        {
            Console.Error.WriteLine("CONNECTION RECOVER EXCEPTION: " + ex.Message);
            Environment.Exit(-1);
        }
    }

    public void OnMessage(Message msg)  {
        try 
        {
            Console.WriteLine("Received message: " + msg);
        } 
        catch (Exception e) 
        {
            Console.Error.WriteLine("Unexpected exception message callback!");
            Console.Error.WriteLine(e.StackTrace);
            Environment.Exit(-1);
        }
    }

    public static void Main(String[] args) 
    {
        csUFOAsyncMsgConsumer generatedAux = new csUFOAsyncMsgConsumer(args);
    }
}
