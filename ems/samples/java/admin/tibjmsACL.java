/* 
 * Copyright (c) 2002-$Date: 2017-03-27 16:01:48 -0500 (Mon, 27 Mar 2017) $ TIBCO Software Inc. 
 * Licensed under a BSD-style license. Refer to [LICENSE]
 * For more information, please contact:
 * TIBCO Software Inc., Palo Alto, California, USA
 * 
 * $Id: tibjmsACL.java 92619 2017-03-27 21:01:48Z olivierh $
 * 
 */

/*
 * Example of how to use TIBCO Enterprise Message Service Administration API
 * to create and administer Access Control Lists (ACLs).
 *
 * This sample does the following:
 * 1) Creates the topic specified by the "-topic" parameter
 *    through the admin connection to the TIBCO EMS server.
 *
 * 2) Creates the queue specified by the "-queue" parameter
 *    through the admin connection to the TIBCO EMS server.
 *
 * 3) Gets the user "mair" from the server. If user "mair" does not exist, it
 *    is created.
 *
 * 4) Gets the user "ahren" from the server. If user "ahren" does not exist, it
 *    is created.
 *
 * 5) Creates a Permissions (p1) and sets the following Permissions to "true":
 *          Permissions.PUBLISH
 *          Permissions.SEND
 *          Permissions.BROWSE
 *
 * 6) Creates another Permissions (p2) and sets the following Permissions to "true":
 *          Permissions.SUBSCRIBE
 *          Permissions.RECEIVE
 *          Permissions.DURABLE
 *          Permissions.BROWSE
 *
 * 7) Creates an ACLEntry for user "mair" on the previously created topic
 *    with Permissions p1 (see above).
 *
 * 8) Creates an ACLEntry for user "mair" on the previously created queue
 *    with Permissions p2 (see above).
 *
 * 9) Creates an ACLEntry for user "ahren" on the previously created topic
 *    with Permissions p1 (see above).
 *
 * 10) Creates an ACLEntry for user "ahren" on the previously created queue
 *    with Permissions p2 (see above).
 *
 * 11) Grants all four of the above ACLs at once.
 *
 * 12) Gets all of the ACLs from the server and prints them out.
 *
 *
 *
 * Usage:  java tibjmsACL  [options]
 *
 *    where options are:
 *
 *      -server     Server URL.
 *                  If not specified this sample assumes a
 *                  serverUrl of null
 *
 *      -user       Admin user name. Default is "admin".
 *      -password   Admin password. Default is null.
 *      -queue      Queue name. Default is 'sample.admin.queue'.
 *      -topic      Topic name. Default is 'sample.admin.topic'.
 *
 */

import javax.jms.*;

import com.tibco.tibjms.admin.*;

public class tibjmsACL
{
    String      serverUrl       = null;
    String      username        = "admin";
    String      password        = null;
    String      topicName       = "sample.admin.topic";
    String      queueName       = "sample.admin.queue";

