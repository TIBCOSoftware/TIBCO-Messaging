/* 
 * Copyright (c) 2001-$Date: 2016-12-13 17:00:37 -0600 (Tue, 13 Dec 2016) $ TIBCO Software Inc. 
 * Licensed under a BSD-style license. Refer to [LICENSE]
 * For more information, please contact:
 * TIBCO Software Inc., Palo Alto, California, USA
 * 
 * $Id: tibjmsToRv.java 90180 2016-12-13 23:00:37Z olivierh $
 * 
 */

 /* Important: to compile and run this sample you need to have
  * the TIBCO Rendezvous software installed and have the appropriate
  * configuration. Read the description below for more details.
  * Note that configuration problems may lead to this sample not
  * working properly.
  */

 /*
  * This sample demonstrates message exchange between a simple
  * JMS client and a TIBCO Rendezvous client. Both clients are
  * implemented in this sample program.
  *
  * In order to compile and run this sample you must have
  * both TIBCO Enterprise Message Service and TIBCO Rendezvous software
  * installed on your computer.
  *
  * This sample uses two topics for message exchange:
  *
  * topic.sample.exported
  *
  *   - topic used by JMS client to send messages to a
  *     TIBCO Rendezvous listener. This topic or any of its ancestors
  *     should be present in the TIBCO EMS server
  *     configuration file. The following is an example of the
  *     topic description line in the topics configuration file:
  *
  *     topic.sample.exported   tibrv_export
  *
  * topic.sample.imported
  *
  *   - topic used by JMS client to receive messages from
  *     a TIBCO Rendezvous publisher. This topic or any its ancestors
  *     should be present in the TIBCO EMS server
  *     configuration file. The following is an example of the
  *     topic description line in the topics configuration file:
  *
  *     topic.sample.imported   tibrv_import
  *
  * Both topics can be changed using command-line parameters.
  *
  * Usage: java tibjmsToRv [options]
  *
  *     where options are:
  *
  *     -server       TIBCO EMS server;
  *                   By default this sample connects to the EMS
  *                   server running on the local computer.
  *     -export       name of export topic (default value is
  *                   'topic.sample.exported')
  *     -import       name of import topic (default value is
  *                   'topic.sample.imported')
  *     -service      TIBCO Rendezvous transport service parameter
  *     -network      TIBCO Rendezvous transport network parameter
  *     -daemon       TIBCO Rendezvous transport daemon parameter
  *
  * It is important to make sure the 'service', 'network' and 'daemon'
  * parameters match those specified in the main configuration file for
  * the TIBCO EMS server: tibrv_service, tibrv_network and
  * tibrv_daemon. Also check if TIBCO Rendezvous transports are enabled in the
  * server configuration file. The sample configuration file shipped with
  * TIBCO EMS disables TIBCO Rendezvous transport by default.
  *
  * Note that this sample uses direct access to TIBCO EMS
  * ConnectionFactory class. This is done for simplicity,
  * accessing factories via the JNDI interface is the preferred
  * approach and is demonstrated with other samples.
  *
  */

import javax.jms.*;

/*
 * Import TIBCO Rendezvous classes
 */
import com.tibco.tibrv.*;

/* use JNDI in real environment, here for simplicity
 * will use direct access to TIBCO EMS factory class.
 */
import com.tibco.tibjms.*;


/*
 * tibjmsToRv
 */
public class tibjmsToRv
{
    String                      serverUrl           = null;

    String                      jms_to_rv_topic_name= "topic.sample.exported";
    String                      rv_to_jms_topic_name= "topic.sample.imported";

    javax.jms.Topic             jms_to_rv_topic     = null;
    javax.jms.Topic             rv_to_jms_topic     = null;

    javax.jms.Session           subscribe_session   = null;
    javax.jms.Session           publish_session     = null;

    javax.jms.MessageConsumer   consumer            = null;
    javax.jms.MessageProducer   producer            = null;

    //=====================================================================
    // This implements TIBCO Rendezvous portion of this sample.
    //
    // It only uses TIBCO Rendezvous API and not JMS API.
    //=====================================================================

    String                      rv_service          = null;
    String                      rv_network          = null;
    String                      rv_daemon           = null;

    TibrvTransport              rv_transport        = null;

    /*
     * Initializes TIBCO Rendezvous environment and starts a listener
     * on the subject exported from EMS
     */
    public void initTIBRendezvous()
    {
        try
        {
            /*
            * Open TIBCO Rendezvous environment in NATIVE implementation
            */
            Tibrv.open(Tibrv.IMPL_NATIVE);

            /*
            * Create TIBCO Rendezvous transport
            */
            rv_transport = new TibrvRvdTransport(rv_service,rv_network,rv_daemon);

            /*
            * Create a queue and start dispatching it
            */
            TibrvQueue rv_queue = new TibrvQueue();
            new TibrvDispatcher(rv_queue);

            /*
            * Start a listener on the subject exported from EMS
            */
            new TibrvListener(rv_queue,new RvCallback(),rv_transport,
                            jms_to_rv_topic_name,null);
        }
        catch (TibrvException e)
        {
            System.err.println("**TibrvException has occurred while initializing"+
                               "TIBCO Rendezvous environment");
            e.printStackTrace();
            System.exit(0);
        }
    }

