/* 
 * Copyright (c) 2004-$Date: 2017-03-27 16:01:48 -0500 (Mon, 27 Mar 2017) $ TIBCO Software Inc. 
 * Licensed under a BSD-style license. Refer to [LICENSE]
 * For more information, please contact:
 * TIBCO Software Inc., Palo Alto, California, USA
 * 
 * $Id: tibjmsLookup.java 92619 2017-03-27 21:01:48Z olivierh $
 * 
 */

import java.util.Hashtable;

import javax.jms.Connection;
import javax.jms.ExceptionListener;
import javax.jms.JMSException;
import javax.jms.Message;
import javax.jms.MessageConsumer;
import javax.jms.Session;
import javax.naming.Context;
import javax.naming.NamingException;
import javax.naming.directory.DirContext;
import javax.naming.directory.InitialDirContext;

import com.tibco.tibjms.TibjmsConnectionFactory;
import com.tibco.tibjms.TibjmsQueue;
import com.tibco.tibjms.TibjmsTopic;

/**
 * Example of how to use JNDI to store EMS administered objects in a directory
 * service as attributes. Objects stored this way may be looked up by clients
 * written in other program languages besides Java.
 * 
 * The program creates a destination and a ConnectionFactory based on the 
 * command line options. It stores the newly created objects in a directory
 * service. Objects are looked up subsequently and used to create a consumer.
 * 
 * Example usage:
 * 
 * java tibjmsLookup -provider "ldap://myserver:389/o=JNDITutorial"
 * -principal "cn=Directory Manager" -credential "passwd" -topic someTopic
 * -cfactory someFactory -server "tcp://emsserver:7222" -rounds 2
 */
public class tibjmsLookup implements ExceptionListener {

    private static final String defaultServerURL = "tcp://localhost:7222";

    String providerUrl = null;
    String principal = null;
    String credential = null;

    static String serverUrl = null;
    String queueName = null;
    String topicName = null;
    static String cfactName = null;

    static TibjmsQueue qlooked = null;
    static TibjmsTopic tlooked = null;
    static MessageConsumer consumer = null;
    static Connection connection = null;
    static Session session = null;
    static Message msg = null;
    static int rounds = 0;

    public tibjmsLookup(String[] args) {
        parseArgs(args);

        if (providerUrl == null) {
            System.err.println("Must specify a naming/directory service.");
            usage();
        }
        
        if (queueName == null && topicName == null) {
            System.err.println("Must specify a destination");
            usage();
        }

        if (serverUrl == null)
            serverUrl = defaultServerURL;

        if (cfactName == null)
            cfactName = serverUrl;

        Hashtable<String,String> env = new Hashtable<String,String>(5);
        env.put(
            Context.INITIAL_CONTEXT_FACTORY,
            "com.sun.jndi.ldap.LdapCtxFactory");
        env.put(Context.SECURITY_AUTHENTICATION, "simple");
        env.put(Context.PROVIDER_URL, providerUrl);
        env.put(Context.SECURITY_PRINCIPAL, principal);
        env.put(Context.SECURITY_CREDENTIALS, credential);

        env.put(
            Context.OBJECT_FACTORIES,
            "com.tibco.tibjms.naming.TibjmsAdministeredDirObjectFactory");
        env.put(
            Context.STATE_FACTORIES,
            "com.tibco.tibjms.naming.TibjmsStateFactory");

        try {
            DirContext ctx = new InitialDirContext(env);

            lookupDestination(ctx);

            lookupConnFact(ctx);

            ctx.close();
        } catch (NamingException e) {
            e.printStackTrace();
        }

    }

    public static void main(String args[]) {
        tibjmsLookup l = new tibjmsLookup(args);
    }

    private void lookupDestination(Context ctx) throws NamingException {
        if (queueName != null) {
            TibjmsQueue testQ = new TibjmsQueue(queueName);

            System.out.println("\nStoring Queue: " + queueName);
            ctx.rebind("cn=" + queueName, testQ);

            System.out.println("\nLooking up Queue: " + queueName);
            qlooked = (TibjmsQueue) ctx.lookup("cn=" + queueName);
            System.out.println("\nQueue looked up:" + qlooked);
        } else if (topicName != null) {
            TibjmsTopic testT = new TibjmsTopic(topicName);

            System.out.println("\nStoring Topic: " + topicName);
            ctx.rebind("cn=" + topicName, testT);

            System.out.println("\nLooking up Topic: " + topicName);
            tlooked = (TibjmsTopic) ctx.lookup("cn=" + topicName);
            System.out.println("\nTopic looked up:" + tlooked);
        }
    }

