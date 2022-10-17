/* 
 * Copyright (c) 2001-$Date: 2017-03-27 16:01:48 -0500 (Mon, 27 Mar 2017) $ TIBCO Software Inc. 
 * Licensed under a BSD-style license. Refer to [LICENSE]
 * For more information, please contact:
 * TIBCO Software Inc., Palo Alto, California, USA
 * 
 * $Id: tibjmsJNDIStore.java 92619 2017-03-27 21:01:48Z olivierh $
 * 
 */

 /*
  * Example of how to use JNDI to store TIBCO Enterprise Message Service
  * administered objects in a foreign naming/directory service.
  *
  * This program looks up administered objects from the TIBCO JMS server
  * and stores them in a foreign naming or directory service.
  * In order to use this program you must have installed some third-party
  * naming or directory service such as: file system, RMI registry
  * or LDAP. You also need to have installed a JNDI service provider
  * for that service.
  * The Java 2 SDK, v1.3 comes with JNDI providers for LDAP and RMI
  * registry, among others. The file system service provider can be
  * obtained from the JNDI Web site:
  *
  * http://java.sun.com/products/jndi/serviceproviders.html.
  *
  * If a connection factory, topic or queue is specified with the -cfactory,
  * -topic or -queue command line options, then that connection factory,
  * topic or queue will be looked up in the TIBCO EMS server
  * and stored in the foreign naming/directory service.
  * It will be stored in the form of a URL reference to the source of the
  * original object, i.e. the TIBCO EMS server.
  *
  * If a topic or queue is specified with the -newtopic or -newqueue
  * command line options, then that topic or queue will be created locally
  * and stored in the foreign directory.  In this case it will be stored as
  * a reference to an object factory that can re-create the object locally.
  *
  * Objects are bound into the foreign JNDI using the same names that were
  * used to look them up, unless LDAP is being used as the foreign JNDI store.
  * In the case of LDAP, the objects are bound with the string "cn="
  * prepended to the name, as required by LDAP.
  *
  * When the object is subsequently looked up in the foreign directory
  * (via another program), the foreign directory JNDI provider will look
  * up the object differently, depending on how it was stored.  If it was
  * stored as a URL reference to its original location, then the foreign JNDI
  * provider will look it up from its original location by following the URL
  * (the TIBCO JMS server must be running for this to work).  On the other
  * hand, if the object was stored as a local reference, then the object
  * factory will create the object locally.
  *
  * Example LDAP usage:
  *
  *   java tibjmsJNDIStore -factory com.sun.jndi.ldap.LdapCtxFactory
  *                        -url ldap://localhost:3535/o=JNDITutorial
  *                        -topic topic.sample
  *
  * Example File System usage:
  *
  *   java tibjmsJNDIStore -factory com.sun.jndi.fscontext.RefFSContextFactory
  *                        -url file:/tmp/tutorial
  *                        -topic topic.sample
  *
  */

import java.util.*;

import javax.naming.*;
import javax.jms.*;

import com.tibco.tibjms.*;

public class tibjmsJNDIStore
{
    private static final String defaultProviderURL = "tibjmsnaming://localhost:7222";
    private static final String defaultServerURL   = "tcp://localhost:7222";

    /* Command Line parameters */
    String      providerUrl            = null;
    String      userName               = null;
    String      password               = null;
    String      foreignProviderFactory = null;
    String      foreignProviderURL     = null;
    String      foreignUserName        = null;
    String      foreignPassword        = null;
    String      connectionFactoryName  = null;
    String      topicName              = null;
    String      queueName              = null;
    String      newTopicName           = null;
    String      newQueueName           = null;
    String      newFactoryName         = null;
    String      serverUrl              = null;
    String      clientId               = null;


    /* Internal variables */
    Context     ctxTibjms       = null;
    Context     ctxForeign      = null;
    boolean     usingLDAP       = false;
    String      bindName;

