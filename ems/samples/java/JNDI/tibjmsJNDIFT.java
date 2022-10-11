/* 
 * Copyright (c) 2002-$Date: 2016-12-13 17:00:37 -0600 (Tue, 13 Dec 2016) $ TIBCO Software Inc. 
 * Licensed under a BSD-style license. Refer to [LICENSE]
 * For more information, please contact:
 * TIBCO Software Inc., Palo Alto, California, USA
 * 
 * $Id: tibjmsJNDIFT.java 90180 2016-12-13 23:00:37Z olivierh $
 * 
 */

/*
 * Example demonstrating fault tolerance of JNDI lookups of administered
 * objects. The program simply runs in an infinite loop looking up a
 * ConnectionFactory. It gives you the opportunity to kill the primary server
 * and observe that the lookups continue (from the backup server which has
 * now become the primary). If you then restart the server that you previously
 * killed, it will become the backup. You can then kill the current primary
 * and the other backup will then take over.
 *
 * This program runs forever until you stop it with CTRL-C.
 *
 *
 * Usage:  java tibjmsJNDIFT  [options]
 *
 *    where options are:
 *
 *      -provider   Provider URLs used for JNDI access
 *                  to administered objects on both the primary and backup
 *                  server.  If not specified this sample assumes
 *                  providerUrls of:
 *
 *            "tibjmsnaming://localhost:7222, tibjmsnaming://localhost:7224"
 *
 *      -user       user name, required if authorization is on
 *      -password   password, required if authorization is on
 *      -factory    name of factory to look up, can be specified multiple times
 *                  If not specified, this program uses:
 *
 *                  "FTConnectionFactory"
 *
 */

import javax.jms.*;
import javax.naming.*;
import java.util.*;


public class tibjmsJNDIFT
{
    static final String  providerContextFactory =
                            "com.tibco.tibjms.naming.TibjmsInitialContextFactory";

    static final String  defaultProviderURLs =
                 "tibjmsnaming://localhost:7222, tibjmsnaming://localhost:7224";

    static final String defaultConnectionFactory =
                 "FTConnectionFactory";

    String providerUrls = null;
    String userName     = null;
    String password     = null;
    String [] factory   = null;


    public tibjmsJNDIFT(String[] args)
    {

        parseArgs(args);

        if (providerUrls == null)
           providerUrls = defaultProviderURLs;

        if (factory == null)
            factory = new String [] {defaultConnectionFactory};

        System.err.println("Using JNDI FT with TIBCO EMS sample.");
        System.err.println("Using providers: "+providerUrls);

        try
        {
            /*
             * Init JNDI Context.
             */
            Hashtable<String,String> env = new Hashtable<String,String>();
            env.put(Context.INITIAL_CONTEXT_FACTORY, providerContextFactory);
            env.put(Context.PROVIDER_URL, providerUrls);

            if (userName != null)
            {
                env.put(Context.SECURITY_PRINCIPAL, userName);

                if (password != null)
                    env.put(Context.SECURITY_CREDENTIALS, password);
            }

            InitialContext jndiContext = new InitialContext(env);

            for (;;)
            {
                /*
                 * Look them up
                 */
                for (int j=0; j<factory.length; j++)
                {
                    if (factory[j] != null)
                    {
                        ConnectionFactory connectionFactory =
                        (ConnectionFactory)jndiContext.lookup(factory[j]);
                        System.err.println("OK - successfully did lookup " + factory[j]);
                    }
                }

                System.err.println("\n***This test will loop so that you can test fault tolerant operation.***\n");

                try
                {
                    Thread.currentThread().sleep(500);
                }
                catch (InterruptedException ie)
                {
                    ie.printStackTrace();
                }
            }
        }
        catch (NamingException e)
        {
            e.printStackTrace();
            System.exit(0);
        }
    }

    public static void main(String args[])
    {
        tibjmsJNDIFT t = new tibjmsJNDIFT(args);
    }

    void usage()
    {
        System.err.println("\nUsage: java tibjmsJNDIFT [options]");
        System.err.println("");
        System.err.println("  where options are:");
        System.err.println("");
        System.err.println(" -provider  <provider URLs> - JNDI provider URLs, default is two local servers,");
        System.err.println("                              with ports 7222 and 7224");
        System.err.println(" -user      <user name>     - user name, default is null");
        System.err.println(" -password  <password>      - password, default is null");
        System.err.println(" -factory   <factory name>  - name of factory to look up, can be specified multiple times");
        System.exit(0);
    }

    void parseArgs(String[] args)
    {
        int i=0;
        int j=0;

        while (i < args.length)
        {
            if (args[i].compareTo("-provider")==0)
            {
                if ((i+1) >= args.length) usage();
                providerUrls = args[i+1];
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
            if (args[i].compareTo("-factory")==0)
            {
                if ((i+1) >= args.length) usage();
                if (factory == null)
                   factory = new String [args.length];
                factory[j++] = args[i+1];
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
