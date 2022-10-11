/*
 * Copyright (c) 2017-$Date: 2017-03-27 16:01:48 -0500 (Mon, 27 Mar 2017) $ TIBCO Software Inc. 
 * Licensed under a BSD-style license. Refer to [LICENSE]
 * For more information, please contact:
 * TIBCO Software Inc., Palo Alto, California, USA
 * 
 * $Id: csACL.cs 92619 2017-03-27 21:01:48Z olivierh $
 * 
 */

/// <summary>
/// Example of how to use TIBCO Enterprise Message Service Administration API
/// to create and administer Access Control Lists (ACLs).
///
/// This sample does the following:
/// 1) Creates the topic specified by the "-topic" parameter
///    through the admin connection to the TIBCO EMS server.
///
/// 2) Creates the queue specified by the "-queue" parameter
///    through the admin connection to the TIBCO EMS server.
///
/// 3) Gets the user "mair" from the server. If user "mair" does not exist, it
///    is created.
///
/// 4) Gets the user "ahren" from the server. If user "ahren" does not exist, it
///    is created.
///
/// 5) Creates a Permissions (p1) and sets the following Permissions to "true":
///          Permissions.PUBLISH
///          Permissions.SEND
///          Permissions.BROWSE
///
/// 6) Creates another Permissions (p2) and sets the following Permissions to "true":
///          Permissions.SUBSCRIBE
///          Permissions.RECEIVE
///          Permissions.DURABLE
///          Permissions.BROWSE
///
/// 7) Creates an ACLEntry for user "mair" on the previously created topic
///    with Permissions p1 (see above).
///
/// 8) Creates an ACLEntry for user "mair" on the previously created queue
///    with Permissions p1 (see above).
///
/// 9) Creates an ACLEntry for user "ahren" on the previously created topic
///    with Permissions p2 (see above).
///
/// 10) Creates an ACLEntry for user "ahren" on the previously created queue
///    with Permissions p2 (see above).
///
/// 11) Grants all four of the above ACLs at once.
///
/// 12) Gets all of the ACLs from the server and prints them out.
///
///
///
/// Usage:  csACL  [options]
///
///    where options are:
///
///      -server     Server URL.
///                  If not specified this sample assumes a
///                  serverUrl of null
///
///      -user       Admin user name. Default is "admin".
///      -password   Admin password. Default is null.
///      -queue      Queue name. Default is 'sample.admin.queue'.
///      -topic      Topic name. Default is 'sample.admin.topic'.
///
/// </summary>

using System;
using TIBCO.EMS.ADMIN;

public class csACL
{
    internal string serverUrl = null;
    internal string username  = "admin";
    internal string password  = null;
    internal string topicName = "sample.admin.topic";
    internal string queueName = "sample.admin.queue";

    public csACL(string[] args)
    {
        parseArgs(args);

        Console.WriteLine("Admin: Grant ACLs sample.");
        Console.WriteLine("Using server:          " + serverUrl);
        Console.WriteLine("Using queue:           " + queueName);
        Console.WriteLine("Using topic:           " + topicName);

        try
        {
            // Create the admin connection to the TIBCO EMS server
            Admin admin = new Admin(serverUrl, username, password);

            // Create the TopicInfo and QueueInfo
            TopicInfo topic = new TopicInfo(topicName);
            QueueInfo queue = new QueueInfo(queueName);

            // Create the topic. If it exists, get its TopicInfo from the server.
            try
            {
                topic = admin.CreateTopic(topic);
                Console.WriteLine("Created Topic: " + topic);
            }
            catch (AdminNameExistsException)
            {
                Console.WriteLine("Topic already exists: " + topic);
                topic = admin.GetTopic(topicName);
            }

            // Create the queue. If it exists, get its QueueInfo from the server.
            try
            {
                queue = admin.CreateQueue(queue);
                Console.WriteLine("Created Queue: " + queue);
            }
            catch (AdminNameExistsException)
            {
                Console.WriteLine("Queue already exists: " + queue);
                queue = admin.GetQueue(queueName);
            }

            // Get the users "mair" and "ahren". If they don't exist, create them.
            UserInfo u1 = admin.GetUser("mair");
            UserInfo u2 = admin.GetUser("ahren");

            if (u1 == null)
            {
                u1 = new UserInfo("mair", null);
                u1 = admin.CreateUser(u1);
                Console.WriteLine("Created User: " + u1);
            }

            if (u2 == null)
            {
                u2 = new UserInfo("ahren", null);
                u2 = admin.CreateUser(u2);
                Console.WriteLine("Created User: " + u2);
            }

            // Create the Permissions
            Permissions p1 = new Permissions();
            p1.SetPermission(Permissions.PUBLISH, true);
            p1.SetPermission(Permissions.SEND, true);
            p1.SetPermission(Permissions.BROWSE, true);

            Permissions p2 = new Permissions();
            p2.SetPermission(Permissions.SUBSCRIBE, true);
            p2.SetPermission(Permissions.RECEIVE, true);
            p2.SetPermission(Permissions.DURABLE, true);
            p2.SetPermission(Permissions.BROWSE, true);

            ACLEntry[] acls = new ACLEntry[4];

            // Create ACLEntries for "mair"
            acls[0] = new ACLEntry(topic, u1, p1);
            acls[1] = new ACLEntry(queue, u1, p1);

            // Create ACLEntries for "ahren"
            acls[2] = new ACLEntry(topic, u2, p2);
            acls[3] = new ACLEntry(queue, u2, p2);

            // Grant the ACLs
            admin.Grant(acls);

            // Get all of the ACLs from the server
            acls = admin.ACLEntries;

            if (acls != null)
            {
                Console.WriteLine("ACLs defined in the server:");
                for (int i = 0; i < acls.Length; i++)
                {
                    Console.WriteLine("  " + acls[i]);
                }
            }
            else
            {
                Console.WriteLine("No ACLs defined in the server.");
            }
        }
        catch (AdminException e)
        {
            Console.Error.WriteLine("Exception in csACL: " + e.Message);
            Console.Error.WriteLine(e.StackTrace);
            Environment.Exit(-1);
        }
    }

    public static void Main(string[] args)
    {
        csACL t = new csACL(args);
    }

    internal void usage()
    {
        Console.Error.WriteLine("\nUsage:  csACL [options]");
        Console.Error.WriteLine("");
        Console.Error.WriteLine("   where options are:");
        Console.Error.WriteLine("");
        Console.Error.WriteLine(" -server    <server URL> - JMS server URL, default is local server");
        Console.Error.WriteLine(" -user      <user name>  - admin user name, default is 'admin'");
        Console.Error.WriteLine(" -password  <password>   - admin password, default is null");
        Console.Error.WriteLine(" -queue     <queueName>  - queue to which the ACLs will be granted, default is 'sample.admin.queue'");
        Console.Error.WriteLine(" -topic     <topicName>  - topic to which the ACLs will be granted, default is 'sample.admin.topic'");
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
            if (args[i].CompareTo("-queue") == 0)
            {
                if ((i + 1) >= args.Length)
                {
                    usage();
                }
                queueName = args[i + 1];
                i += 2;
            }
            else
            if (args[i].CompareTo("-topic") == 0)
            {
                if ((i + 1) >= args.Length)
                {
                    usage();
                }
                topicName = args[i + 1];
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