    private void lookupConnFact(Context ctx) throws NamingException {
        TibjmsConnectionFactory cfact = new TibjmsConnectionFactory(serverUrl);

        System.out.println("\nConnection Factory created: " + cfact);

        System.out.println("\nStoring Connection Factory: " + cfactName);
        ctx.rebind("cn=" + cfactName, cfact);

        System.out.println("\nLooking up Connection Factory: " + cfactName);
        TibjmsConnectionFactory cflooked =
            (TibjmsConnectionFactory) ctx.lookup("cn=" + cfactName);
        System.out.println("\nConnection Factory looked up: " + cflooked);

        try {
            System.out.println(
                "\nUsing Objects looked up to receive messages ...");

            connection = cflooked.createConnection();

            session =
                connection.createSession(
                    false,
                    javax.jms.Session.AUTO_ACKNOWLEDGE);

            connection.setExceptionListener((ExceptionListener) this);

            if (qlooked != null)
                consumer = session.createConsumer(qlooked);
            else
                consumer = session.createConsumer(tlooked);

            connection.start();

            while (true) {
                msg = consumer.receive();
                if (msg == null)
                    break;

                System.out.println("\nReceived message: " + msg);

                if (rounds > 0 && --rounds == 0)
                    break;
            }

            connection.close();

        } catch (JMSException e) {
            e.printStackTrace();
        }

    }

    void parseArgs(String[] args) {
        int i = 0;

        while (i < args.length) {
            if (args[i].compareTo("-provider") == 0) {
                if ((i + 1) >= args.length)
                    usage();
                providerUrl = args[i + 1];
                i += 2;
            } else if (args[i].compareTo("-principal") == 0) {
                if ((i + 1) >= args.length)
                    usage();
                principal = args[i + 1];
                i += 2;
            } else if (args[i].compareTo("-credential") == 0) {
                if ((i + 1) >= args.length)
                    usage();
                credential = args[i + 1];
                i += 2;
            } else if (args[i].compareTo("-cfactory") == 0) {
                if ((i + 1) >= args.length)
                    usage();
                cfactName = args[i + 1];
                i += 2;
            } else if (args[i].compareTo("-topic") == 0) {
                if ((i + 1) >= args.length)
                    usage();
                topicName = args[i + 1];
                i += 2;
            } else if (args[i].compareTo("-queue") == 0) {
                if ((i + 1) >= args.length)
                    usage();
                queueName = args[i + 1];
                i += 2;
            } else if (args[i].compareTo("-server") == 0) {
                if ((i + 1) >= args.length)
                    usage();
                serverUrl = args[i + 1];
                i += 2;
            } else if (args[i].compareTo("-rounds") == 0) {
                if ((i + 1) >= args.length)
                    usage();
                try {
                    rounds = new Integer(args[i + 1]).intValue();
                } catch (NumberFormatException e) {
                    e.printStackTrace();
                }
                i += 2;
            } else {
                System.err.println("Unrecognized parameter: " + args[i]);
                usage();
            }
        }
    }

    void usage() {
        System.err.println("\nUsage: java tibjmsLookup [options]");
        System.err.println("");
        System.err.println("  where options are:");
        System.err.println("");
        System.err.println(" -provider  <provider URL> - JNDI provider URL");
        System.err.println(
            " -principal <user name>    - foreign provider user name");
        System.err.println(
            " -credential <password>    - foreign provider password");
        System.err.println(
            " -cfactory  <factory>      - connection factory to look up and store");
        System.err.println(
            " -topic     <topic name>   - topic to look up and store in foreign service");
        System.err.println(
            " -queue     <queue name>   - queue to look up and store in foreign service");
        System.err.println(
            " -server    <server URL>   - JMS server URL for new topic or queue connection");
        System.err.println(
            "                             factory, default is local server");
        System.err.println(
            " -rounds    <number>       - number of messages to receive before exit");
        System.exit(0);
    }

    public void onException(JMSException e) {
        /* print the connection exception status */
        System.err.println("CONNECTION EXCEPTION: " + e.getMessage());
    }
}
