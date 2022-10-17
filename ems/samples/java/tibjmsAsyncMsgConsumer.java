/* 
 * Copyright (c) 2002-$Date: 2016-12-13 17:00:37 -0600 (Tue, 13 Dec 2016) $ TIBCO Software Inc. 
 * Licensed under a BSD-style license. Refer to [LICENSE]
 * For more information, please contact:
 * TIBCO Software Inc., Palo Alto, California, USA
 * 
 * $Id: tibjmsAsyncMsgConsumer.java 90180 2016-12-13 23:00:37Z olivierh $
 * 
 */

/*
 * This is a simple sample of a basic asynchronous
 * tibjmsMsgConsumer.
 *
 * This sampe subscribes to specified destination and
 * receives and prints all received messages.
 *
 * Notice that the specified destination should exist in your configuration
 * or your topics/queues configuration file should allow
 * creation of the specified destination.
 *
 * If this sample is used to receive messages published by
 * tibjmsMsgProducer sample, it must be started prior
 * to running the tibjmsMsgProducer sample.
 *
 * Usage:  java tibjmsAsyncMsgConsumer [options]
 *
 *    where options are:
 *
 *      -server     Server URL.
 *                  If not specified this sample assumes a
 *                  serverUrl of "tcp://localhost:7222"
 *
 *      -user       User name. Default is null.
 *      -password   User password. Default is null.
 *      -topic      Topic name. Default is "topic.sample"
 *      -queue      Queue name. No default
 *
 *
 */

import javax.jms.*;

public class tibjmsAsyncMsgConsumer
       implements ExceptionListener, MessageListener
{
    /*-----------------------------------------------------------------------
     * Parameters
     *----------------------------------------------------------------------*/

    String    serverUrl   = null;
    String    userName    = null;
    String    password    = null;
    String    name        = "topic.sample";
    boolean   useTopic    = true;

    /*-----------------------------------------------------------------------
     * Variables
     *----------------------------------------------------------------------*/
    Connection      connection  = null;
    Session         session     = null;
    MessageConsumer msgConsumer = null;
    Destination     destination = null;


    public tibjmsAsyncMsgConsumer(String[] args)
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

        /* print parameters */
        System.err.println("------------------------------------------------------------------------");
        System.err.println("tibjmsAsyncMsgConsumer Sample");
        System.err.println("------------------------------------------------------------------------");
        System.err.println("Server....................... "+serverUrl != null?serverUrl:"localhost");
        System.err.println("User......................... "+userName != null?userName:"(null)");
        System.err.println("Destination.................. "+name);
        System.err.println("------------------------------------------------------------------------");

        try
        {
            int c;

            ConnectionFactory factory = new com.tibco.tibjms.TibjmsConnectionFactory(serverUrl);

            /* create the connection */
            connection = factory.createConnection(userName,password);

            /* create the session */
            session = connection.createSession();

            /* set the exception listener */
            connection.setExceptionListener(this);

            /* create the destination */
            if (useTopic)
                destination = session.createTopic(name);
            else
                destination = session.createQueue(name);

            System.err.println("Subscribing to destination: "+name);

            /* create the consumer */
            msgConsumer = session.createConsumer(destination);

            /* set the message listener */
            msgConsumer.setMessageListener(this);

            /* start the connection */
            connection.start();

            // Note: when message callback is used, the session
            // creates the dispatcher thread which is not a daemon
            // thread by default. Thus we can quit this method however
            // the application will keep running. It is possible to
            // specify that all session dispatchers are daemon threads.
        }
        catch (Exception e)
        {
            e.printStackTrace();
        }
    }

    /*-----------------------------------------------------------------------
     * usage
     *----------------------------------------------------------------------*/
    void usage()
    {
        System.err.println("\nUsage: java tibjmsAsyncMsgConsumer [options] [ssl options]");
        System.err.println("\n");
        System.err.println("   where options are:\n");
        System.err.println("\n");
        System.err.println(" -server   <server URL> - EMS server URL, default is local server\n");
        System.err.println(" -user     <user name>  - user name, default is null\n");
        System.err.println(" -password <password>   - password, default is null\n");
        System.err.println(" -topic    <topic-name> - topic name, default is \"topic.sample\"\n");
        System.err.println(" -queue    <queue-name> - queue name, no default\n");
        System.err.println(" -help-ssl              - help on ssl parameters\n");
        System.exit(0);
    }

    /*-----------------------------------------------------------------------
     * parseArgs
     *----------------------------------------------------------------------*/
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
                name = args[i+1];
                i += 2;
            }
            else
            if (args[i].compareTo("-queue")==0)
            {
                if ((i+1) >= args.length) usage();
                name = args[i+1];
                i += 2;
                useTopic = false;
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


    /*---------------------------------------------------------------------
     * onException
     *---------------------------------------------------------------------*/
    public void onException(JMSException e)
    {
        /* print the connection exception status */
        System.err.println("CONNECTION EXCEPTION: "+ e.getMessage());
    }

    /*---------------------------------------------------------------------
     * onMessage
     *---------------------------------------------------------------------*/
    public void onMessage(Message msg)
    {
        try
        {
            System.err.println("Received message: " + msg);
        }
        catch (Exception e)
        {
            System.err.println("Unexpected exception in the message callback!");
            e.printStackTrace();
            System.exit(-1);
        }
    }

    /*-----------------------------------------------------------------------
     * main
     *----------------------------------------------------------------------*/
    public static void main(String[] args)
    {
        new tibjmsAsyncMsgConsumer(args);
    }

}


