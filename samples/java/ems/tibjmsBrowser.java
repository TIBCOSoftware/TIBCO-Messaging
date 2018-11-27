/* 
 * Copyright (c) 2002-$Date: 2016-12-13 17:00:37 -0600 (Tue, 13 Dec 2016) $ TIBCO Software Inc. 
 * Licensed under a BSD-style license. Refer to [LICENSE]
 * For more information, please contact:
 * TIBCO Software Inc., Palo Alto, California, USA
 * 
 * $Id: tibjmsBrowser.java 90180 2016-12-13 23:00:37Z olivierh $
 * 
 */

/*
 * This sample demonstrates the use of the JMS QueueBrowser.
 *
 * Notice that TIBCO Enterprise Message Service implements dynamic
 * queue browsers. This means that the QueueBrowser can
 * dynamically receive new messages added to the queue.
 * If hasMoreElements() method of the Enumeration returned by
 * the QueueBrowser class returns false, the JMS application
 * can wait and try to call it later. If the queue being browsed
 * has received new messages, the hasMoreElements() method will
 * return true and the application can browse new messages.
 * If hasMoreElements() returns false, the application can choose
 * to quit browsing or can wait for more messages to be delivered
 * into the queue.
 *
 * After all queue messages have been delivered to the queue
 * browser, TIBCO Enterprise Message Service waits for some time and then
 * tries to query if any new messages. This happens behind the scene,
 * user application can try to call hasMoreElements() at any time,
 * the internal engine will only actually query the queue every fixed
 * interval. TIBCO Enterprise Message Service queries the queue
 * not more often than every 5 seconds but the length of that
 * interval is a subject to change without notice.
 *
 * Usage:  java tibjmsBrowser [options]
 *
 *    where options are:
 *
 *      -server     Server URL.
 *                  If not specified this sample assumes a
 *                  serverUrl of "tcp://localhost:7222"
 *
 *      -queue      Queue name. Default is "queue.sample.browser"
 *      -user       User name. Default is null.
 *      -password   User password. Default is null.
 *
 *
 */

import javax.jms.*;
import java.util.Enumeration;

public class tibjmsBrowser
{
    String      serverUrl       = null;
    String      userName        = null;
    String      password        = null;
    String      queueName       = "queue.sample.browser";

    public tibjmsBrowser(String[] args)
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

        System.err.println("Browser sample.");
        System.err.println("Using server:   "+serverUrl);
        System.err.println("Browsing queue: "+queueName);

        try
        {
            ConnectionFactory factory = new com.tibco.tibjms.TibjmsConnectionFactory(serverUrl);

            Connection connection = factory.createConnection(userName,password);

            Session session = connection.createSession();

            javax.jms.Queue queue = session.createQueue(queueName);

            MessageProducer producer = session.createProducer(queue);

            javax.jms.Message message = null;

            connection.start();

            /*
             * drain the queue
             */
            MessageConsumer consumer = session.createConsumer(queue);

            int drain_count = 0;

            System.err.println("Draining the queue "+queueName);

            /* read queue until empty */
            while (consumer.receive(1000) != null)
            {
                drain_count++;
            }
            System.err.println("Drained "+drain_count+" messages from the queue");

            /*
             * close consumer to prevent any
             * queue messages to be delivered
             */
            consumer.close();

            int message_number = 0;

            /*
             * send 5 messages into queue
             */
            System.err.println("Sending 5 messages into queue.");
            for (int i=0; i<5; i++)
            {
                message_number++;
                message = session.createMessage();
                message.setIntProperty("msg_num",message_number);
                producer.send(message);
            }

            /*
             * create browser and browse what is there in the queue
             */
            System.err.println("--- Browsing the queue.");

            javax.jms.QueueBrowser browser = session.createBrowser(queue);

            Enumeration msgs = browser.getEnumeration();

            int browseCount=0;

            while (msgs.hasMoreElements())
            {
                message = (javax.jms.Message)msgs.nextElement();
                System.err.println("Browsed message: number="+message.getIntProperty("msg_num"));
                browseCount++;
            }

            System.err.println("--- No more messages in the queue.");

            /*
             * send 5 more messages into queue
             */
            System.err.println("Sending 5 more messages into queue.");
            for (int i=0; i<5; i++)
            {
                message_number++;
                message = session.createMessage();
                message.setIntProperty("msg_num",message_number);
                producer.send(message);
            }

            /*
             * try to browse again, if no success for some time
             * then quit
             */

            /* notice that we will *not* receive new messages
             * instantly. It happens because QueueBrowser limits
             * the frequency of query requests sent into the queue
             * after the queue was empty. Internal engine only queries
             * the queue every so many seconds, so we'll likely have
             * to wait here for some time.
             */

            int attemptCount = 0;

            while (!msgs.hasMoreElements())
            {
                attemptCount++;
                System.err.println("Waiting for messages to arrive, count="+attemptCount);
                Thread.sleep(1000);
                if (attemptCount > 30)
                {
                    System.err.println("Still no messages in the queue after "+attemptCount+" seconds");
                    System.exit(0);
                }
            }

            /*
             * got more messages, continue browsing
             */
            System.err.println("Found more messages. Continue browsing.");
            while (msgs.hasMoreElements())
            {
                message = (javax.jms.Message)msgs.nextElement();
                System.err.println("Browsed message: number="+message.getIntProperty("msg_num"));
            }

            /*
             * close all and quit
             */
            browser.close();

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
        tibjmsBrowser t = new tibjmsBrowser(args);
    }

    void usage()
    {
        System.err.println("\nUsage: java tibjmsBrowser [options]");
        System.err.println("");
        System.err.println("  where options are:");
        System.err.println("");
        System.err.println("  -server   <server-url>  - EMS server URL, default is local server");
        System.err.println("  -user     <user-name>   - user name, default is null");
        System.err.println("  -password <password>    - password, default is null");
        System.err.println("  -queue    <queue-name>  - queue name, default is \"queue.sample.browser\"");
        System.err.println("  -help-ssl               - help on ssl parameters\n");
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
            if (args[i].compareTo("-queue")==0)
            {
                if ((i+1) >= args.length) usage();
                queueName = args[i+1];
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


