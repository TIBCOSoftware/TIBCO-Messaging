/* 
 * Copyright (c) 2002-$Date: 2016-12-13 17:00:37 -0600 (Tue, 13 Dec 2016) $ TIBCO Software Inc. 
 * Licensed under a BSD-style license. Refer to [LICENSE]
 * For more information, please contact:
 * TIBCO Software Inc., Palo Alto, California, USA
 * 
 * $Id: tibjmsSSLJNDI.java 90180 2016-12-13 23:00:37Z olivierh $
 * 
 */

/*
 * This is a simple sample of SSL connectivity for
 * JMS clients. It demonstrates both ssl-based JNDI
 * lookups as well as ssl-based client/server communications.
 *
 * This sample establishes an SSL connection, sends and
 * receives back a single message.
 *
 * If not specified this sample assumes a provider URL of
 *                  "tibjmsnaming://localhost:7243".
 * Note that your TIBCO EMS server must have been configured with an SSL port
 * of 7243.
 *
 * Required command line arguments:
 *
 *
 *      -factory          Name of a connection factory to look up, that will
 *                        create ssl-based connections. It must have previously
 *                        been created. The example usage below uses a factory
 *                        name that is provided in the default factories.conf
 *                        sample file.
 *
 * Example usage:
 *
 *  java tibjmsSSLJNDI -factory SSLConnectionFactory
 */

import java.util.*;
import java.security.*;
import javax.jms.*;
import javax.naming.*;

import com.tibco.tibjms.naming.*;

public class tibjmsSSLJNDI
{
    String      sslProviderUrl  = "tibjmsnaming://localhost:7243";
    String      userName        = null;
    String      password        = null;
    String      factoryName     = null;
    String      topicName       = "topic.sample";

    // SSL options
    boolean         ssl_trace                = false;
    boolean         ssl_debug_trace          = false;
    Vector<String>  ssl_trusted              = new Vector<String>();
    String          ssl_identity             = null;
    String          ssl_key                  = null;
    String          ssl_password             = null;
    String          ssl_vendor               = null;
    boolean         disable_verify_host_name = false;
    boolean         disable_verify_host      = false;


    /**
     * This method creates a java.Security.SecureRandom object seeded with
     * the current time.  It allows the samples that use SSL to initialize
     * the SSL environment much faster than if they had to generate a truly
     * random seed.
     *
     * NOTE: THIS SHOULD NOT BE USED IN A PRODUCTION ENVIRONMENT AS IT IS
     *       NOT SECURE.
     */
    public static SecureRandom createUnsecureRandom()
         throws JMSSecurityException
    {
        try
        {
             SecureRandom sr = SecureRandom.getInstance("SHA1PRNG");
             sr.setSeed(System.currentTimeMillis());

            return sr;

        }
        catch (NoSuchAlgorithmException e)
        {
          JMSSecurityException jmse = new JMSSecurityException(
                    "Error creating SecureRandom object: " + e.getMessage());
          jmse.setLinkedException(e);
          throw jmse;
        }
    }

