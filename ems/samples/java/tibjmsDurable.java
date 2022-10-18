
/* 
 * Copyright (c) 2002-$Date: 2016-12-13 17:00:37 -0600 (Tue, 13 Dec 2016) $ TIBCO Software Inc. 
 * Licensed under a BSD-style license. Refer to [LICENSE]
 * For more information, please contact:
 * TIBCO Software Inc., Palo Alto, California, USA
 * 
 * $Id: tibjmsDurable.java 90180 2016-12-13 23:00:37Z olivierh $
 * 
 */

/*
 * This is a simple sample of a topic durable subscriber.
 *
 * This sampe creates durable subscriber on the specified topic and
 * receives and prints all received messages.
 *
 * When this sample started with -unsubscribe parameter,
 * it unsubsribes from specified topic and quits.
 * If -unsubscribe is not specified this sample creates a durable
 * subscriber on the specified topic and receives and prints
 * all received messages. Note that you must provide correct clientID
 * and durable name for this sample to work correctly.
 *
 * Notice that the specified topic should exist in your configuration
 * or your topics configuration file should allow
 * creation of the specified topic.
 *
 * This sample can subscribe to dynamic topics thus it is
 * using Session.createTopic() method in order to obtain
 * the Topic object.
 *
 * Usage:  java tibjmsDurable [options]
 *
 *    where options are:
 *
 *      -server       Server URL.
 *                    If not specified this sample assumes a
 *                    serverUrl of "tcp://localhost:7222"
 *
 *      -user         User name. Default is null.
 *      -password     User password. Default is null.
 *      -topic        Topic name. Default is "topic.sample"
 *      -clientID     Connection Client ID. Default is null.
 *      -durable      Durable name. Default is "subscriber".
 *      -unsubscribe  Unsubscribe and quit.
 *
 *
 */

import javax.jms.*;

public class tibjmsDurable
{
    String      serverUrl       = null;
    String      userName        = null;
    String      password        = null;

    String      topicName       = "topic.sample";
    String      clientID        = null;
    String      durableName     = "subscriber";

    boolean     unsubscribe     = false;

    public tibjmsDurable(String[] args)
    {
        parseArgs(args);

        try
        {
            tibjmsUtilities.initSSLParams(serverUrl,args);
        }
        catch (JMSSecurityException e)
        {
            System.err.println("JMSSecurityException: "+e.getMessage()+", provider="+e.getErrorCode());
            e.printStackTrace();
            System.exit(0);
        }

        if (!unsubscribe && (topicName == null))
        {
            System.err.println("Error: must specify topic name");
            usage();
        }

        if (durableName == null)
        {
            System.err.println("Error: must specify durable subscriber name");
            usage();
        }

        System.err.println("Durable sample.");
        System.err.println("Using clientID:       "+clientID);
        System.err.println("Using Durable Name:   "+durableName);

        try
        {
            ConnectionFactory factory = new com.tibco.tibjms.TibjmsConnectionFactory(serverUrl);

            Connection connection = factory.createConnection(userName,password);

            /* if clientID is specified we must set it right here */
            if (clientID != null)
                connection.setClientID(clientID);

            Session session = connection.createSession();

            if (unsubscribe)
            {
                System.err.println("Unsubscribing durable subscriber "+durableName);
                session.unsubscribe(durableName);
                System.err.println("Successfully unsubscribed "+durableName);
                connection.close();
                return;
            }

            System.err.println("Subscribing to topic: "+topicName);

            /*
             * Use createTopic() to enable subscriptions to dynamic topics.
             */
            javax.jms.Topic topic = session.createTopic(topicName);

            TopicSubscriber subscriber = session.createDurableSubscriber(topic,durableName);

            connection.start();

            /* read topic messages */
            while (true)
            {
                javax.jms.Message message = subscriber.receive();
                if (message == null)
                    break;

                System.err.println("\nReceived message: "+message);
            }

            connection.close();
        }
        catch (JMSException e)
        {
            e.printStackTrace();
            System.exit(0);
        }
    }

    public static void main(String args[])
    {
        tibjmsDurable t = new tibjmsDurable(args);
    }

    void usage()
    {
        System.err.println("\nUsage: java tibjmsDurable [options]");
        System.err.println("");
        System.err.println("   where options are:");
        System.err.println("");
        System.err.println(" -server   <server URL> - EMS server URL, default is local server");
        System.err.println(" -user     <user name>  - user name, default is null");
        System.err.println(" -password <password>   - password, default is null");
        System.err.println(" -topic    <topic-name> - topic name, default is \"topic.sample\"");
        System.err.println(" -clientID <client-id>  - clientID, default is null");
        System.err.println(" -durable  <name>       - durable subscriber name,");
        System.err.println("                          default is \"subscriber\"");
        System.err.println(" -unsubscribe           - unsubscribe and quit");
        System.err.println(" -help-ssl              - help on ssl parameters\n");
        System.exit(0);
    }

    void parseArgs(String[] args)
    {
        int i=0;

        while (i < args.length)
        {
            if (args[i].compareTo("-server")==0)
            {
                if ((i+1) >= args.length) usage();
                serverUrl = args[i+1];
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
            if (args[i].compareTo("-user")==0)
            {
                if ((i+1) >= args.length) usage();
                userName = args[i+1];
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
            if (args[i].compareTo("-durable")==0)
            {
                if ((i+1) >= args.length) usage();
                durableName = args[i+1];
                i += 2;
            }
            else
            if (args[i].compareTo("-clientID")==0)
            {
                if ((i+1) >= args.length) usage();
                clientID = args[i+1];
                i += 2;
            }
            else
            if (args[i].compareTo("-unsubscribe")==0)
            {
                unsubscribe = true;
                i += 1;
            }
            else
            if (args[i].compareTo("-help")==0)
            {
                usage();
            }
            else
            if (args[i].compareTo("-help-ssl")==0)
            {
                tibjmsUtilities.sslUsage();
            }
            else
            if (args[i].startsWith("-ssl"))
            {
                i += 2;
            }
            else
            {
                System.err.println("Unrecognized parameter: "+args[i]);
                usage();
            }
        }
    }

}


