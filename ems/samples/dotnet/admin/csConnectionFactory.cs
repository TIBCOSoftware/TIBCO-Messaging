/*
 * Copyright (c) 2017-$Date: 2017-03-27 16:01:48 -0500 (Mon, 27 Mar 2017) $ TIBCO Software Inc. 
 * Licensed under a BSD-style license. Refer to [LICENSE]
 * For more information, please contact:
 * TIBCO Software Inc., Palo Alto, California, USA
 * 
 * $Id: csConnectionFactory.cs 92619 2017-03-27 21:01:48Z olivierh $
 * 
 */

/// <summary>
/// Example of how to use TIBCO Enterprise Message Service Administration API
/// to create and administer a ConnectionFactory.
///
/// This sample does the following:
///
/// 1) Creates a queue ConnectionFactory through the admin connection
///    to the TIBCO EMS server with parameters:
///
///    url = "tcp://7222"
///    clientID = "ACME_QUEUE"
///
///    This ConnectionFactory is bound to the JNDI name "ACME_QUEUE_CONN_FACTORY"
///
/// 2) Creates a topic ConnectionFactory through the admin connection
///    to the TIBCO EMS server with parameters:
///
///    url = "tcp://7222"
///    clientID = "ACME_TOPIC"
///
///    This ConnectionFactory is bound to the JNDI name "ACME_TOPIC_CONN_FACTORY"
///
/// 3) Changes the Client ID of the queue ConnectionFactory to "ACME_RETAILERS"
///    and updates the queue ConnectionFactory
///
/// 4) Changes the URL of the topic ConnectionFactory to "tcp://7223"
///    and updates the topic ConnectionFactory
///
/// 5) Destroys the queue ConnectionFactory
///
/// 6) Destroys the topic ConnectionFactory
///
/// Usage:   csConnectionFactory  [options]
///
///    where options are:
///
///      -server     Server URL.
///                  If not specified this sample assumes a
///                  serverUrl of null
///      -user       Admin user name. Default is "admin".
///      -password   Admin password. Default is null.
///
/// </summary>

using System;
using TIBCO.EMS.ADMIN;

public class csConnectionFactory
{
    internal string serverUrl     = null;
    internal string cfUrl         = "tcp://7222";
    internal string username      = "admin";
    internal string password      = null;
    internal string queueClientID = "ACME_QUEUE";
    internal string topicClientID = "ACME_TOPIC";
    internal string queueJNDIName = "ACME_QUEUE_CONN_FACTORY";
    internal string topicJNDIName = "ACME_TOPIC_CONN_FACTORY";

    public csConnectionFactory(string[] args)
    {
        parseArgs(args);
        Console.WriteLine("Admin: ConnectionFactory sample.");
        Console.WriteLine("Using server:     " + serverUrl);

        try
        {
            // Create the admin connection to the TIBCO EMS server
            Admin admin = new Admin(serverUrl, username, password);

            // Create the queue ConnectionFactory
            ConnectionFactoryInfo cf1 = new ConnectionFactoryInfo(cfUrl, queueClientID, DestinationType.Queue, null);

            try
            {
                admin.CreateConnectionFactory(queueJNDIName, cf1);
                cf1 = (ConnectionFactoryInfo)admin.Lookup(queueJNDIName);
                Console.WriteLine("Created ConnectionFactory: " + cf1);
            }
            catch (AdminNameExistsException)
            {
                cf1 = null;
                cf1 = (ConnectionFactoryInfo)admin.Lookup(queueJNDIName);
                Console.WriteLine("ConnectionFactory already exists: " + cf1);
            }

            // Create the topic ConnectionFactory
            ConnectionFactoryInfo cf2 = new ConnectionFactoryInfo(cfUrl, topicClientID, DestinationType.Topic, null);

            try
            {
                admin.CreateConnectionFactory(topicJNDIName, cf2);
                cf2 = (ConnectionFactoryInfo)admin.Lookup(topicJNDIName);
                Console.WriteLine("Created ConnectionFactory: " + cf2);
            }
            catch (AdminNameExistsException)
            {
                cf2 = null;
                cf2 = (ConnectionFactoryInfo)admin.Lookup(topicJNDIName);
                Console.WriteLine("ConnectionFactory already exists: " + cf2);
            }

            // Change the Client ID of the ACME_QUEUE_CONN_FACTORY
            cf1.ClientID = "ACME_RETAILERS";
            admin.UpdateConnectionFactory(queueJNDIName, cf1);
            cf1 = (ConnectionFactoryInfo)admin.Lookup(queueJNDIName);
            Console.WriteLine("Updated ConnectionFactory: " + cf1);

            // Change the URL of the ACME_TOPIC_CONN_FACTORY
            cf2.URL = "tcp://7223";
            admin.UpdateConnectionFactory(topicJNDIName, cf2);
            cf2 = (ConnectionFactoryInfo)admin.Lookup(topicJNDIName);
            Console.WriteLine("Updated ConnectionFactory: " + cf2);

            // Destroy the queue ConnectionFactory
            admin.DestroyConnectionFactory(queueJNDIName);
            Console.WriteLine("Destroyed ConnectionFactory: " + queueJNDIName);

            // Destroy the topic ConnectionFactory
            admin.DestroyConnectionFactory(topicJNDIName);
            Console.WriteLine("Destroyed ConnectionFactory: " + topicJNDIName);
        }
        catch (AdminException e)
        {
            Console.Error.WriteLine("Exception in csConnectionFactory: " + e.Message);
            Console.Error.WriteLine(e.StackTrace);
            Environment.Exit(-1);
        }
    }

    public static void Main(string[] args)
    {
        csConnectionFactory t = new csConnectionFactory(args);
    }

    internal void usage()
    {
        Console.Error.WriteLine("\nUsage:  csConnectionFactory [options]");
        Console.Error.WriteLine("");
        Console.Error.WriteLine("   where options are:");
        Console.Error.WriteLine("");
        Console.Error.WriteLine(" -server    <server URL>    - JMS server URL, default is local server");
        Console.Error.WriteLine(" -user      <user name>     - admin user name, default is 'admin'");
        Console.Error.WriteLine(" -password  <password>      - admin password, default is null");
        Environment.Exit(0);
    }

    internal void parseArgs(string[] args)
    {
        int i = 0;

        while (i < args.Length)
        {
            if (args[i].CompareTo("-server") == 0)
            {
                if ((i + 1) >= args.Length)
                {
                    usage();
                }
                serverUrl = args[i + 1];
                i += 2;
            }
            else
            if (args[i].CompareTo("-user") == 0)
            {
                if ((i + 1) >= args.Length)
                {
                    usage();
                }
                username = args[i + 1];
                i += 2;
            }
            else
            if (args[i].CompareTo("-password") == 0)
            {
                if ((i + 1) >= args.Length)
                {
                    usage();
                }
                password = args[i + 1];
                i += 2;
            }
            else
            if (args[i].CompareTo("-help") == 0)
            {
                usage();
            }
            else
            {
                Console.WriteLine("Unrecognized parameter: " + args[i]);
                usage();
            }
        }
    }
}
