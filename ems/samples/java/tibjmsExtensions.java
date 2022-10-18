/* 
 * Copyright (c) 2001-$Date: 2016-12-13 17:00:37 -0600 (Tue, 13 Dec 2016) $ TIBCO Software Inc. 
 * Licensed under a BSD-style license. Refer to [LICENSE]
 * For more information, please contact:
 * TIBCO Software Inc., Palo Alto, California, USA
 * 
 * $Id: tibjmsExtensions.java 90180 2016-12-13 23:00:37Z olivierh $
 * 
 */

/*
 * Sample of how TIBCO Enterprise Message Service extensions can be used.
 *
 * This sample publishes a specified number of messages on a topic
 * and receives all of those messages in a different session. After
 * all messages have been received by the subsciber, it publishes
 * back a confirmation message to ensure all messages have been
 * delivered.
 *
 * This test is performed 4 times using four combinations of delivery mode
 * and acknowledge mode:
 *
 *   Publisher mode        Subscriber mode
 *  ----------------------------------------------
 *   NON_PERSISTENT        CLIENT_ACKNOWLEDGE
 *   NON_PERSISTENT        AUTO_ACKNOWLEDGE
 *   NON_PERSISTENT        DUPS_OK_ACKNOWLEDGE
 *  *RELIABLE_DELIVERY    *NO_ACKNOWLEDGE
 *
 * Notice the last combination utilizes TIBCO Enterprise Message Service
 * proprietary extensions of delivery mode and acknowledge mode.
 *
 * Depending on your hardware you may need to change the number of
 * messages being published in a single run. The run should persist for
 * at least 3 seconds to give reliable results.
 * For best results make sure it takes no less than 10 seconds to complete.
 * The default value in this sample is 10,000 messages. This may not be large
 * enough if you run this sample on fast hardware.
 *
 * Under normal circumstances you should see each run to perform
 * faster than the previous and you can compare the results. The run
 * which uses proprietary values of RELIABLE_DELIVERY and NO_ACKNOWLEDGE
 * should perform much faster than other runs, although performance
 * will depend on a number of external parameters.
 *
 * It is *strongly* recommended to run this sample on a computer not
 * performing any other calculational or networking tasks.
 *
 * Notice that increased performance is achieved by the client and the JMS
 * server omitting certain confirmation messages which in turn increases
 * the chances of messages being lost during a provider failure. Please
 * refer to the TIBCO Enterprise Message Service documentation which
 * explains these issues in detail. This sample should not be used to
 * calculate the performance of TIBCO Enterprise Message Service, it exists
 * solely to compare relative performance of topic delivery utilizing
 * different delivery and acknowledgment modes.
 *
 * Also notice this sample only demonstrates relative performance in a
 * particular situation when the publisher and the subscriber run in the
 * same application on a dedicated computer.
 * The difference in performance may change greatly depending on particular
 * usage. This includes a possibility that the RELIABLE delivery and
 * NO_ACKNOWLEDGE acknowledge modes may deliver even greater performance
 * advantage compared to other modes if the publisher and subscribers
 * run on different computers.
 *
 * Usage:  java tibjmsExtensions  [options]
 *
 *    where options are:
 *
 *      -server       Server URL.
 *                    If not specified this sample assumes a
 *                    serverUrl of "tcp://localhost:7222"
 *
 *      -user        User name. Default is null.
 *      -password    User password. Default is null.
 *      -count       Number of messages to use in each run. Default is 20000.
 *
 */

import javax.jms.*;
import javax.naming.*;

public class tibjmsExtensions implements Runnable
{
    String              serverUrl   = null;
    String              userName    = null;
    String              password    = null;

    int                 count       = 20000;

    Connection          connection  = null;

    /*
     * these used to publish and receive all messages
     */
    Topic               topic       = null;
    Session             publishSession;
    Session             subscribeSession;
    MessageProducer     publisher;
    MessageConsumer     subscriber;

    /*
     * these used to send final confirmation when the subscriber
     * receives all messages it supposed to receive
     */
    Topic               checkTopic  = null;
    MessageProducer     checkPublisher;
    MessageConsumer     checkSubscriber;


    /*
     * simple helper to print the performance nummbers
     */
    public String getPerformance(long startTime, long endTime, int count)
    {
        if (startTime >= endTime)
            return String.valueOf(Integer.MAX_VALUE);

        int seconds = (int)((endTime-startTime)/1000);
        int millis  = (int)((endTime-startTime)%1000);
        double d = ((double)count)/(((double)(endTime-startTime))/1000.0);
        int iperf = (int)d;
        return (""+count+" times took "+seconds+"."+millis+" seconds, performance is "+iperf+" messages/second");
    }

