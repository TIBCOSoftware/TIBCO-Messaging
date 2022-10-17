/* 
 * Copyright (c) 2002-$Date: 2016-12-13 17:00:37 -0600 (Tue, 13 Dec 2016) $ TIBCO Software Inc. 
 * Licensed under a BSD-style license. Refer to [LICENSE]
 * For more information, please contact:
 * TIBCO Software Inc., Palo Alto, California, USA
 * 
 * $Id: tibjmsXAMsgProducer.java 90180 2016-12-13 23:00:37Z olivierh $
 * 
 */

/*
 * This sample demonstrates how to use the tibjmsXAResource object
 * for sending messages.
 *
 * Note that the tibjmsXAResource calls are normally made by the
 * Transaction Manager, not directly by the application being managed
 * by the TM.
 *
 * This samples publishes specified message(s) on a specified
 * destination and quits.
 *
 * Notice that the specified destination should exist in your configuration
 * or your topics/queues configuration file should allow
 * creation of the specified topic or queue. Sample configuration supplied with
 * the TIBCO Enterprise Message Service distribution allows creation of any
 * destination.
 *
 * If this sample is used to publish messages into
 * tibjmsMsgConsumer sample, the tibjmsMsgConsumer
 * sample must be started first.
 *
 * If -topic is not specified this sample will use a topic named
 * "topic.sample".
 *
 * Usage:  java tibjmsMsgProducer  [options]
 *                               <message-text1>
 *                               ...
 *                               <message-textN>
 *
 *  where options are:
 *
 *   -server    <server-url>  Server URL.
 *                            If not specified this sample assumes a
 *                            serverUrl of "tcp://localhost:7222"
 *   -user      <user-name>   User name. Default is null.
 *   -password  <password>    User password. Default is null.
 *   -topic     <topic-name>  Topic name. Default value is "topic.sample"
 *   -queue     <queue-name>  Queue name. No default
 *
 */

import java.util.*;
import javax.jms.*;
import javax.transaction.xa.*;

/* only used to generate unique XID */
import java.rmi.server.*;

public class tibjmsXAMsgProducer
{
    /*-----------------------------------------------------------------------
     * Parameters
     *----------------------------------------------------------------------*/

    String          serverUrl    = null;
    String          userName     = null;
    String          password     = null;
    String          name         = "topic.sample";
    Vector<String>  data         = new Vector<String>();
    boolean         useTopic     = true;

    /*-----------------------------------------------------------------------
     * Variables
     *----------------------------------------------------------------------*/
    XAConnection    connection   = null;
    XASession       session      = null;
    MessageProducer msgProducer  = null;
    Destination     destination  = null;

    public tibjmsXAMsgProducer(String[] args)
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
        System.err.println("tibjmsXAMsgProducer SAMPLE");
        System.err.println("------------------------------------------------------------------------");
        System.err.println("Server....................... "+((serverUrl != null)?serverUrl:"localhost"));
        System.err.println("User......................... "+((userName != null)?userName:"(null)"));
        System.err.println("Destination.................. "+name);
        System.err.println("Message Text................. ");
        for (int i=0;i<data.size();i++)
        {
            System.err.println("\t"+data.elementAt(i));
        }
        System.err.println("------------------------------------------------------------------------\n");

        try
        {
            run();
        }
        catch (JMSException e)
        {
            e.printStackTrace();
            System.exit(-1);
        }
    }

    /*-----------------------------------------------------------------------
     * usage
     *----------------------------------------------------------------------*/
    private void usage()
    {
        System.err.println("\nUsage: java tibjmsXAMsgProducer [options] [ssl options]");
        System.err.println("                                  <message-text-1>");
        System.err.println("                                  [<message-text-2>] ...");
        System.err.println("\n");
        System.err.println("   where options are:");
        System.err.println("");
        System.err.println("   -server   <server URL>  - EMS server URL, default is local server");
        System.err.println("   -user     <user name>   - user name, default is null");
        System.err.println("   -password <password>    - password, default is null");
        System.err.println("   -topic    <topic-name>  - topic name, default is \"topic.sample\"");
        System.err.println("   -queue    <queue-name>  - queue name, no default");
        System.err.println("   -help-ssl               - help on ssl parameters\n");
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
                data.addElement(args[i]);
                i++;
            }
        }
    }


    /*-----------------------------------------------------------------------
     * run
    *----------------------------------------------------------------------*/
    private void run() throws JMSException
    {
        TextMessage msg;
        int         i;

        if (data.size() == 0)
        {
            System.err.println("***Error: must specify at least one message text\n");
            usage();
        }

        System.err.println("Publishing to destination '"+name+"'\n");

        XAConnectionFactory factory = new com.tibco.tibjms.TibjmsXAConnectionFactory(serverUrl);

        connection = factory.createXAConnection(userName,password);

        /* create the session */
        session = connection.createXASession();

        /* create the destination */
        if (useTopic)
            destination = session.createTopic(name);
        else
            destination = session.createQueue(name);

        /* create the producer */
        msgProducer = session.createProducer(null);

        /* get the XAResource for the XASession */
        XAResource xaResource = session.getXAResource();

        /* create a transaction id */
        UID uid = new java.rmi.server.UID();
        Xid xid = new com.tibco.tibjms.TibjmsXid(0, uid.toString(),
            "branch");
        
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

        /* publish messages */
        for (i = 0; i<data.size(); i++)
        {
            /* create the text message */
            msg = session.createTextMessage();

            /* set the message text */
            msg.setText(data.elementAt(i));

            /* publish the message */
            msgProducer.send(destination, msg);

            System.err.println("Published message: "+data.elementAt(i));
        }

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
        
        /* close the connection */
        connection.close();
    }

    /*-----------------------------------------------------------------------
     * main
     *----------------------------------------------------------------------*/
    public static void main(String[] args)
    {
        tibjmsXAMsgProducer t = new tibjmsXAMsgProducer(args);
    }
}
