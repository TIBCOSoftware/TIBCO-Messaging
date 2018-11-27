/* 
 * Copyright (c) 2001-$Date: 2016-12-13 17:00:37 -0600 (Tue, 13 Dec 2016) $ TIBCO Software Inc. 
 * Licensed under a BSD-style license. Refer to [LICENSE]
 * For more information, please contact:
 * TIBCO Software Inc., Palo Alto, California, USA
 * 
 * $Id: tibjmsSSL.java 90180 2016-12-13 23:00:37Z olivierh $
 * 
 */

/*
 * This is a simple sample of SSL connectivity for
 * JMS clients.  It demonstrates ssl-based client/server communications.
 *
 * This sample establishes an SSL connection, publishes and
 * receives back three messages.
 *
 *      -server     Server URL passed to the TibjmsConnectionFactory
 *                  constructor.
 *                  If not specified this sample assumes a
 *                  serverUrl of "ssl://localhost:7243"
 *
 *
 * Example usage:
 *
 *  java tibjmsSSL -server ssl://localhost:7243
 *
 * This sample can also demonstrate full use of SSL with the mutual
 * authentication. The following command line will cause the client
 * to verify the server certificate and also establishes the client
 * identity which will be verified by the server. In the sample
 * command line below the client-side SSL trace is being enabled,
 * if you also running the server with -ssl_debug_trace parameter
 * you will see the entire authentication trace:
 *
 * java tibjmsSSL -ssl_trace
 *                -ssl_trusted ../certs/server_root.cert.pem
 *                -ssl_hostname server
 *                -ssl_identity ../certs/client_identity.p12
 *                -ssl_password password
 */

import java.util.*;
import javax.jms.*;

public class tibjmsSSL
{
    String      serverUrl       = "ssl://localhost:7243";
    String      userName        = null;
    String      password        = null;
    String      topicName       = "topic.sample";

    // SSL options
    boolean        ssl_trace                = false;
    boolean        ssl_debug_trace          = false;
    Vector<String> ssl_trusted              = new Vector<String>();
    String         ssl_hostname             = null;
    String         ssl_identity             = null;
    String         ssl_key                  = null;
    String         ssl_password             = null;
    String         ssl_vendor               = null;
    String         ssl_ciphers              = null;
    boolean        disable_verify_host_name = false;
    boolean        disable_verify_host      = false;