    public tibjmsSSLJNDI(String[] args)
    {
        parseArgs(args);

        if (factoryName == null)
        {
            System.err.println("Error: must specify factory name");
            usage();
        }

        if (topicName == null)
        {
            System.err.println("Error: must specify topic name");
            usage();
        }

        System.err.println("SSL with JNDI sample.");

        // initialize SSL environment
        Hashtable<String,Object> environment = new Hashtable<String,Object>();
        try
        {
            if (userName != null)
            {
                environment.put(Context.SECURITY_PRINCIPAL,userName);
            
                if (password != null)
                    environment.put(Context.SECURITY_CREDENTIALS,password);
            }
            
            // set SSL vendor
            if (ssl_vendor != null)
                environment.put(TibjmsContext.SSL_VENDOR, ssl_vendor);

            // set trace for client-side operations, loading of certificates
            // and other
            if (ssl_trace)
                environment.put(TibjmsContext.SSL_TRACE, new Boolean(true));

            // set vendor trace. Has no effect for "j2se", "entrust6" uses
            // this to trace SSL handshake
            if (ssl_debug_trace)
                environment.put(TibjmsContext.SSL_DEBUG_TRACE, new Boolean(true));

            // set trusted certificates if specified
            if (ssl_trusted.size() != 0)
                environment.put(TibjmsContext.SSL_TRUSTED_CERTIFICATES, ssl_trusted);

            // set client identity if specified. ssl_key may be null
            // if identity is PKCS12, JKS or EPF. 'j2se' only supports
            // PKCS12 and JKS. 'entrust6' also supports PEM and PKCS8.
            if (ssl_identity != null)
            {
                if (ssl_password == null)
                {
                    System.err.println("Error: must specify -ssl_password if identity is set");
                    System.exit(-1);
                }
                environment.put(TibjmsContext.SSL_IDENTITY, ssl_identity);
                environment.put(TibjmsContext.SSL_PASSWORD, ssl_password);

                if (ssl_key != null)
                   environment.put(TibjmsContext.SSL_PRIVATE_KEY, ssl_key);
            }

            if (disable_verify_host_name)
              environment.put(TibjmsContext.SSL_ENABLE_VERIFY_HOST_NAME, new Boolean(false));

            if (disable_verify_host || ssl_trusted.size()==0)
               environment.put(TibjmsContext.SSL_ENABLE_VERIFY_HOST, new Boolean(false));

            /*
             * Install our own random number generator which is fast but not secure!
             */
            com.tibco.tibjms.TibjmsSSL.setSecureRandom(createUnsecureRandom());
        }
        catch (Exception e)
        {
            e.printStackTrace();

            if (e instanceof JMSException)
            {
                JMSException je = (JMSException)e;
                if (je.getLinkedException() != null)
                {
                    System.err.println("##### Linked Exception:");
                    je.getLinkedException().printStackTrace();
                }
            }
            System.exit(-1);
        }

        try
        {
            environment.put(Context.INITIAL_CONTEXT_FACTORY, "com.tibco.tibjms.naming.TibjmsInitialContextFactory");
            environment.put(Context.PROVIDER_URL, sslProviderUrl);
            environment.put(Context.URL_PKG_PREFIXES, "com.tibco.tibjms.naming");

            // specify ssl as the security protocol to us by the Initial Context
            environment.put(TibjmsContext.SECURITY_PROTOCOL, "ssl");

            Context context = new InitialContext(environment);

            // use SSL to lookup the ConnectionFactory
            Object object = context.lookup(factoryName);

            if (!(object instanceof ConnectionFactory))
               throw new NamingException("Expected ConnectionFactory but found: " + object.getClass().getName());
            
            ConnectionFactory factory    = (ConnectionFactory)object;
            Connection        connection = factory.createConnection(userName,password);
            Session           session    = connection.createSession();
            javax.jms.Topic   topic      = session.createTopic(topicName);
            MessageProducer   producer   = session.createProducer(topic);
            MessageConsumer   consumer   = session.createConsumer(topic);

            connection.start();

            javax.jms.MapMessage message = session.createMapMessage();
            message.setStringProperty("field","SSL message");

            for (int i=0; i<3; i++)
            {
                producer.send(message);
                System.err.println("\nPublished message: "+message);

                /* receive the same message back */
                message = (javax.jms.MapMessage)consumer.receive();
                if (message == null)
                    System.err.println("\nCould not receive message");
                else
                    System.err.println("\nReceived message: "+message);

                try
                {
                    Thread.sleep(1000);
                }
                catch (Exception e){}
            }

            connection.close();
        }
        catch (JMSException e)
        {
            e.printStackTrace();

            if (e.getLinkedException() != null)
            {
                System.err.println("##### Linked Exception:");
                e.getLinkedException().printStackTrace();
            }
            System.exit(-1);
        }
        catch (NamingException e)
        {
            e.printStackTrace();
            System.exit(-1);
        }
    }

