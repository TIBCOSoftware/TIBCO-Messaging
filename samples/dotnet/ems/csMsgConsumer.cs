/* 
 * Copyright (c) 2001-$Date: 2016-12-13 17:00:37 -0600 (Tue, 13 Dec 2016) $ TIBCO Software Inc. 
 * Licensed under a BSD-style license. Refer to [LICENSE]
 * For more information, please contact:
 * TIBCO Software Inc., Palo Alto, California, USA
 * 
 * $Id: csMsgConsumer.cs 90180 2016-12-13 23:00:37Z olivierh $
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
///    -server    <server-url>  Server URL.
///                             If not specified this sample assumes a
///                             serverUrl of "tcp://localhost:7222"
///    -user      <user-name>   User name. Default is null.
///    -password  <password>    User password. Default is null.
///    -topic     <topic-name>  Topic name. Default value is "topic.sample"
///    -queue     <queue-name>  Queue name. No default
///    -ackmode   <mode>        Message acknowledge mode. Default is AUTO.
///                             Other values: DUPS_OK, CLIENT, EXPLICIT_CLIENT,
///                             EXPLICIT_CLIENT_DUPS_OK and NO.
///
/// </summary>

using System;
using TIBCO.EMS;

public class csMsgConsumer : IExceptionListener
{
    String  serverUrl  = null;
    String  userName   = null;
    String  password   = null;
    String  name       = "topic.sample";
    bool    useTopic   = true;
    int     ackMode    = Session.AUTO_ACKNOWLEDGE;
    
    Connection       connection  = null;
    Session          session     = null;
    MessageConsumer  msgConsumer = null;
    Destination      destination = null;
    
    public csMsgConsumer(String[] args) 
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

        Console.WriteLine("\n------------------------------------------------------------------------");
        Console.WriteLine("csMsgConsumer SAMPLE");
        Console.WriteLine("------------------------------------------------------------------------");
        Console.WriteLine("Server....................... " + ((serverUrl != null)?serverUrl:"localhost"));
        Console.WriteLine("User......................... " + ((userName != null)?userName:"(null)"));
        Console.WriteLine("Destination.................. " + name);
        Console.WriteLine("------------------------------------------------------------------------\n");
        
        try
            {
            Run();
        }
        catch (EMSException e)
        {
            Console.Error.WriteLine("Exception in csMsgConsumer: " + e.Message);
            Console.Error.WriteLine(e.StackTrace);
        }
    }
    
    private void Usage() 
    {
        Console.WriteLine("\nUsage: csMsgConsumer [options]");
        Console.WriteLine("");
        Console.WriteLine("   where options are:");
        Console.WriteLine("");
        Console.WriteLine("   -server   <server URL> - Server URL, default is local server");
        Console.WriteLine("   -user     <user name>  - user name, default is null");
        Console.WriteLine("   -password <password>   - password, default is null");
        Console.WriteLine("   -topic    <topic-name> - topic name, default is \"topic.sample\"");
        Console.WriteLine("   -queue    <queue-name> - queue name, no default");
        Console.WriteLine("   -ackmode  <ack-mode>   - acknowledge mode, default is AUTO");
        Console.WriteLine("                            other modes: CLIENT, DUPS_OK, NO,");
        Console.WriteLine("                            EXPLICIT_CLIENT and EXPLICIT_CLIENT_DUPS_OK");
        Console.WriteLine("   -help-ssl              - help on ssl parameters");
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
            if (args[i].CompareTo("-ackmode") == 0) 
            {
                if ((i + 1) >= args.Length)
                    Usage();
                if (args[i+1].CompareTo("AUTO")==0)
                    ackMode = Session.AUTO_ACKNOWLEDGE;
                else if (args[i+1].CompareTo("CLIENT")==0)
                    ackMode = Session.CLIENT_ACKNOWLEDGE;
                else if (args[i+1].CompareTo("DUPS_OK")==0)
                    ackMode = Session.DUPS_OK_ACKNOWLEDGE;
                else if (args[i+1].CompareTo("EXPLICIT_CLIENT")==0)
                    ackMode = Session.EXPLICIT_CLIENT_ACKNOWLEDGE;
                else if (args[i+1].CompareTo("EXPLICIT_CLIENT_DUPS_OK")==0)
                    ackMode = Session.EXPLICIT_CLIENT_DUPS_OK_ACKNOWLEDGE;
                else if (args[i+1].CompareTo("NO")==0)
                    ackMode = Session.NO_ACKNOWLEDGE;
                else
                {
                    Console.Error.WriteLine("Unrecognized -ackmode: " + args[i+1]);
                    Usage();
                }
                i += 2;
            } 
            else 
            if (args[i].CompareTo("-help") == 0) {
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
            else {
                Console.Error.WriteLine("Unrecognized parameter: " + args[i]);
                Usage();
            }
        }
    }
    
    public void OnException(EMSException e) 
    {
        // print the connection exception status
        Console.Error.WriteLine("Connection Exception: " + e.Message);
    }

    private void Run()  {

        Message msg = null;
        
        Console.WriteLine("Subscribing to destination: " + name + "\n");
        
        ConnectionFactory factory = new TIBCO.EMS.ConnectionFactory(serverUrl);
        
        // create the connection
        connection = factory.CreateConnection(userName, password);
        
        // create the session
        session = connection.CreateSession(false, ackMode);
        
        // set the exception listener
        connection.ExceptionListener = this;
        
        // create the destination
        if (useTopic)
            destination = session.CreateTopic(name);
        else
            destination = session.CreateQueue(name);
        
        // create the consumer
        msgConsumer = session.CreateConsumer(destination);
        
        // start the connection
        connection.Start();
        
        // read messages
        while (true)
        {
            // receive the message
            msg = msgConsumer.Receive();
            if (msg == null)
                break;
            
            if (ackMode == Session.CLIENT_ACKNOWLEDGE ||
                ackMode == Session.EXPLICIT_CLIENT_ACKNOWLEDGE ||
                ackMode == Session.EXPLICIT_CLIENT_DUPS_OK_ACKNOWLEDGE)
                msg.Acknowledge();

            Console.WriteLine("Received message: " + msg);
            if (msg is BytesMessage)
            {
                BytesMessage bm = (BytesMessage)msg;
                Console.WriteLine(bm.ReadBoolean());
                Console.WriteLine(bm.ReadChar());
                Console.WriteLine(bm.ReadShort());
                Console.WriteLine(bm.ReadInt());
                Console.WriteLine(bm.ReadLong());
                Console.WriteLine(bm.ReadFloat());
                Console.WriteLine(bm.ReadDouble());
            }
        }
        
        // close the connection
        connection.Close();
    }
    
    public static void Main(String[] args) 
    {
        csMsgConsumer generatedAux = new csMsgConsumer(args);
    }
}