    public tibjmsSSL(String[] args)
    {

        parseArgs(args);

        if (topicName == null)
        {
            System.err.println("Error: must specify topic name");
            usage();
        }

        System.err.println("SSL sample.");

        // initialize SSL environment
        Hashtable<String,Object> environment = new Hashtable<String,Object>();
        try
        {

            // set SSL vendor
            if (ssl_vendor != null)
                environment.put(com.tibco.tibjms.TibjmsSSL.VENDOR, ssl_vendor);

            // set trace for client-side operations, loading of certificates
            // and other
            if (ssl_trace)
                environment.put(com.tibco.tibjms.TibjmsSSL.TRACE, new Boolean(true));

            // set vendor trace. Has no effect for "j2se", "entrust6" uses
            // this to trace SSL handshake
            if (ssl_debug_trace)
                environment.put(com.tibco.tibjms.TibjmsSSL.DEBUG_TRACE, new Boolean(true));

            // set trusted certificates if specified
            if (ssl_trusted.size() != 0)
                environment.put(com.tibco.tibjms.TibjmsSSL.TRUSTED_CERTIFICATES, ssl_trusted);

            // set expected host name in the certificate if specified
            if (ssl_hostname != null)
                environment.put(com.tibco.tibjms.TibjmsSSL.EXPECTED_HOST_NAME, ssl_hostname);

            //
            if (ssl_ciphers != null)
                environment.put(com.tibco.tibjms.TibjmsSSL.CIPHER_SUITES, ssl_ciphers);

            if (disable_verify_host_name)
              environment.put(com.tibco.tibjms.TibjmsSSL.ENABLE_VERIFY_HOST_NAME, new Boolean(false));

            if (disable_verify_host || ssl_trusted.size()==0)
               environment.put(com.tibco.tibjms.TibjmsSSL.ENABLE_VERIFY_HOST, new Boolean(false));

            // set client identity if specified. ssl_key may be null
            // if identity is PKCS12, JKS or EPF. 'j2se' only supports
            // PKCS12 and JKS. 'entrust6' also supports PEM/PKCS8 combination.
            if (ssl_identity != null)
            {
                if (ssl_password == null)
                {
                    System.err.println("Error: must specify -ssl_password if identity is set");
                    System.exit(-1);
                }
                environment.put(com.tibco.tibjms.TibjmsSSL.IDENTITY, ssl_identity);
                environment.put(com.tibco.tibjms.TibjmsSSL.PASSWORD, ssl_password);

                if (ssl_key != null)
                   environment.put(com.tibco.tibjms.TibjmsSSL.PRIVATE_KEY, ssl_key);
            }
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

            ConnectionFactory factory = new com.tibco.tibjms.TibjmsConnectionFactory(serverUrl,null,environment);

            Connection connection = factory.createConnection(userName,password);

            Session session = connection.createSession();

            javax.jms.Topic topic = session.createTopic(topicName);

            MessageProducer publisher  = session.createProducer(topic);
            MessageConsumer subscriber = session.createConsumer(topic);

            connection.start();

            javax.jms.MapMessage message = session.createMapMessage();
            message.setStringProperty("field","SSL message");

            for (int i=0; i<3; i++)
            {
                publisher.send(message);
                System.err.println("\nPublished message: "+message);

                /* read same message back */

                message = (javax.jms.MapMessage)subscriber.receive();
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
    }

    public static void main(String args[])
    {
        tibjmsSSL t = new tibjmsSSL(args);
    }

    void usage()
    {
        System.err.println("\nUsage: java tibjmsSSL [options]");
        System.err.println("");
        System.err.println("   where options are:");
        System.err.println("");
        System.err.println(" -server        <server URL>   - EMS server URL, default is ssl://localhost:7243");
        System.err.println(" -user          <user name>    - user name, default is null");
        System.err.println(" -password      <password>     - password, default is null");
        System.err.println(" -topic         <topic-name>   - topic name, default is \"topic.sample\"");
        System.err.println("");
        System.err.println("  SSL options:");
        System.err.println("");
        System.err.println(" -ssl_vendor       <name>          - SSL vendor: 'j2se' or 'entrust6',");
        System.err.println("                                     default is 'j2se'");
        System.err.println(" -ssl_trace                        - trace SSL initialization");
        System.err.println(" -ssl_debug_trace                  - trace SSL handshake and related");
        System.err.println(" -ssl_trusted      <file-name>     - file with trusted certificate(s),");
        System.err.println("                                     this parameter may repeat if more");
        System.err.println("                                     than one file required");
        System.err.println(" -ssl_hostname     <host-name>     - name expected in the server certificate");
        System.err.println(" -ssl_identity     <file-name>     - client identity file");
        System.err.println(" -ssl_key          <file-name>     - client key file (optional)");
        System.err.println(" -ssl_password     <string>        - password to decrypt client identity");
        System.err.println("                                     or key file");
        System.err.println(" -no_verify_host_name              - disable host name verification");
        System.err.println(" -no_verify_host                   - disable host verification");
        System.err.println(" -ssl_ciphers      <suite-name(s)> - cipher suite names, colon separated");
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
            if (args[i].compareTo("-ssl_hostname")==0)
            {
                if ((i+1) >= args.length) usage();
                ssl_hostname = args[i+1];
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
            if (args[i].compareTo("-ssl_ciphers")==0)
            {
                if ((i+1) >= args.length) usage();
                ssl_ciphers = args[i+1];
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