    public static void main(String args[])
    {
        tibjmsSSLJNDI t = new tibjmsSSLJNDI(args);
    }

    void usage()
    {
        System.err.println("\nUsage: java tibjmsSSLJNDI [options]");
        System.err.println("");
        System.err.println("   where options are:");
        System.err.println("");
        System.err.println(" -provider_url  <provider URL> - JNDI provider URL, default is local server");
        System.err.println(" -user          <user name>    - user name, default is null");
        System.err.println(" -password      <password>     - password, default is null");
        System.err.println(" -factory       <name>         - connection factory name, required");
        System.err.println(" -topic         <topic-name>   - topic name, default is \"topic.sample\"");
        System.err.println("");
        System.err.println("  SSL options:");
        System.err.println("");
        System.err.println(" -ssl_vendor       <name>      - SSL vendor: 'j2se' or 'entrust6',");
        System.err.println("                                 default is 'j2se'");
        System.err.println(" -ssl_trace                    - trace SSL initialization");
        System.err.println(" -ssl_debug_trace              - trace SSL handshake and related");
        System.err.println(" -ssl_trusted      <file-name> - file with trusted certificate(s),");
        System.err.println("                                 this parameter may repeat if more");
        System.err.println("                                 than one file required");
        System.err.println(" -ssl_identity     <file-name> - client identity file");
        System.err.println(" -ssl_key          <file-name> - client key file (optional)");
        System.err.println(" -ssl_password     <string>    - password to decrypt client identity");
        System.err.println("                                 or key file");
        System.err.println(" -no_verify_host_name          - disable host name verification");
        System.err.println(" -no_verify_host               - disable host verification");
        System.exit(0);
    }

    void parseArgs(String[] args)
    {
        int i=0;

        while (i < args.length)
        {
            if (args[i].compareTo("-provider_url")==0)
            {
                if ((i+1) >= args.length) usage();
                sslProviderUrl = args[i+1];
                i += 2;
            }
            else
            if (args[i].compareTo("-factory")==0)
            {
                if ((i+1) >= args.length) usage();
                factoryName = args[i+1];
                i += 2;
            }
            else
            if (args[i].compareTo("-topic")==0)
            {
                if ((i+1) >= args.length) usage();
                topicName = args[i+1];
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
            if (args[i].compareTo("-ssl_trace")==0)
            {
                ssl_trace = true;
                i += 1;
            }
            else
            if (args[i].compareTo("-ssl_debug_trace")==0)
            {
                ssl_debug_trace = true;
                i += 1;
            }
            else
            if (args[i].compareTo("-ssl_trusted")==0)
            {
                if ((i+1) >= args.length) usage();
                ssl_trusted.addElement(args[i+1]);
                i += 2;
            }
            else
            if (args[i].compareTo("-ssl_identity")==0)
            {
                if ((i+1) >= args.length) usage();
                ssl_identity = args[i+1];
                i += 2;
            }
            else
            if (args[i].compareTo("-ssl_key")==0)
            {
                if ((i+1) >= args.length) usage();
                ssl_key = args[i+1];
                i += 2;
            }
            else
            if (args[i].compareTo("-ssl_password")==0)
            {
                if ((i+1) >= args.length) usage();
                ssl_password = args[i+1];
                i += 2;
            }
            else
            if (args[i].compareTo("-ssl_vendor")==0)
            {
                if ((i+1) >= args.length) usage();
                ssl_vendor = args[i+1];
                i += 2;
            }
            else
            if (args[i].compareTo("-no_verify_host_name")==0)
            {
                disable_verify_host_name = true;
                i += 1;
            }
            else
            if (args[i].compareTo("-no_verify_host")==0)
            {
                disable_verify_host = true;
                i += 1;
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