    /*
     * A callback called by TIBCO Rendezvous when a message has
     * been received
     */
    class RvCallback implements TibrvMsgCallback
    {
        public void onMsg(TibrvListener listener, TibrvMsg msg)
        {
            try
            {
                /*
                 * print a message we have received
                 */
                System.err.println("-----------------------------------------------------------------------");
                System.err.println("TIBCO Rendezvous client has received a message from JMS client:");
                System.err.println("Send subject  = "+msg.getSendSubject());
                System.err.println("Reply subject = "+msg.getReplySubject());
                System.err.println("Message:");
                System.err.println(msg);

                /*
                 * now let's send a different message back to JMS client
                 */

                /* build a message */
                TibrvMsg response = new TibrvMsg();
                response.add("rv_int",5);
                response.add("rv_string","From TIBCO Rendezvous to EMS");

                /* set send subject */
                response.setSendSubject(rv_to_jms_topic_name);

                /* send a message */
                rv_transport.send(response);
            }
            catch (TibrvException e)
            {
                e.printStackTrace();
                System.exit(0);
            }
        }
    }

    /*
     * constructor and main working code
     */
    public tibjmsToRv(String[] args)
    {
        parseArgs(args);

        System.err.println("Sample of JMS client exchanging messages with TIBCO Rendezvous");

        try
        {
            /* initialize TIBCO Rendezvous */
            initTIBRendezvous();

            /* use direct access here, use JNDI in other situations */
            ConnectionFactory factory =
                new com.tibco.tibjms.TibjmsConnectionFactory(serverUrl);

            Connection connection = factory.createConnection();

            publish_session       = connection.createSession();
            subscribe_session     = connection.createSession();

            /* could use JNDI here as well, use direct creation for simplicity */

            /* create topic we will use to send messages to TIBCO Rendezvous */
            jms_to_rv_topic = publish_session.createTopic(jms_to_rv_topic_name);

            /* create topic we will use to receive messages from TIBCO Rendezvous */
            rv_to_jms_topic = subscribe_session.createTopic(rv_to_jms_topic_name);

            /* create a producer */
            producer = publish_session.createProducer(jms_to_rv_topic);

            /* create a consumer */
            consumer = subscribe_session.createConsumer(rv_to_jms_topic);

            /* start the connection */
            connection.start();

            /*
             * Setup is complete, send messages
             */

            /* build message */
            MapMessage sendMessage = publish_session.createMapMessage();
            sendMessage.setInt("jms_int",7);
            sendMessage.setString("jms_string","From EMS to TIBCO Rendezvous");

            /* send message */
            producer.send(sendMessage);

            /* now wait to receive a message from TIBCO Rendezvous */
            Message reply = consumer.receive(5000);
            if (reply == null)
            {
                System.err.println("**Error: failed to receive a message from TIBCO Rendezvous.");
                System.err.println("         Receive timed out.");
                System.err.println("         It is possible that the TIBCO Rendezvous transports not enabled");
                System.err.println("         in the server configuration file. Another reason is if the topics");
                System.err.println("         used by this program do not have appropriate export and import");
                System.err.println("         properties assigned to them.");
                System.exit(0);
            }
            else
            {
                System.err.println("-----------------------------------------------------------------------");
                System.err.println("JMS client has received a message from TIBCO Rendezvous client:");
                System.err.println("Destination topic = "+
                                    ((javax.jms.Topic)reply.getJMSDestination()).getTopicName());
                if (reply.getJMSReplyTo() != null)
                System.err.println("ReplyTo topic     = "+
                                    ((javax.jms.Topic)reply.getJMSReplyTo()).getTopicName());
                System.err.println("Message:");
                System.err.println(reply);
            }

            /*
             * close TIBCO Rendezvous
             */
            try
            {
                Tibrv.close();
            }
            catch (TibrvException e)
            {
                e.printStackTrace();
                System.exit(0);
            }

            /*
             * close Connection
             */
            connection.close();
            System.exit(0);

        }
        catch (JMSException e)
        {
            System.err.println("JMSException: message="+e.getMessage()+", provider="+e.getErrorCode());
            e.printStackTrace();
            System.exit(0);
        }
    }


    public static void main(String args[])
    {
        tibjmsToRv t = new tibjmsToRv(args);
    }

    void usage()
    {
        System.err.println("\nUsage: java tibjmsToRv [options]");
        System.err.println("");
        System.err.println("   where options are:");
        System.err.println("");
        System.err.println("   -server  <server URL>         - EMS server URL");
        System.err.println("   -export  <export topic name>  - export topic name");
        System.err.println("   -import  <import topic name>  - import topic name");
        System.err.println("   -service <service-spec>       - TIBCO Rendezvous service");
        System.err.println("   -network <network-spec>       - TIBCO Rendezvous network");
        System.err.println("   -daemon  <daemon-spec>        - TIBCO Rendezvous daemon");
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
            if (args[i].compareTo("-export")==0)
            {
                if ((i+1) >= args.length) usage();
                jms_to_rv_topic_name = args[i+1];
                i += 2;
            }
            else
            if (args[i].compareTo("-import")==0)
            {
                if ((i+1) >= args.length) usage();
                rv_to_jms_topic_name = args[i+1];
                i += 2;
            }
            else
            if (args[i].compareTo("-service")==0)
            {
                if ((i+1) >= args.length) usage();
                rv_service = args[i+1];
                i += 2;
            }
            else
            if (args[i].compareTo("-network")==0)
            {
                if ((i+1) >= args.length) usage();
                rv_network = args[i+1];
                i += 2;
            }
            else
            if (args[i].compareTo("-daemon")==0)
            {
                if ((i+1) >= args.length) usage();
                rv_daemon = args[i+1];
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
