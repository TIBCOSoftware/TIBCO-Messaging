/* 
 * Copyright (c) 2001-$Date: 2016-12-13 17:00:37 -0600 (Tue, 13 Dec 2016) $ TIBCO Software Inc. 
 * Licensed under a BSD-style license. Refer to [LICENSE]
 * For more information, please contact:
 * TIBCO Software Inc., Palo Alto, California, USA
 * 
 * $Id: tibjmsSelector.java 90180 2016-12-13 23:00:37Z olivierh $
 * 
 */

/*
 * This sample demonstrates the use of Selectors.
 *
 * This program creates a queue sender which sends a few messages
 * into a queue, each message has an integer property set to the
 * sequential number of the message. Three asynchronous queue
 * receivers are created each with selectors such that each
 * message satisfies only a single receiver's selector.
 *
 * This sample supports the '-noselector' option which allows to
 * disable use of sleectors and see what difference does it make.
 *
 * Usage:  java tibjmsSelector [options]
 *
 *    where options are:
 *
 *      -server      Server URL.
 *                   If not specified this sample assumes a
 *                   serverUrl of "tcp://localhost:7222"
 *
 *      -user        User name. Default is null.
 *      -password    User password. Default is null.
 *      -queue       Queue name. Default is "queue.sample.selector"
 *
 *      -noselector  Use it to see the difference when run with selectors
 *                   and without
 *
 */

import javax.jms.*;

public class tibjmsSelector
{
    String      serverUrl       = null;
    String      userName        = null;
    String      password        = null;

    String      queueName       = "queue.sample.selector";
    boolean     noselector      = false;


    class MyMessageListener implements MessageListener
    {
        int receiverNumber;

        MyMessageListener(int receiverNumber)
        {
            this.receiverNumber = receiverNumber;
        }

        public void onMessage(Message message)
        {
            try
            {
                System.err.println("Receiver "+receiverNumber+" received message "+
                            message.getIntProperty("count"));
            }
            catch (JMSException e)
            {
                e.printStackTrace();
                System.exit(0);
            }
        }
    }

    public tibjmsSelector(String[] args)
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

        System.err.println("Queue with Selector sample.");
        System.err.println("Using queue:  "+queueName);

        if (!noselector)
        {
            System.err.println("\n*** Also try to run this sample with the -noselector");
            System.err.println  ("*** option to see the difference it makes.");
        }

        try
        {
            ConnectionFactory factory = new com.tibco.tibjms.TibjmsConnectionFactory(serverUrl);

            Connection connection = factory.createConnection(userName,password);

            Session receive_session = connection.createSession();
            Session send_session = connection.createSession();

            javax.jms.Queue queue = send_session.createQueue(queueName);

            /*
             * Start the connection so we can drain the queue
             * and then proceed.
             */
            connection.start();

            /*
             * drain the queue
             */
            MessageConsumer receiver = receive_session.createConsumer(queue);

            int drain_count = 0;
            System.err.println("\nDraining the queue "+queueName);

            /* read queue until empty */
            while (receiver.receive(1000) != null)
                drain_count++;

            System.err.println("Drained "+drain_count+" messages from the queue");

            /*
             * close receiver to prevent any
             * queue messages to be delivered
             */
            receiver.close();

            /*
             * create receivers with selectors
             */
            System.err.println("");
            if (!noselector)
                System.err.println("Creating receivers with selectors:\n");
            else
                System.err.println("Creating receivers without selectors:\n");
            Thread.sleep(500);

            int receiver_count = 3;
            for (int i=0; i<receiver_count; i++)
            {
                String selector = null;

                if (!noselector)
                    selector = "count >= "+(i*4)+" AND count < "+(i*4+4);

                receiver = receive_session.createConsumer(queue,selector);

                if (!noselector)
                    System.err.println("Created receiver "+i+" with selector: \""+selector+"\"");
                else
                    System.err.println("Created receiver "+i);

                receiver.setMessageListener(new MyMessageListener(i));
                Thread.sleep(500);
            }

            /*
             * create sender
             */
            MessageProducer sender = send_session.createProducer(queue);

            javax.jms.Message message = null;

            int message_number = 0;

            /*
             * send 12 messages into queue
             */
            System.err.println("");
            System.err.println("Sending 12 messages into the queue:\n");

            Thread.sleep(200);

            for (int i=0; i<12; i++)
            {
                message = send_session.createMessage();
                message.setIntProperty("count",message_number);
                sender.send(message);
                Thread.sleep(500);
                message_number++;
            }

            /*
             * wait for some time while all messages received
             */
            Thread.sleep(1000);

            connection.close();
        }
        catch (JMSException e)
        {
            e.printStackTrace();
            System.exit(0);
        }
        catch (InterruptedException e)
        {
            e.printStackTrace();
            System.exit(0);
        }

    }

    public static void main(String args[])
    {
        tibjmsSelector t = new tibjmsSelector(args);
    }

    void usage()
    {
        System.err.println("\nUsage: java tibjmsSelector [options]");
        System.err.println("");
        System.err.println("   where options are:");
        System.err.println("");
        System.err.println(" -server     <server URL> - EMS server URL, default is local server");
        System.err.println(" -user       <user name>  - user name, default is null");
        System.err.println(" -password   <password>   - password, default is null");
        System.err.println(" -queue      <queue name> - queue name, default is \"queue.sample.selector\"");
        System.err.println(" -noselector              - do not use selectors");
        System.err.println(" -help-ssl                - help on ssl parameters\n");
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
            if (args[i].compareTo("-queue")==0)
            {
                if ((i+1) >= args.length) usage();
                queueName = args[i+1];
                i += 2;
            }
            else
            if (args[i].compareTo("-noselector")==0)
            {
                noselector = true;
                i += 1;
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
}