    /* returns printable delivery mode name */
    public static String deliveryModeName(int delMode)
    {
        switch(delMode)
        {
            case javax.jms.DeliveryMode.PERSISTENT:         return "PERSISTENT";
            case javax.jms.DeliveryMode.NON_PERSISTENT:     return "NON_PERSISTENT";
            case com.tibco.tibjms.Tibjms.RELIABLE_DELIVERY: return "RELIABLE";
            default:                                        return "???";
        }
    }

    /* returns printable acknowledge mode name */
    public static String acknowledgeModeName(int ackMode)
    {
        switch(ackMode)
        {
            case javax.jms.Session.CLIENT_ACKNOWLEDGE:   return "CLIENT_ACKNOWLEDGE";
            case javax.jms.Session.AUTO_ACKNOWLEDGE:     return "AUTO_ACKNOWLEDGE";
            case javax.jms.Session.DUPS_OK_ACKNOWLEDGE:  return "DUPS_OK_ACKNOWLEDGE";
            case com.tibco.tibjms.Tibjms.NO_ACKNOWLEDGE: return "NO_ACKNOWLEDGE";
            default:                                     return "???";
        }
    }

    /*
     * a thread which keeps printing dots on the screen
     */
    public void run()
    {
        try
        {
            while (true)
            {
                Thread.sleep(1000);
                System.err.print(".");
            }
        }
        catch (InterruptedException e)
        {
            // return
        }
    }

    /*
     * A listener for the asynchronous consumer receiving messages
     * published on a topic. It does pretty much nothing but receives
     * and counts messages delivered to it by the subscriber session.
     * When all messages have been received it publishes a confirmation
     * message to the publishing session.
     */
    class MyMessageListener implements MessageListener
    {
        int     receivedCount   = 0;
        int     acknowledgeMode = 0;

        boolean warmupMode      = false;

        MyMessageListener(int acknowledgeMode, boolean warmupMode)
        {
            this.acknowledgeMode = acknowledgeMode;
            this.warmupMode      = warmupMode;

            receivedCount = 0;
        }

        public void onMessage(Message message)
        {
            receivedCount++;

            try
            {
                /* acknowledge if CLIENT_ACKNOWLEDGE */
                if (acknowledgeMode == javax.jms.Session.CLIENT_ACKNOWLEDGE)
                    message.acknowledge();

                /* if it was the last message, publish a confirmation
                 * that we have received all messages
                 */
                if (receivedCount == count)
                    checkPublisher.send(message);
            }
            catch (JMSException e)
            {
                e.printStackTrace();
                System.exit(0);
            }
        }
    }

    /*
     * Main method which actually publishes all messages, checks
     * when they all have been received by the subscriber, then
     * calculates and prints the performance numbers. The delivery
     * mode and acknowledge mode are parameters to this method.
     */
    public void doRun(int deliveryMode, int acknowledgeMode, boolean warmupMode) throws JMSException
    {
        /* we need two sessions */
        publishSession   = connection.createSession(acknowledgeMode);
        subscribeSession = connection.createSession(acknowledgeMode);

        /* A thread displaying dots on the screen */
        Thread tracker = null;

        /* if not yet created, create main topic and the check topic used
         * to send confirmations back from the subscriber to the publisher
         */
        if (topic == null)
        {
            topic      = publishSession.createTopic("test.extensions.topic");
            checkTopic = publishSession.createTopic("test.extensions.check.topic");
        }

        /* create the publisher and set its delivery mode */
        publisher  = publishSession.createProducer(topic);
        publisher.setDeliveryMode(deliveryMode);

        /* create the subscriber and set its listener */
        subscriber = subscribeSession.createConsumer(topic);
        subscriber.setMessageListener(new MyMessageListener(acknowledgeMode,warmupMode));

        /* create a publisher and a subscriber to deal with the
         * final confirmation message
         */
        checkPublisher  = subscribeSession.createProducer(checkTopic);
        checkSubscriber = publishSession.createConsumer(checkTopic);

        /* will publish a virtually empty message */
        Message message = publishSession.createMessage();

        if (!warmupMode)
        {
            System.err.println("");
            System.err.println("Sending and Receiving "+count+" messages");
            System.err.println("        delivery mode:    "+deliveryModeName(deliveryMode));
            System.err.println("        acknowledge mode: "+acknowledgeModeName(acknowledgeMode));
        }

        /* start printing dots while we run the main loop */
        tracker = new Thread(this);
        tracker.start();

        long startTime = System.currentTimeMillis();

        /*
         * now publish all messages
         */
        for (int i=0; i<count; i++)
            publisher.send(message);

        /*
         * Wait for the subscriber to receive all messages
         * and publish the confirmation message which we'll receive here.
         * If anything goes wrong we will get stuck here, but let's
         * hope everything will be fine
         */
        checkSubscriber.receive();

        /*
         * Print performance, close all and we are done
         */
        long endTime = System.currentTimeMillis();

        tracker.interrupt();

        if (!warmupMode)
        {
            System.err.println(" completed");

            if ((endTime-startTime) < 3000)
            {
                System.err.println("**Warning**: elapsed time is too small to calculate reliable performance");
                System.err.println("             numbers. You need to re-run this test with greater number");
                System.err.println("             of messages. Use '-count' command-line parameter to specify");
                System.err.println("             greater message count.");
            }

            System.err.println(getPerformance(startTime,endTime,count));
        }

        publishSession.close();
        subscribeSession.close();
    }


