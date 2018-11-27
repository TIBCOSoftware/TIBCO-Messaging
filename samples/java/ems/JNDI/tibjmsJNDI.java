/* 
 * Copyright (c) 2002-$Date: 2016-12-13 17:00:37 -0600 (Tue, 13 Dec 2016) $ TIBCO Software Inc. 
 * Licensed under a BSD-style license. Refer to [LICENSE]
 * For more information, please contact:
 * TIBCO Software Inc., Palo Alto, California, USA
 * 
 * $Id: tibjmsJNDI.java 90180 2016-12-13 23:00:37Z olivierh $
 * 
 */

/*
 * Example of how to use JNDI to lookup administered objects
 * from a TIBCO Enterprise Message Service client.
 *
 * There are three types of administered objects:
 *   - ConnectionFactory
 *   - Topic
 *   - Queue
 *
 * TIBCO Enterprise Message Service provides the ability to lookup
 * those administered objects using the TIBCO Enterprise Message Service
 * server as a JNDI provider.
 *
 * Note that TIBCO Enterprise Message Service's JNDI interface only
 * allows lookup of static topics and queues. Static topics and queues are
 * those which have direct entries in the topics and queues configuration
 * files. On the contrary, dynamic topics and queues are created
 * by the applications using wildcard matching and the rules of
 * properties inheritance and do not have explicit entries in the
 * configuration files.
 *
 * Also note that JNDI lookup of topics and queues may fail if
 * the configuration contains a topic and a queue with the same name.
 * In this case, the application should use name qualifiers such as
 * $topics and $queues as to qualify as demonstrated by this sample.
 *
 * This sample assumes the use of the original sample configuration
 * files distributed with TIBCO Enterprise Message Service software. If the
 * configuration is altered this sample may not work as expected.
 *
 *
 * Usage:  java tibjmsJNDI  [-provider <providerUrl>]
 *
 *    where options are:
 *
 *      -provider   Provider URL used for JNDI access
 *                  to administered objects.
 *                  If not specified this sample assumes a
 *                  providerUrl of "tibjmsnaming://localhost:7222"
 *
 */

import javax.jms.*;
import javax.naming.*;
import java.util.*;


public class tibjmsJNDI
{
    static final String  providerContextFactory =
                            "com.tibco.tibjms.naming.TibjmsInitialContextFactory";

    static final String  defaultProviderURL =
                                            "tibjmsnaming://localhost:7222";


    String      providerUrl     = null;
    String      userName        = null;
    String      password        = null;


