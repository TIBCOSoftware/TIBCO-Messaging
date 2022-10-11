/* 
 * Copyright (c) 2002-$Date: 2016-12-13 17:00:37 -0600 (Tue, 13 Dec 2016) $ TIBCO Software Inc. 
 * Licensed under a BSD-style license. Refer to [LICENSE]
 * For more information, please contact:
 * TIBCO Software Inc., Palo Alto, California, USA
 * 
 * $Id: tibjmsJNDIRead.java 90180 2016-12-13 23:00:37Z olivierh $
 * 
 */

 /*
  * Example of how to use JNDI to read TIBCO Enterprise Message Service
  * administered objects from a foreign naming or directory service,
  * and then use them to send messages.
  *
  * Objects are looked up from the foreign JNDI using the names that were provided
  * on the command line, unless LDAP is being used as the foreign JNDI store. In
  * the case of LDAP, the objects are looked up with the string "cn=" prepended
  * to the name, as required by LDAP.
  *
  * Example LDAP usage:
  *
  *   java tibjmsJNDIRead -factory com.sun.jndi.ldap.LdapCtxFactory -url ldap://localhost:3535/o=JNDITutorial -dest myDest
  *
  * Example File System usage:
  *
  *   java tibjmsJNDIRead -factory com.sun.jndi.fscontext.RefFSContextFactory -url file:/tmp/tutorial -dest myDest
  *
  * Example RMI Registry usage:
  *
  *   java tibjmsJNDIRead -factory com.sun.jndi.rmi.registry.RegistryContextFactory -url rmi://localhost:1099 -dest myDest
  */

import java.util.*;

import javax.naming.*;
import javax.jms.*;


public class tibjmsJNDIRead
{
    /* Command Line parameter variables */
    String      foreignProviderFactory = null;
    String      foreignProviderURL     = null;
    String      connectionFactoryName  = null;
    String      destinationName        = null;
    int         iterations             = 1;

    /* Internal variables */
    Context     ctxForeign      = null;
    boolean     usingLDAP       = false;
    String      lookupName;

    /* Constructor */
    public tibjmsJNDIRead(String[] args)
    {
        parseArgs(args);

        /*
         * Check for required command line parameters
         */
        if (foreignProviderFactory == null || foreignProviderURL == null)
        {
            System.err.println("Error: must specify both foreign provider factory and foreign provider url");
            usage();
        }

        if (connectionFactoryName == null && destinationName == null)
        {
            System.err.println("Error: must specify at least one of: connection factory or destination name");
            usage();
        }

        System.err.println("Using JNDI to read objects from a foreign naming/directory service sample.");
        System.err.println("Using server: "+foreignProviderURL);

        ConnectionFactory       factory     = null;
        Destination             destination = null;

        if (foreignProviderURL.substring(0, 5).equals("ldap:"))
           usingLDAP = true;

        try
        {
            /*
             * Init Foreign Naming/Directory Context.
             */
            ctxForeign = createInitialContext(foreignProviderFactory, foreignProviderURL);

            /*
             * Add new properties to foreign provider context for TIBCO Object Factories (very important!)
             * This allows the foreign JNDI provider to invoke the TIBCO Object Factories when objects are
             * looked up in the foreign directory.
             */
            ctxForeign.addToEnvironment(Context.OBJECT_FACTORIES, "com.tibco.tibjms.naming.TibjmsObjectFactory");
            ctxForeign.addToEnvironment(Context.URL_PKG_PREFIXES, "com.tibco.tibjms.naming");

            for (int i=0; i<iterations; i++)
            {
                if (connectionFactoryName != null)
                {
                    lookupName = (usingLDAP) ? "cn=" + connectionFactoryName : connectionFactoryName;
                    factory    = (ConnectionFactory)ctxForeign.lookup(lookupName);
                    System.err.println("looked up connection factory = " + factory);
                }

                if (destinationName != null)
                {
                    lookupName  = (usingLDAP) ? "cn=" + destinationName : destinationName;
                    destination = (Destination)ctxForeign.lookup(lookupName);
                    System.err.println("looked up destination = " + destination);
                }
            }
        }
        catch (NamingException e)
        {
            e.printStackTrace();
            System.exit(0);
        }

        try
        {
            /*
             * Send a message on the corresponding destination.
             */
            if (factory instanceof ConnectionFactory && destination != null)
            {
                ConnectionFactory cf = (ConnectionFactory)factory;

                Connection      connection      = cf.createConnection();
                Session         session         = connection.createSession();
                MessageProducer messageProducer = session.createProducer(destination);
                String          string          = "Hello World";
                TextMessage     message         = session.createTextMessage(string);
                
                messageProducer.send(message);
                System.err.println("Sent message '" + string + "' on " + destination);
            }
        }
        catch (JMSException e)
        {
            e.printStackTrace();
            System.exit(0);
        }
    }

    private static Context createInitialContext(String contextFactory,
        String providerUrl) throws NamingException
    {

        Hashtable<String,String> env = new Hashtable<String,String>();
        env.put(Context.INITIAL_CONTEXT_FACTORY, contextFactory);
        env.put(Context.PROVIDER_URL, providerUrl);
        env.put(Context.REFERRAL, "throw");

        Context ctx = new InitialContext(env);

        return ctx;
    }

    public static void main(String args[])
    {
        tibjmsJNDIRead t = new tibjmsJNDIRead(args);
    }

    void usage()
    {
        System.err.println("\nUsage: java tibjmsJNDIRead [options]");
        System.err.println("");
        System.err.println("  where options are:");
        System.err.println("");
        System.err.println(" -user      <user name>   - user name, default is null");
        System.err.println(" -password  <password>    - password, default is null");
        System.err.println(" -factory   <factory>     - foreign JNDI context factory, required");
        System.err.println(" -url       <URL>         - foreign provider URL, required");
        System.err.println(" -cfactory  <factory>     - connection factory to look up   [at least one of");
        System.err.println(" -dest      <destination> - destination name to look up      these is required]");
        System.err.println(" -iterations <#>          - # of lookup iterations");

        System.exit(0);
    }

    void parseArgs(String[] args)
    {
        int i=0;

        while (i < args.length)
        {
            if (args[i].compareTo("-factory")==0)
            {
                if ((i+1) >= args.length) usage();
                foreignProviderFactory = args[i+1];
                i += 2;
            }
            else
            if (args[i].compareTo("-url")==0)
            {
                if ((i+1) >= args.length) usage();
                foreignProviderURL = args[i+1];
                i += 2;
            }
            else
            if (args[i].compareTo("-cfactory")==0)
            {
                if ((i+1) >= args.length) usage();
                connectionFactoryName = args[i+1];
                i += 2;
            }
            else
            if (args[i].compareTo("-dest")==0)
            {
                if ((i+1) >= args.length) usage();
                destinationName = args[i+1];
                i += 2;
            }
            else
            if (args[i].compareTo("-iterations")==0)
            {
                if ((i+1) >= args.length) usage();
                String its = args[i+1];
                iterations = Integer.parseInt(its);
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