    /*
     *
     */
    public tibjmsExtensions(String[] args)
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

        System.err.println("TIBCO Enterprise Message Service Extensions sample.");
        System.err.println("Using server:       "+serverUrl);

        try
        {
            ConnectionFactory factory = new com.tibco.tibjms.TibjmsConnectionFactory(serverUrl);

            connection = factory.createConnection(userName,password);

            /* do not forget to start the connection... */
            connection.start();

            int n = count;

            /*
             * Warm up the server and HotSpot...
             */
            System.err.print("Warming up before testing the performance. Please wait...");
            count = 3000;
            doRun(DeliveryMode.NON_PERSISTENT,Session.AUTO_ACKNOWLEDGE,true);
            System.err.println("");

            /* let EMS server to calm down */
            try
            {
                Thread.sleep(1000);
            }
            catch (InterruptedException e){}

            /*
             * recover actual count
             */
            count = n;

            /*
             * Use PERSISTENT / CLIENT_ACKNOWLEDGE modes
             */
            doRun(DeliveryMode.PERSISTENT,Session.CLIENT_ACKNOWLEDGE,false);

            /*
             * Use PERSISTENT / AUTO_ACKNOWLEDGE modes
             */
            doRun(DeliveryMode.PERSISTENT,Session.AUTO_ACKNOWLEDGE,false);

            /*
             * Use PERSISTENT / DUPS_OK_ACKNOWLEDGE modes
             */
            doRun(DeliveryMode.PERSISTENT,Session.DUPS_OK_ACKNOWLEDGE,false);

            /*
             * Use NON_PERSISTENT / CLIENT_ACKNOWLEDGE modes
             */
            doRun(DeliveryMode.NON_PERSISTENT,Session.CLIENT_ACKNOWLEDGE,false);

            /*
             * Use NON_PERSISTENT / AUTO_ACKNOWLEDGE modes
             */
            doRun(DeliveryMode.NON_PERSISTENT,Session.AUTO_ACKNOWLEDGE,false);

            /*
             * Use NON_PERSISTENT / DUPS_OK_ACKNOWLEDGE modes
             */
            doRun(DeliveryMode.NON_PERSISTENT,Session.DUPS_OK_ACKNOWLEDGE,false);

            /*
             * Use proprietary RELIABLE_DELIVERY / NO_ACKNOWLEDGE modes
             */
            doRun(com.tibco.tibjms.Tibjms.RELIABLE_DELIVERY,
                  com.tibco.tibjms.Tibjms.NO_ACKNOWLEDGE,false);

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
        tibjmsExtensions t = new tibjmsExtensions(args);
    }

    void usage()
    {
        System.err.println("\nUsage: java tibjmsExtensions [options]");
        System.err.println("");
        System.err.println("  where options are:");
        System.err.println("");
        System.err.println(" -server    <server URL> - EMS server URL, default is local server");
        System.err.println(" -user      <user name>  - user name, default is null");
        System.err.println(" -password  <password>   - password, default is null");
        System.err.println(" -count     <nnnn>       - number of messages, default is 20000");
        System.err.println(" -help-ssl               - help on ssl parameters\n");
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
            if (args[i].compareTo("-count")==0)
            {
                if ((i+1) >= args.length) usage();
                try
                {
                    count = Integer.parseInt(args[i+1]);
                }
                catch (NumberFormatException e)
                {
                    System.err.println("Error: invalid value of -count parameter");
                    usage();
                }
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