    public tibjmsJNDI(String[] args)
    {
        parseArgs(args);

        if (providerUrl == null)
           providerUrl = defaultProviderURL;

        System.err.println("Using JNDI with TIBCO EMS sample.");
        System.err.println("Using server: "+providerUrl);

        try
        {
            /*
             * Init JNDI Context.
             */
            Hashtable<String,String> env = new Hashtable<String,String>();
            env.put(Context.INITIAL_CONTEXT_FACTORY, providerContextFactory);
            env.put(Context.PROVIDER_URL, providerUrl);

            if (userName != null)
            {
               env.put(Context.SECURITY_PRINCIPAL, userName);

               if (password != null)
                  env.put(Context.SECURITY_CREDENTIALS, password);
            }

            InitialContext jndiContext = new InitialContext(env);

            /*
             * Lookup connection factory which must exist in the factories
             * config file.
             */
            ConnectionFactory factory =
                (ConnectionFactory)jndiContext.lookup("ConnectionFactory");
            System.err.println("OK - successfully did lookup ConnectionFactory, " + factory);

            /*
             * Let's create a connection to the server and a session to
             * verify the server is running so we can continue with our lookup
             * operations.
             */
            Connection connection = null;
            Session    session    = null;

            try
            {
                connection = factory.createConnection(userName,password);
                session    = connection.createSession();
            }
            catch (JMSException e)
            {
                System.err.println("**JMSException occurred while creating connection");
                System.err.println("  Most likely the server is not running, the exception trace follows:");
                e.printStackTrace();

                /* Any lookup would fail, exit. */
                System.exit(0);
            }

            /*
             * Lookup topic 'topic.sample' which must exist in the
             * topics configuration file, if not successful then
             * most likely such topic does not exist.
             */
            javax.jms.Topic sampleTopic = null;

            try
            {
                sampleTopic =
                    (javax.jms.Topic)jndiContext.lookup("topic.sample");
                System.err.println("OK - successfully did lookup topic 'topic.sample'");
            }
            catch (NamingException ne)
            {
                ne.printStackTrace();

                System.err.println("**NamingException occurred while looking up the topic topic.sample");
                System.err.println("  Most likely such topic does not exist in your configuration.");
                System.err.println("  Exception message follows:");
                System.err.println(ne.getMessage());
                System.exit(0);
            }

            /*
             * Lookup queue 'queue.sample' which must exist in the
             * queues configuration file, if not successful then it
             * does not exist...
             */
            javax.jms.Queue sampleQueue = null;

            try
            {
                sampleQueue =
                    (javax.jms.Queue)jndiContext.lookup("queue.sample");
                System.err.println("OK - successfully did lookup queue 'queue.sample'");
            }
            catch (NamingException ne)
            {
                System.err.println("**NamingException occurred while looking up the queue queue.sample");
                System.err.println("  Most likely such queue does not exist in your configuration.");
                System.err.println("  Exception message follows:");
                System.err.println(ne.getMessage());
                System.exit(0);
            }

            /*
             * Try to lookup a topic when there is a topic *and* a queue with same
             * name exist in the configuration. The sample configuration has both
             * topic and queue with the name 'sample' used here. If everything is OK,
             * we will get an exception, then try to lookup those objects using
             * fully-qualified names.
             */

            try
            {
                Object object = jndiContext.lookup("sample");

                /* If sample config was not altered, we should not get to
                 * this line because lookup() should throw an exception.
                 */

                System.err.println("Object named 'sample' has been looked up as: "+object);

            }
            catch (NamingException ne)
            {
                System.err.println("OK - NamingException occurred while looking up the object named 'sample'");
                System.err.println("     This is the CORRECT BEHAVIOUR if the exception message below");
                System.err.println("     specifies name conflict situation. Exception message follows:");
                System.err.println("     "+ne.getMessage());

                /* Let's look them up using name qualifiers */
                try
                {
                    javax.jms.Topic t =
                        (javax.jms.Topic)jndiContext.lookup("$topics.sample");
                    System.err.println("OK - successfully did lookup topic '$topics.sample'");
                    javax.jms.Queue q =
                        (javax.jms.Queue)jndiContext.lookup("$queues.sample");
                    System.err.println("OK - successfully did lookup queue '$queues.sample'");

                }
                catch (NamingException nex)
                {
                    /* These may not be configured: Print a warning and continue. */
                    System.err.println("**NamingException occurred while looking up the topic or queue 'sample'");
                    System.err.println("  Exception message follows:");
                    System.err.println("  "+nex.getMessage());
                }
            }

            /*
             * Now let's try to lookup a topic which definitely does not exist.
             * A lookup() operation will throw an exception and then we try
             * to create such topic explicitly with the createTopic() method.
             */

            String dynamicName = "topic.does.not.exist."+System.currentTimeMillis();
            javax.jms.Topic dynamicTopic = null;

            try
            {
                dynamicTopic =
                    (javax.jms.Topic)jndiContext.lookup(dynamicName);

                /* If sample config was not altered, we should not get to
                 * this line because lookup() should throw an exception.
                 */
                System.err.println("**Error: dynamic topic "+dynamicName+" found by lookup");

            }
            catch (NamingException ne)
            {
                /* This is OK. */
                System.err.println("OK - could not lookup dynamic topic "+dynamicName);
            }

            /*
             * Try to create it.
             */
            try
            {
                dynamicTopic = session.createTopic(dynamicName);
                System.err.println("OK - successfully created dynamic topic "+dynamicName);
            }
            catch (JMSException e)
            {
                /*
                 *  A few reasons are possible here. Maybe there is no > entry
                 *  in the topics configuration or the server security is turned
                 *  on. If the original sample configuration is used, this should work.
                 */
                System.err.println("**JMSException occurred while creating topic "+dynamicName);
                System.err.println("  Please verify your topics configuration and security settings.");
                e.printStackTrace();
                System.exit(0);
            }

            connection.close();

            System.err.println("OK - Done.");
        }
        catch (JMSException e)
        {
            e.printStackTrace();
            System.exit(0);
        }
        catch (NamingException e)
        {
            e.printStackTrace();
            System.exit(0);
        }
    }

    public static void main(String args[])
    {
        tibjmsJNDI t = new tibjmsJNDI(args);
    }

    void usage()
    {
        System.err.println("\nUsage: java tibjmsJNDI [options]");
        System.err.println("");
        System.err.println("  where options are:");
        System.err.println("");
        System.err.println(" -provider  <provider URL> - JNDI provider URL, default is local server");
        System.err.println(" -user      <user name>    - user name, default is null");
        System.err.println(" -password  <password>     - password, default is null");
        System.exit(0);
    }

    void parseArgs(String[] args)
    {
        int i=0;

        while (i < args.length)
        {
            if (args[i].compareTo("-provider")==0)
            {
                if ((i+1) >= args.length) usage();
                providerUrl = args[i+1];
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
}