    public tibjmsACL(String[] args) {

        parseArgs(args);

        System.out.println("Admin: Grant ACLs sample.");
        System.out.println("Using server:          "+serverUrl);
        System.out.println("Using queue:           "+queueName);
        System.out.println("Using topic:           "+topicName);

        try {
            //Create the admin connection to the TIBCO EMS server
            TibjmsAdmin admin = new TibjmsAdmin(serverUrl,username,password);

            //Create the TopicInfo and QueueInfo
            TopicInfo topic = new TopicInfo(topicName);
            QueueInfo queue = new QueueInfo(queueName);

            //Create the topic. If it exists, get it's TopicInfo from the server
            try {
                topic = admin.createTopic(topic);
                System.out.println("Created Topic: " + topic);
            }
            catch(TibjmsAdminNameExistsException e) {
                System.out.println("Topic already exists: " + topic);
                topic = admin.getTopic(topicName);
            }

            //Create the queue. If it exists, get it's QueueInfo from the server
            try {
                queue = admin.createQueue(queue);
                System.out.println("Created Queue: " + queue);
            }
            catch(TibjmsAdminNameExistsException e) {
                System.out.println("Queue already exists: " + queue);
                queue = admin.getQueue(queueName);
            }


            //Get the users "mair" and "ahren". If they don't exist, create them.
            UserInfo u1 = admin.getUser("mair");
            UserInfo u2 = admin.getUser("ahren");

            if(u1 == null) {
                u1 = new UserInfo("mair",null);
                u1 = admin.createUser(u1);
                System.out.println("Created User: " + u1);
            }

            if(u2 == null) {
                u2 = new UserInfo("ahren",null);
                u2 = admin.createUser(u2);
                System.out.println("Created User: " + u2);
            }

            //Create the Permissions
            Permissions p1 = new Permissions();
            p1.setPermission(Permissions.PUBLISH,true);
            p1.setPermission(Permissions.SEND,true);
            p1.setPermission(Permissions.BROWSE,true);

            Permissions p2 = new Permissions();
            p2.setPermission(Permissions.SUBSCRIBE,true);
            p2.setPermission(Permissions.RECEIVE,true);
            p2.setPermission(Permissions.DURABLE,true);
            p2.setPermission(Permissions.BROWSE,true);

            ACLEntry[] acls = new ACLEntry[4];

            //Create ACLEntries for "mair"
            acls[0] = new ACLEntry(topic,u1,p1);
            acls[1] = new ACLEntry(queue,u1,p1);

            //Create ACLEntries for "ahren"
            acls[2] = new ACLEntry(topic,u2,p2);
            acls[3] = new ACLEntry(queue,u2,p2);

            //Grant the ACLs
            admin.grant(acls);

            //Get all of the ACLs from the server
            acls = admin.getACLEntries();

            if (acls != null)
            {
                System.out.println("ACLs defined in the server:");
                for(int i=0; i<acls.length; i++) {
                    System.out.println("  " + acls[i]);
                }
            }
            else
            {
                System.out.println("No ACLs defined in the server.");
            }
        }
        catch(TibjmsAdminException e)
        {
            e.printStackTrace();
        }
    }

    public static void main(String args[])
    {
        tibjmsACL t = new tibjmsACL(args);
    }

    void usage()
    {
        System.err.println("\nUsage: java tibjmsACL [options]");
        System.err.println("");
        System.err.println("   where options are:");
        System.err.println("");
        System.err.println(" -server    <server URL> - JMS server URL, default is local server");
        System.err.println(" -user      <user name>  - admin user name, default is null");
        System.err.println(" -password  <password>   - admin password, default is null");
        System.err.println(" -queue     <queueName>  - queue to which the ACLs will be granted, default is 'sample.admin.queue'");
        System.err.println(" -topic     <topicName>  - topic to which the ACLs will be granted, default is 'sample.admin.topic'");
        System.exit(0);
    }

    void parseArgs(String[] args)
    {
        int i=0;

        while(i < args.length)
        {
            if (args[i].compareTo("-server")==0)
            {
                if ((i+1) >= args.length) usage();
                serverUrl = args[i+1];
                i += 2;
            }
            else
            if (args[i].compareTo("-user")==0)
            {
                if ((i+1) >= args.length) usage();
                username = args[i+1];
                i += 2;
            }
            else
            if (args[i].compareTo("-password")==0)
            {
                if ((i+1) >= args.length) usage();
                password = args[i+1];
                i += 2;
            }
            else
            if (args[i].compareTo("-queue")==0)
            {
                if ((i+1) >= args.length) usage();
                queueName = args[i+1];
                i += 2;
            }
            else
            if (args[i].compareTo("-topic")==0)
            {
                if ((i+1) >= args.length) usage();
                topicName = args[i+1];
                i += 2;
            }
            else
            if (args[i].compareTo("-help")==0)
            {
                usage();
            }
            else
            {
                System.out.println("Unrecognized parameter: "+args[i]);
                usage();
            }
        }
    }

}

