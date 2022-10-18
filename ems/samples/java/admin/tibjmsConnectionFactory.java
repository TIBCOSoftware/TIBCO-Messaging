/* 
 * Copyright (c) 2002-$Date: 2017-03-27 16:01:48 -0500 (Mon, 27 Mar 2017) $ TIBCO Software Inc. 
 * Licensed under a BSD-style license. Refer to [LICENSE]
 * For more information, please contact:
 * TIBCO Software Inc., Palo Alto, California, USA
 * 
 * $Id: tibjmsConnectionFactory.java 92619 2017-03-27 21:01:48Z olivierh $
 * 
 */

/*
 * Example of how to use TIBCO Enterprise Message Service Administration API
 * to create and administer a ConnectionFactory.
 *
 * This sample does the following:
 *
 * 1) Creates a queue ConnectionFactory through the admin connection
 *    to the TIBCO EMS server with parameters:
 *
 *    url = "tcp://7222"
 *    clientID = "ACME_QUEUE"
 *
 *    This ConnectionFactory is bound to the JNDI name "ACME_QUEUE_CONN_FACTORY"
 *
 * 2) Creates a topic ConnectionFactory through the admin connection
 *    to the TIBCO EMS server with parameters:
 *
 *    url = "tcp://7222"
 *    clientID = "ACME_TOPIC"
 *
 *    This ConnectionFactory is bound to the JNDI name "ACME_TOPIC_CONN_FACTORY"
 *
 * 3) Changes the Client ID of the queue ConnectionFactory to "ACME_RETAILERS"
 *    and updates the queue ConnectionFactory
 *
 * 4) Changes the URL of the topic ConnectionFactory to "tcp://7223"
 *    and updates the topic ConnectionFactory
 *
 * 5) Destroys the queue ConnectionFactory
 *
 * 6) Destroys the topic ConnectionFactory
 *
 *
 *
 * Usage:  java tibjmsConnectionFactory  [options]
 *
 *    where options are:
 *
 *      -server     Server URL.
 *                  If not specified this sample assumes a
 *                  serverUrl of null
 *      -user       Admin user name. Default is "admin".
 *      -password   Admin password. Default is null.
 *
 */

import javax.jms.*;

import com.tibco.tibjms.admin.*;

public class tibjmsConnectionFactory
{
    String      serverUrl       = null;
    String      cfUrl           = "tcp://7222";
    String      username        = "admin";
    String      password        = null;
    String      queueClientID   = "ACME_QUEUE";
    String      topicClientID   = "ACME_TOPIC";
    String      queueJNDIName   = "ACME_QUEUE_CONN_FACTORY";
    String      topicJNDIName   = "ACME_TOPIC_CONN_FACTORY";

    public tibjmsConnectionFactory(String[] args) {

        parseArgs(args);

        System.out.println("Admin: ConnectionFactory sample.");
        System.out.println("Using server:     "+serverUrl);

        try {
            //Create the admin connection to the TIBCO EMS server
            TibjmsAdmin admin = new TibjmsAdmin(serverUrl,username,password);

            //Create the queue ConnectionFactory
            ConnectionFactoryInfo cf1 = new ConnectionFactoryInfo(cfUrl,queueClientID,
                                                     DestinationInfo.QUEUE_TYPE,null);
            try {
                admin.createConnectionFactory(queueJNDIName,cf1);
                cf1 = (ConnectionFactoryInfo)admin.lookup(queueJNDIName);
                System.out.println("Created ConnectionFactory: " + cf1);
            }
            catch(TibjmsAdminNameExistsException e) {
                cf1 = (ConnectionFactoryInfo)admin.lookup(queueJNDIName);
                System.out.println("ConnectionFactory already exists: " + cf1);
            }

            //Create the topic ConnectionFactory
            ConnectionFactoryInfo cf2 = new ConnectionFactoryInfo(cfUrl,topicClientID,
                                                     DestinationInfo.TOPIC_TYPE,null);
            try {
                admin.createConnectionFactory(topicJNDIName,cf2);
                cf2 = (ConnectionFactoryInfo)admin.lookup(topicJNDIName);
                System.out.println("Created ConnectionFactory: " + cf2);
            }
            catch(TibjmsAdminNameExistsException e) {
                cf2 = (ConnectionFactoryInfo)admin.lookup(topicJNDIName);
                System.out.println("ConnectionFactory already exists: " + cf2);
            }

            //Change the Client ID of the ACME_QUEUE_CONN_FACTORY
            cf1.setClientID("ACME_RETAILERS");
            admin.updateConnectionFactory(queueJNDIName,cf1);
            cf1 = (ConnectionFactoryInfo)admin.lookup(queueJNDIName);
            System.out.println("Updated ConnectionFactory: " + cf1);

            //Change the URL of the ACME_TOPIC_CONN_FACTORY
            cf2.setURL("tcp://7223");
            admin.updateConnectionFactory(topicJNDIName,cf2);
            cf2 = (ConnectionFactoryInfo)admin.lookup(topicJNDIName);
            System.out.println("Updated ConnectionFactory: " + cf2);

            //Destroy the queue ConnectionFactory
            admin.destroyConnectionFactory(queueJNDIName);
            System.out.println("Destroyed ConnectionFactory: " + queueJNDIName);

            //Destroy the topic ConnectionFactory
            admin.destroyConnectionFactory(topicJNDIName);
            System.out.println("Destroyed ConnectionFactory: " + topicJNDIName);
        }
        catch(TibjmsAdminException e)
        {
            e.printStackTrace();
        }
    }

    public static void main(String args[])
    {
        tibjmsConnectionFactory t = new tibjmsConnectionFactory(args);
    }

    void usage()
    {
        System.err.println("\nUsage: java tibjmsConnectionFactory [options]");
        System.err.println("");
        System.err.println("   where options are:");
        System.err.println("");
        System.err.println(" -server    <server URL>    - JMS server URL, default is local server");
        System.err.println(" -user      <user name>     - admin user name, default is null");
        System.err.println(" -password  <password>      - admin password, default is null");
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


