/* 
 * Copyright (c) 2002-$Date: 2016-12-13 17:00:37 -0600 (Tue, 13 Dec 2016) $ TIBCO Software Inc. 
 * Licensed under a BSD-style license. Refer to [LICENSE]
 * For more information, please contact:
 * TIBCO Software Inc., Palo Alto, California, USA
 * 
 * $Id: tibjmsXAMsgConsumer.java 90180 2016-12-13 23:00:37Z olivierh $
 * 
 */

/*
 * This sample demonstrates how to use the tibjmsXAResource object
 * for consuming messages.
 *
 * Note that the tibjmsXAResource calls are normally made by the
 * Transaction Manager, not directly by the application being managed
 * by the TM.
 *
 * This sample subscribes to specified destination and
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
 * Usage:  java tibjmsXAMsgConsumer [options]
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
import javax.transaction.xa.*;

/* only used to generate unique XID */
import java.rmi.server.*;

public class tibjmsXAMsgConsumer
       implements ExceptionListener
{
    /*-----------------------------------------------------------------------
     * Parameters
     *----------------------------------------------------------------------*/

     String           serverUrl   = null;
     String           userName    = null;
     String           password    = null;
     String           name        = "topic.sample";
     boolean          useTopic    = true;

    /*-----------------------------------------------------------------------
     * Variables
     *----------------------------------------------------------------------*/
    XAConnection    connection  = null;
    XASession       session     = null;
    MessageConsumer msgConsumer = null;
    Destination     destination = null;

    public tibjmsXAMsgConsumer(String[] args)
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
        System.err.println("\n------------------------------------------------------------------------");
        System.err.println("tibjmsXAMsgConsumer SAMPLE");
        System.err.println("------------------------------------------------------------------------");
        System.err.println("Server....................... "+((serverUrl != null)?serverUrl:"localhost"));
        System.err.println("User......................... "+((userName != null)?userName:"(null)"));
        System.err.println("Destination.................. "+name);
        System.err.println("------------------------------------------------------------------------\n\n");

        try
        {
            run();
        }
        catch (JMSException e)
        {
            e.printStackTrace();
        }
    }


    /*-----------------------------------------------------------------------
     * usage
     *----------------------------------------------------------------------*/
    void usage()
    {
        System.err.println("\nUsage: tibjmsXAMsgConsumer [options] [ssl options]");
        System.err.println("");
        System.err.println("   where options are:");
        System.err.println("");
        System.err.println(" -server   <server URL> - EMS server URL, default is local server");
        System.err.println(" -user     <user name>  - user name, default is null");
        System.err.println(" -password <password>   - password, default is null");
        System.err.println(" -topic    <topic-name> - topic name, default is \"topic.sample\"");
        System.err.println(" -queue    <queue-name> - queue name, no default");
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
        System.err.println("CONNECTION EXCEPTION: " + e.getMessage());
    }

    /*-----------------------------------------------------------------------
     * run
     *----------------------------------------------------------------------*/
     void run() throws JMSException
     {
        Message       msg         = null;
        boolean       receive     = true;
        String        msgType     = "UNKNOWN";

        System.err.println("Subscribing to destination: '"+name+"'\n");

        XAConnectionFactory factory = new com.tibco.tibjms.TibjmsXAConnectionFactory(serverUrl);

        /* create the connection */
        connection = factory.createXAConnection(userName,password);

        /* create the session */
        session = connection.createXASession();

        /* set the exception listener */
        connection.setExceptionListener(this);

        /* create the destination */
        if (useTopic)
            destination = session.createTopic(name);
        else
            destination = session.createQueue(name);

        /* create the consumer */
        msgConsumer = session.createConsumer(destination);

        /* get the XAResource for the XASession */
        XAResource xaResource = session.getXAResource();

        /* start the connection */
        connection.start();

        /* create a transaction id */
        UID uid = new java.rmi.server.UID();
        Xid xid = new com.tibco.tibjms.TibjmsXid(0, uid.toString(),
            "branch");
        
        /* read messages */
        while (receive)
        {
            /* start a transaction */
            try
            {
                xaResource.start(xid, XAResource.TMNOFLAGS);
            }
            catch (XAException e)
            {
                System.err.println("XAException: " + " errorCode=" + e.errorCode);
                e.printStackTrace();
                System.exit(0);
            }

            /* receive the message */
            msg = msgConsumer.receive();

            if (msg == null)
               break;

            if (msg instanceof TextMessage)
               System.out.println("Received TEXT message: " + ((TextMessage)msg).getText());
            else
                System.err.println("Received message: "+ msg);
            
            /* end and prepare the transaction */
            try
            {
                xaResource.end(xid, XAResource.TMSUCCESS);
                xaResource.prepare(xid);
            }
            catch (XAException e)
            {
                System.err.println("XAException: " + " errorCode=" + e.errorCode);
                e.printStackTrace();
                
                Throwable cause = e.getCause();
                if (cause != null)
                {
                    System.err.println("cause: ");
                    cause.printStackTrace();
                }
                
                try
                {
                    xaResource.rollback(xid);
                }
                catch (XAException re) {}
                
                System.exit(0);
            }
            
            /* commit the transaction */
            try
            {
                xaResource.commit(xid, false);
            }
            catch (XAException e)
            {
                System.err.println("XAException: " + " errorCode=" + e.errorCode);
                e.printStackTrace();
                
                Throwable cause = e.getCause();
                if (cause != null)
                {
                    System.err.println("cause: ");
                    cause.printStackTrace();
                }
                
                System.exit(0);
            }
        }

        /* close the connection */
        connection.close();
    }

    /*-----------------------------------------------------------------------
     * main
     *----------------------------------------------------------------------*/
    public static void main(String[] args)
    {
        new tibjmsXAMsgConsumer(args);
    }
}