    /* Constructor */
    public tibjmsJNDIStore(String[] args)
    {
        parseArgs(args);

        if (providerUrl == null) providerUrl = defaultProviderURL;

        if (newFactoryName != null)
        {
           if (serverUrl == null)
              serverUrl = defaultServerURL;
        }

        /*
         * Check for required command line parameters
         */
        if (foreignProviderFactory == null || foreignProviderURL == null) {
            System.err.println("Error: must specify both foreign "+
                               "provider factory and foreign provider url");
            usage();
        }

        if (connectionFactoryName == null && topicName == null &&
           queueName == null && newTopicName == null &&
           newQueueName == null && newFactoryName == null)
        {
            System.err.println("Error: must specify at least one of: "+
                               "connection factory, topic, queue, "+
                               "new topic or new queue name or new factory name");
            usage();
        }

        System.err.println("Using JNDI to store and lookup objects"+
                           " in a foreign naming/directory service sample.");

        System.err.println("Reading from tibjms JNDI provider: "+
                           providerUrl);

        System.err.println("Storing to naming/directory service: "+
                           foreignProviderURL);

        if (foreignProviderURL.substring(0, 5).equals("ldap:"))
           usingLDAP = true;

        try
        {
            /*
             * Init Tibjms JNDI Context.
             */
            ctxTibjms = createInitialContext(
                            "com.tibco.tibjms.naming.TibjmsInitialContextFactory",
                            providerUrl, userName, password);

            /*
             * Init Foreign Naming/Directory Context.
             */
             ctxForeign = createInitialContext(foreignProviderFactory,
                                               foreignProviderURL, foreignUserName,
                                               foreignPassword);

             if (connectionFactoryName != null)
             {
                /* look it up */
                ConnectionFactory factory =
                     (ConnectionFactory)ctxTibjms.lookup(connectionFactoryName);

                System.err.println("Retrieved connection factory = "+factory);

                /* bind it */
                try
                {
                    bindName = (usingLDAP) ? "cn=" + connectionFactoryName :
                                                     connectionFactoryName;
                    ctxForeign.bind(bindName, factory);

                    System.err.println("Bound connection factory " + factory +
                                       " to name: " + bindName);
                }
                catch (NameAlreadyBoundException e)
                {
                    System.err.println("Warning, name '"+connectionFactoryName+
                                       "' already bound, rebinding...");

                    ctxForeign.rebind(bindName, factory);

                    System.err.println("Re-bound connection factory " +
                                       factory + " to name: " + bindName);
                }
             }

             if (topicName != null)
             {
                /* look it up */
                Topic topic = (Topic)ctxTibjms.lookup(topicName);
                System.err.println("Retrieved topic = " + topic);

                /* bind it */
                try
                {
                    bindName = (usingLDAP) ? "cn=" + topicName : topicName;

                    ctxForeign.bind(bindName, topic);

                    System.err.println("Bound topic " + topic +
                                       " to name: " + bindName);

                }
                catch (NameAlreadyBoundException e)
                {
                    System.err.println("Warning, name '"+topicName+
                                       "' already bound, rebinding...");

                    ctxForeign.rebind(bindName, topic);

                    System.err.println("Re-bound topic " +
                                       topic + " to name: " + bindName);
                }
             }

             if (queueName != null)
             {
                /* look it up */
                javax.jms.Queue queue = (javax.jms.Queue)ctxTibjms.lookup(queueName);
                System.err.println("Retrieved queue = " + queue);

                /* bind it */
                try
                {
                    bindName = (usingLDAP) ? "cn=" + queueName : queueName;

                    ctxForeign.bind(bindName, queue);

                    System.err.println("Bound queue " + queue +
                                       " to name: " + bindName);
                }
                catch (NameAlreadyBoundException e)
                {
                    System.err.println("Warning, name '" + queueName +
                                       "' already bound, rebinding...");

                    ctxForeign.rebind(bindName, queue);

                    System.err.println("Re-bound queue " + queue +
                                       " to name: " + bindName);
                }
             }

             if (newTopicName != null)
             {
                /* create a new topic locally */
                Topic newTopic = new TibjmsTopic(newTopicName);
                System.err.println("Created topic = " + newTopic);

                /* Bind it */
                try
                {
                    bindName = (usingLDAP) ? "cn=" + newTopicName : newTopicName;

                    ctxForeign.bind(bindName, newTopic);

                    System.err.println("Bound topic " + newTopic +
                                       " to name: " + bindName);
                }
                catch (NameAlreadyBoundException e)
                {
                    System.out.println("Warning, name '" + newTopicName +
                                       "' already bound, rebinding...");

                    ctxForeign.rebind(bindName, newTopic);

                    System.err.println("Re-bound topic " + newTopic +
                                       " to name: " + bindName);
                }
             }

             if (newQueueName != null)
             {
                /* create a new queue locally */
                javax.jms.Queue newQueue = new TibjmsQueue(newQueueName);
                System.err.println("Created queue = " + newQueue);

                try
                {
                    bindName = (usingLDAP) ? "cn="+newQueueName :
                                             newQueueName;

                    ctxForeign.bind(bindName, newQueue);

                    System.err.println("Bound queue "+newQueue+
                                       " to name: "+bindName);
                }
                catch (NameAlreadyBoundException e)
                {
                    System.out.println("Warning, name '" + newQueueName +
                                       "' already bound, rebinding...");

                    ctxForeign.rebind(bindName, newQueue);

                    System.err.println("Re-bound queue " + newQueue +
                                       " to name: " + bindName);
                }
             }

             if (newFactoryName != null)
             {
                /* 
                 * create a new connection factory locally
                 */
                 
                /* 
                 * Create and populate your Map object here...
                 */
                Map<String,String> properties = new HashMap<String,String>();
                properties.put("myProp", "test property");

                ConnectionFactory newFactory =
                                new TibjmsConnectionFactory(serverUrl,
                                                            clientId,
                                                            properties);

                System.err.println("Created connection factory = " +
                                   newFactory);

                try
                {
                    bindName = (usingLDAP) ? "cn="+newFactoryName :
                                             newFactoryName;

                    ctxForeign.bind(bindName, newFactory);

                    System.err.println("Bound connection factory " +
                                       newFactory + " to name: " + bindName);
                }
                catch (NameAlreadyBoundException e)
                {
                    System.out.println("Warning, name '" + newFactoryName +
                                       "' already bound, rebinding...");
                    ctxForeign.rebind(bindName, newFactory);
                    System.err.println("Re-bound connection factory " +
                                       newFactory + " to name: " + bindName);
                }
             }
        }
        catch(NamingException e)
        {
            e.printStackTrace();
            System.exit(0);
        }
    }

