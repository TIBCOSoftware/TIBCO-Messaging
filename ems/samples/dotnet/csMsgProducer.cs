/* 
 * Copyright (c) 2001-$Date: 2016-12-13 17:00:37 -0600 (Tue, 13 Dec 2016) $ TIBCO Software Inc. 
 * Licensed under a BSD-style license. Refer to [LICENSE]
 * For more information, please contact:
 * TIBCO Software Inc., Palo Alto, California, USA
 * 
 * $Id: csMsgProducer.cs 90180 2016-12-13 23:00:37Z olivierh $
 * 
 */

/// <summary>
///  This is a sample of a basic csMsgProducer.
/// 
///  This samples publishes specified message(s) on a specified
///  destination and quits.
/// 
///  Notice that the specified destination should exist in your configuration
///  or your topics/queues configuration file should allow
///  creation of the specified topic or queue. Sample configuration supplied with
///  the TIBCO EMS allows creation of any destination.
/// 
///  If this sample is used to publish messages into
///  csMsgConsumer sample, the csMsgConsumer
///  sample must be started first.
/// 
///  If -topic is not specified this sample will use a topic named
///  "topic.sample".
/// 
///  Usage:  csMsgProducer  [options]
///                                <message-text1>
///                                ...
///                                <message-textN>
/// 
///   where options are:
/// 
///    -server    <server-url>  Server URL.
///                             If not specified this sample assumes a
///                             serverUrl of "tcp://localhost:7222"
///    -user      <user-name>   User name. Default is null.
///    -password  <password>    User password. Default is null.
///    -topic     <topic-name>  Topic name. Default value is "topic.sample"
///    -queue     <queue-name>  Queue name. No default
/// 
/// </summary>

using System;
using System.Collections;
using TIBCO.EMS;

public class csMsgProducer
{
    String     serverUrl = null;
    String     userName  = null;
    String     password  = null;
    String     name      = "topic.sample";
    ArrayList  data      = new ArrayList();
    bool       useTopic  = true;
    bool       useAsync  = false;

    Connection       connection  = null;
    Session          session     = null;
    MessageProducer  msgProducer = null;
    Destination      destination = null;

    EMSCompletionListener completionListener = null;
    
    class EMSCompletionListener : ICompletionListener
    {
        // Note:  Use caution when modifying a message in a completion
        // listener to avoid concurrent message use.

        public void OnCompletion(Message msg)
        {
            try 
            {
                System.Console.WriteLine("Successfully sent message {0}.",
                    ((TextMessage)msg).Text);
            }
            catch (EMSException e)
            {
                System.Console.WriteLine("Error retrieving message text.");
                System.Console.WriteLine("Message: " + e.Message);
                System.Console.WriteLine(e.StackTrace);
            }
        }

        public void OnException(Message msg, Exception ex)
        {
            try 
            {
                System.Console.WriteLine("Error sending message {0}.",
                        ((TextMessage)msg).Text);
            }
            catch (EMSException e)
            {
                System.Console.WriteLine("Error retrieving message text.");
                System.Console.WriteLine("Message: " + e.Message);
                System.Console.WriteLine(e.StackTrace);
            }
            
            System.Console.WriteLine("Message: " + ex.Message);
            System.Console.WriteLine(ex.StackTrace);
        }
        
    }
    
    public csMsgProducer(String[] args) 
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
        Console.WriteLine("csMsgProducer SAMPLE");
        Console.WriteLine("------------------------------------------------------------------------");
        Console.WriteLine("Server....................... " + ((serverUrl != null)?serverUrl:"localhost"));
        Console.WriteLine("User......................... " + ((userName != null)?userName:"(null)"));
        Console.WriteLine("Destination.................. " + name);
        Console.WriteLine("Send Asynchronously.......... " + useAsync);
        Console.WriteLine("Message Text................. ");

        for (int i = 0; i < data.Count; i++)
        {
            Console.WriteLine(data[i]);
        }
        Console.WriteLine("------------------------------------------------------------------------\n");
        
        try
        {
            TextMessage msg;
            int i;
            
            if (data.Count == 0)
            {
                Console.Error.WriteLine("Error: must specify at least one message text\n");
                Usage();
            }
            
            Console.WriteLine("Publishing to destination '" + name + "'\n");
            
            ConnectionFactory factory = new TIBCO.EMS.ConnectionFactory(serverUrl);
            
            connection = factory.CreateConnection(userName, password);
            
            // create the session
            session = connection.CreateSession(false, Session.AUTO_ACKNOWLEDGE);
            
            // create the destination
            if (useTopic)
                destination = session.CreateTopic(name);
            else
                destination = session.CreateQueue(name);
            
            // create the producer
            msgProducer = session.CreateProducer(null);

            if (useAsync)
                completionListener = new EMSCompletionListener();
            
            // publish messages
            for (i = 0; i < data.Count; i++)
            {
                // create text message
                msg = session.CreateTextMessage();
                
                // set message text
                msg.Text = (String) data[i];
                
                // publish message
                if (useAsync)
                    msgProducer.Send(destination, msg, completionListener);
                else
                    msgProducer.Send(destination, msg);
                
                Console.WriteLine("Published message: " + data[i]);
            }
            
            // close the connection
            connection.Close();
        }
        catch (EMSException e)
        {
            Console.Error.WriteLine("Exception in csMsgProducer: " + e.Message);
            Console.Error.WriteLine(e.StackTrace);
            Environment.Exit(-1);
        }
    }
    
    private void Usage() 
    {
        Console.WriteLine("\nUsage: csMsgProducer [options]");
        Console.WriteLine("                       <message-text-1>");
        Console.WriteLine("                       [<message-text-2>] ...\n");
        Console.WriteLine("");
        Console.WriteLine("   where options are:");
        Console.WriteLine("");
        Console.WriteLine("   -server   <server URL>  - Server URL, default is local server");
        Console.WriteLine("   -user     <user name>   - user name, default is null");
        Console.WriteLine("   -password <password>    - password, default is null");
        Console.WriteLine("   -topic    <topic-name>  - topic name, default is \"topic.sample\"");
        Console.WriteLine("   -queue    <queue-name>  - queue name, no default");
        Console.WriteLine("   -async                   - send asynchronously, default is false");
        Console.WriteLine("   -help-ssl               - help on ssl parameters");
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
                serverUrl = args[i + 1];
                i += 2;
            } 
            else 
            if (args[i].CompareTo("-topic") == 0) 
            {
                if ((i + 1) >= args.Length)
                    Usage();
                name = args[i + 1];
                i += 2;
            } 
            else 
            if (args[i].CompareTo("-queue") == 0) 
            {
                if ((i + 1) >= args.Length)
                    Usage();
                name = args[i + 1];
                i += 2;
                useTopic = false;
            } 
            else 
            if (args[i].CompareTo("-async") == 0)
            {
                i += 1;
                useAsync = true;
            }
            else
            if (args[i].CompareTo("-user") == 0) 
            {
                if ((i + 1) >= args.Length)
                    Usage();
                userName = args[i + 1];
                i += 2;
            } 
            else 
            if (args[i].CompareTo("-password") == 0) 
            {
                if ((i + 1) >= args.Length)
                    Usage();
                password = args[i + 1];
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
                data.Add(args[i]);
                i++;
            }
        }
    }
    
    public static void Main(String[] args) 
    {
         new csMsgProducer(args);
    }
}
