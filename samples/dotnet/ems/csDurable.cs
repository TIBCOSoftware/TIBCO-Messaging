/* 
 * Copyright (c) 2001-$Date: 2016-12-13 17:00:37 -0600 (Tue, 13 Dec 2016) $ TIBCO Software Inc. 
 * Licensed under a BSD-style license. Refer to [LICENSE]
 * For more information, please contact:
 * TIBCO Software Inc., Palo Alto, California, USA
 * 
 * $Id: csDurable.cs 90180 2016-12-13 23:00:37Z olivierh $
 * 
 */

/// <summary>
/// This is a simple sample of a topic durable subscriber.  It uses the
/// generic connection factory, connection and session classes,
/// unlike csDurableSubscriber, which uses the topic specific classes.
///
/// This sample creates durable subscriber on the specified topic and
/// receives and prints all received messages.
///
/// When this sample is started with -unsubscribe parameter,
/// it unsubsribes from specified topic and quits.
/// If -unsubscribe is not specified this sample creates a durable
/// subscriber on the specified topic and receives and prints
/// all received messages. Note that you must provide correct clientID
/// and durable name for this sample to work correctly.
///
/// Notice that the specified topic should exist in your configuration
/// or your topics configuration file should allow
/// creation of the specified topic.
///
/// This sample can subscribe to dynamic topics thus it is
/// using Session.CreateTopic() method in order to obtain
/// the Topic object.
///
/// Usage:  csDurable [options]
///
///    where options are:
///
///    -server    <server-url>  Server URL.
///                             If not specified this sample assumes a
///                             serverUrl of "tcp://localhost:7222"
///    -user      <user-name>   User name. Default is null.
///    -password  <password>    User password. Default is "queue.sample.browser".
///    -topic    <topic-name>   Topic name. Default is "topic.sample".
///    -clientID <client-id>    Connection client ID. Default is null.
///    -durable  <name>         Durable subscriber name.
///                             Default is "subscriber".
///    -unsubscribe             Unsubscribe and quit.
///
/// </summary>

using System;
using TIBCO.EMS;

public class csDurable 
{
    String serverUrl   = null;
    String userName    = null;
    String password    = null;
    String topicName   = "topic.sample";
    String clientID    = null;
    String durableName = "subscriber";

    bool unsubscribe = false;

    public csDurable(String[] args) 
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

        if (!unsubscribe && (topicName == null)) {
            Console.Error.WriteLine("Error: must specify topic name");
            Usage();
        }

        if (durableName == null) {
            Console.Error.WriteLine("Error: must specify durable subscriber name");
            Usage();
        }

        Console.WriteLine("\n------------------------------------------------------------------------");
        Console.WriteLine("csDurable SAMPLE");
        Console.WriteLine("------------------------------------------------------------------------");
        Console.WriteLine("Server....................... " + ((serverUrl != null)?serverUrl:"localhost"));
        Console.WriteLine("User......................... " + ((userName != null)?userName:"(null)"));
        Console.WriteLine("Topic........................ " + topicName);
        Console.WriteLine("Durable...................... " + durableName);
        Console.WriteLine("Client ID.................... " + ((clientID != null)?clientID:"(null)"));
        Console.WriteLine("------------------------------------------------------------------------\n");

        try {
            ConnectionFactory factory = new TIBCO.EMS.ConnectionFactory(serverUrl);

            Connection connection = factory.CreateConnection(userName, password);

            // if clientID is specified we must set it right here
            if (clientID != null)
                connection.ClientID = clientID;

            Session session = connection.CreateSession(false, Session.AUTO_ACKNOWLEDGE);

            if (unsubscribe) {
                Console.WriteLine("Unsubscribing durable subscriber " + durableName);
                session.Unsubscribe(durableName);
                Console.WriteLine("Successfully unsubscribed " + durableName);
                connection.Close();
                return ;
            }

            Console.WriteLine("Subscribing to topic: " + topicName);

            // Use createTopic() to enable subscriptions to dynamic topics.
            Topic topic = session.CreateTopic(topicName);

            TopicSubscriber subscriber = session.CreateDurableSubscriber(topic, durableName);

            connection.Start();

            // read topic messages
            while (true) {
                Message message = subscriber.Receive();
                if (message == null)
                    break;

                Console.WriteLine("\nReceived message: " + message);
            }

            connection.Close();
        } catch (EMSException e) {
            Console.Error.WriteLine("Exception in csDurable: " + e.Message);
            Console.Error.WriteLine(e.StackTrace);
            Environment.Exit(0);
        }
    }

    public static void Main(String[] args) {
        csDurable t = new csDurable(args);
    }

    private void Usage() {
        Console.WriteLine("\nUsage: csDurable [options]");
        Console.WriteLine("");
        Console.WriteLine("   where options are:");
        Console.WriteLine("");
        Console.WriteLine("   -server   <server URL> - Server URL, default is local server");
        Console.WriteLine("   -user     <user name>  - user name, default is null");
        Console.WriteLine("   -password <password>   - password, default is null");
        Console.WriteLine("   -topic    <topic-name> - topic name, default is \"topic.sample\"");
        Console.WriteLine("   -clientID <client-id>  - clientID, default is null");
        Console.WriteLine("   -durable  <name>       - durable subscriber name,");
        Console.WriteLine("                            default is \"subscriber\"");
        Console.WriteLine("   -unsubscribe           - unsubscribe and quit");
        Console.WriteLine("   -help-ssl              - help on ssl parameters");
        Environment.Exit(0);
    }

    private void ParseArgs(String[] args) {
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
                topicName = args[i+1];
                i += 2;
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
            if (args[i].CompareTo("-durable") == 0) 
            {
                if ((i + 1) >= args.Length)
                    Usage();
                durableName = args[i+1];
                i += 2;
            } 
            else 
            if (args[i].CompareTo("-clientID") == 0) 
            {
                if ((i + 1) >= args.Length)
                    Usage();
                clientID = args[i+1];
                i += 2;
            }
            else 
            if (args[i].CompareTo("-unsubscribe") == 0) 
            {
                unsubscribe = true;
                i += 1;
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
            else {
                Console.Error.WriteLine("Unrecognized parameter: " + args[i]);
                Usage();
            }
        }
    }
}