    private static Context createInitialContext(String contextFactory,
                                                String providerUrl,
                                                String userName,
                                                String password)
                                                  throws NamingException
    {
        Hashtable<String,String> env = new Hashtable<String,String>();
        env.put(Context.INITIAL_CONTEXT_FACTORY, contextFactory);
        env.put(Context.PROVIDER_URL, providerUrl);
        env.put(Context.REFERRAL, "throw");

        if (userName != null)
        {
           env.put(Context.SECURITY_PRINCIPAL, userName);

           if (password != null)
              env.put(Context.SECURITY_CREDENTIALS, password);
        }

        Context ctx = new InitialContext(env);

        return ctx;
    }

    public static void main(String args[])
    {
        tibjmsJNDIStore t = new tibjmsJNDIStore(args);
    }

    void usage()
    {
        System.err.println("\nUsage: java tibjmsJNDIStore [options]");
        System.err.println("");
        System.err.println("  where options are:");
        System.err.println("");
        System.err.println(" -provider  <provider URL> - JNDI provider URL, default is local JNDI provider");
        System.err.println(" -user      <user name>    - user name, default is null");
        System.err.println(" -password  <password>     - password, default is null");
        System.err.println(" -factory   <factory>      - foreign JNDI context factory, required");
        System.err.println(" -url       <URL>          - foreign provider URL, required");
        System.err.println(" -fuser     <user name>    - foreign provider user name, default is null");
        System.err.println(" -fpassword <password>     - foreign provider password, default is null");
        System.err.println(" -cfactory  <factory>      - connection factory to look up and store");
        System.err.println(" -topic     <topic name>   - topic to look up and store in foreign service");
        System.err.println(" -queue     <queue name>   - queue to look up and store in foreign service");
        System.err.println(" -newtopic  <topic name>   - topic to create and store in foreign service");
        System.err.println(" -newqueue  <queue name>   - queue to create and store in foreign service");
        System.err.println(" -newconfac <factory name> - connection factory to create and store");
        System.err.println(" -server    <server URL>   - JMS server URL for new topic or queue connection");
        System.err.println("                             factory, default is local server");
        System.err.println(" -clientId  <client ID>    - optional client ID for new topic or");
        System.err.println("                             queue connection factory");
        System.exit(0);
    }

