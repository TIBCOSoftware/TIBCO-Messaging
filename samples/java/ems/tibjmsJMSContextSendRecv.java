/* 
 * Copyright (c) 2013-$Date: 2016-12-13 17:00:37 -0600 (Tue, 13 Dec 2016) $ TIBCO Software Inc. 
 * Licensed under a BSD-style license. Refer to [LICENSE]
 * For more information, please contact:
 * TIBCO Software Inc., Palo Alto, California, USA
 * 
 * $Id: tibjmsJMSContextSendRecv.java 90180 2016-12-13 23:00:37Z olivierh $
 * 
 */

/*
 * This sample uses the simplified JMS API to send and
 * receive a message from a queue.
 *
 * Notice that queue.sample should exist in your configuration
 * or your queues configuration file should allow creation of
 * queue.sample.
 *
 * Usage:  java tibjmsJMSContextSendRecv [options]
 *
 *    where options are:
 *
 *      -server     Server URL.
 *                  If not specified this sample assumes a
 *                  serverUrl of "tcp://localhost:7222"
 *
 *      -user       User name. Default is null.
 *      -password   User password. Default is null.
 *
 */

import javax.jms.*;

public class tibjmsJMSContextSendRecv
{
    /*-----------------------------------------------------------------------
     * Parameters
     *----------------------------------------------------------------------*/

    String           serverUrl   = null;
    String           userName    = null;
    String           password    = null;

     /*-----------------------------------------------------------------------
      * Variables
      *----------------------------------------------------------------------*/
     
    public tibjmsJMSContextSendRecv(String[] args)
    {
        parseArgs(args);

        /* print parameters */
        System.err.println("\n------------------------------------------------------------------------");
        System.err.println("tibjmsJMSContextSendRecv SAMPLE");
        System.err.println("------------------------------------------------------------------------");
        System.err.println("Server....................... "+((serverUrl != null)?serverUrl:"localhost"));
        System.err.println("User......................... "+((userName != null)?userName:"(null)"));
        System.err.println("------------------------------------------------------------------------\n");

        System.err.println("Using queue: queue.sample\n");

        ConnectionFactory factory = new com.tibco.tibjms.TibjmsConnectionFactory(serverUrl);
        
        try (JMSContext context = factory.createContext(userName, password);)
        {
            Queue queue = context.createQueue("queue.sample");
            
            context.createProducer().send(queue, "test message");
            String s = context.createConsumer(queue).receiveBody(String.class);
            
            System.err.printf("Sent and Received '%s'.\n", s);
        }
    }

    /*-----------------------------------------------------------------------
     * usage
     *----------------------------------------------------------------------*/
    void usage()
    {
        System.err.println("\nUsage: tibjmsJMSContextSendRecv [options]");
        System.err.println("");
        System.err.println("   where options are:");
        System.err.println("");
        System.err.println(" -server   <server URL> - EMS server URL, default is local server");
        System.err.println(" -user     <user name>  - user name, default is null");
        System.err.println(" -password <password>   - password, default is null");
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
            {
                System.err.println("Unrecognized parameter: "+args[i]);
                usage();
            }
        }
    }

    /*-----------------------------------------------------------------------
     * main
     *----------------------------------------------------------------------*/
    public static void main(String[] args)
    {
        new tibjmsJMSContextSendRecv(args);
    }
}