    void parseArgs(String[] args)
    {
        int i=0;

        while(i < args.length)
        {
            if (args[i].compareTo("-provider")==0)
            {
                if ((i+1) >= args.length)
                    usage();
                providerUrl = args[i+1];
                i += 2;
            }
            else if (args[i].compareTo("-user")==0)
            {
                if ((i+1) >= args.length)
                    usage();
                userName = args[i+1];
                i += 2;
            } 
            else if (args[i].compareTo("-password")==0)
            {
                if ((i+1) >= args.length)
                    usage();
                password = args[i+1];
                i += 2;
            }
            else if (args[i].compareTo("-factory")==0)
            {
                if ((i+1) >= args.length)
                    usage();
                foreignProviderFactory = args[i+1];
                i += 2;
            }
            else if (args[i].compareTo("-url")==0)
            {
                if ((i+1) >= args.length)
                    usage();
                foreignProviderURL = args[i+1];
                i += 2;
            }
            else if (args[i].compareTo("-fuser")==0)
            {
                if ((i+1) >= args.length)
                    usage();
                foreignUserName = args[i+1];
                i += 2;
            }
            else if (args[i].compareTo("-fpassword")==0)
            {
                if ((i+1) >= args.length)
                    usage();
                foreignPassword = args[i+1];
                i += 2;
            }
            else if (args[i].compareTo("-cfactory")==0)
            {
                if ((i+1) >= args.length)
                    usage();
                connectionFactoryName = args[i+1];
                i += 2;
            }
            else if (args[i].compareTo("-topic")==0)
            {
                if ((i+1) >= args.length)
                    usage();
                topicName = args[i+1];
                i += 2;
            }
            else if (args[i].compareTo("-queue")==0)
            {
                if ((i+1) >= args.length) usage();
                queueName = args[i+1];
                i += 2;
            }
            else if (args[i].compareTo("-newtopic")==0)
            {
                if ((i+1) >= args.length)
                    usage();
                newTopicName = args[i+1];
                i += 2;
            }
            else if (args[i].compareTo("-newqueue")==0)
            {
                if ((i+1) >= args.length)
                    usage();
                newQueueName = args[i+1];
                i += 2;
            }
            else if (args[i].compareTo("-newconfac")==0)
            {
                if ((i+1) >= args.length)
                    usage();
                newFactoryName = args[i+1];
                i += 2;
            }
            else if (args[i].compareTo("-server")==0)
            {
                if ((i+1) >= args.length)
                    usage();
                serverUrl = args[i+1];
                i += 2;
            }
            else if (args[i].compareTo("-clientId")==0)
            {
                if ((i+1) >= args.length)
                    usage();
                clientId = args[i+1];
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
