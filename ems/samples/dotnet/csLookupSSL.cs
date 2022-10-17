/* 
 * Copyright (c) 2001-$Date: 2016-12-13 17:00:37 -0600 (Tue, 13 Dec 2016) $ TIBCO Software Inc. 
 * Licensed under a BSD-style license. Refer to [LICENSE]
 * For more information, please contact:
 * TIBCO Software Inc., Palo Alto, California, USA
 * 
 * $Id: csLookupSSL.cs 90180 2016-12-13 23:00:37Z olivierh $
 * 
 */


/// <summary>
/// TIBCO EMS provides the ability to lookup those administered objects
/// using the TIBCO EMS server as a naming service provider.
/// Example of how to lookup administered objects from a TIBCO EMS
/// client via SSL connection to the tibjmsnaming server.
///
/// This sample looks up an SSL connection factory, a topic and queue.
///
/// This sample assumes the use of the original sample configuration
/// files distributed with TIBCO EMS software. If the configuration
/// has been altered, this sample may not work as expected.
/// 
/// NOTE: This sample will work only if the user specifies a SSL URL.
/// specifying a TCP URL will not work with this sample.
///
/// There are three types of administered objects:
///   - ConnectionFactory
///   - Topic
///   - Queue
///
///
/// 
/// Usage:  csLookupSSL [-provider <providerUrl>] -ssl_identity <identity> 
///                     -ssl_password <password> -ssl_target_hostname <hostname>
///
///    where options are:
///
///    -provider   Provider URL used for looking up administered
///                objects.
///                If not specified this sample assumes a providerUrl
///                of "tibjmsnaming://localhost:7243".
///                e.g. -provider tibjmsnaming://localhost:7243 or
///                e.g. -provider ssl://localhost:7243
///
///    -ssl_identity The identity of the client
///                  e.g. -ssl_identity client_identity.p12
///
///    -ssl_password The password for the ssl_identity.
///                  e.g. -ssl_password password
/// 
///    -ssl_target_hostname This is actually the CN of the server certificate.
///                         Normally the hostname is set as the CN of the server
///                         certificate.
///                         e.g. -ssl_target_hostname server 
/// </summary>

using System;
using System.Collections;
using TIBCO.EMS;

public class csLookupSSL
{
    const String defaultProviderURL = "tibjmsnaming://localhost:7243";
    
    String providerUrl = null;
    String userName = null;
    String password = null;
    String ssl_identity = null;
    String ssl_password = null;
    String ssl_target_hostname = null;

    public csLookupSSL(String[] args) 
    {
        parseArgs(args);
        if (providerUrl == null)
            providerUrl = defaultProviderURL;
        
        // Print parameters
        Console.WriteLine("\n------------------------------------------------------------------------");
        Console.WriteLine("csLookupSSL SAMPLE");
        Console.WriteLine("------------------------------------------------------------------------");
        Console.WriteLine("Server....................... " + providerUrl);
        Console.WriteLine("------------------------------------------------------------------------\n");
        
        try
        {
            Hashtable env = new Hashtable();
            
            env.Add(LookupContext.PROVIDER_URL, providerUrl);
            if (userName != null)
            {
                env.Add(LookupContext.SECURITY_PRINCIPAL, userName);
                if (password != null)
                    env.Add(LookupContext.SECURITY_CREDENTIALS, password);
            }
            
            // Creating the SSL related file store info object.
            EMSSSLFileStoreInfo info = new EMSSSLFileStoreInfo();
            if (ssl_identity != null) 
                info.SetSSLClientIdentity(ssl_identity);
            
            if (ssl_password != null) 
            {
                string _sslpassword = ssl_password;
                info.SetSSLPassword(_sslpassword.ToCharArray());
            }

            if (ssl_target_hostname == null) 
            {
                Console.WriteLine("ssl_target_hostname is required parameter");
                usage();
            }
            
            // Setting up the SSL properties for the lookup context.
            env.Add(LookupContext.SSL_STORE_INFO, info);
            env.Add(LookupContext.SSL_STORE_TYPE, EMSSSLStoreType.EMSSSL_STORE_TYPE_FILE);
            env.Add(LookupContext.SECURITY_PROTOCOL, "ssl");
            env.Add(LookupContext.SSL_TARGET_HOST_NAME, ssl_target_hostname);
            
            LookupContext lcxt = new LookupContext(env);
            
            // Lookup connection factory which must exist in the factories config file.

            ConnectionFactory factory    = null;
            ConnectionFactory sslFactory = null;

            try 
            {
                factory = (ConnectionFactory) lcxt.Lookup("ConnectionFactory");
                Console.WriteLine("OK - successfully did lookup ConnectionFactory");
                
                // Lookup SSL connection factory which must exist in the
                // factories config file.
                sslFactory = (ConnectionFactory) lcxt.Lookup("SSLConnectionFactory");
                Console.WriteLine("OK - successfully did lookup SSLConnectionFactory");
                
                // Setting additional parameters for this SSL factory.
                sslFactory.SetTargetHostName(ssl_target_hostname);

                // Also, if users want to set any other SSL parameters,
                // i.e EMSSSLFileStoreInfo parameters, they can call GetCertificateStore
                // on the connection factory object.
                EMSSSLFileStoreInfo fileStoreInfo = (EMSSSLFileStoreInfo)sslFactory.GetCertificateStore();
                fileStoreInfo.SetSSLClientIdentity(ssl_identity);
                string _sslpassword = ssl_password;
                fileStoreInfo.SetSSLPassword(_sslpassword.ToCharArray());

            }
            catch (EMSException e)
            {
                Console.Error.WriteLine("**EMSException occurred while doing a lookup");
                Console.Error.WriteLine(e.Message);
                Console.Error.WriteLine(e.StackTrace);
                Environment.Exit(0); // Can't lookup anything anyway
            }

            // Let's create a connection to the server and a session to verify
            // that the server is running.
            Connection sslConnection = null;
            Session    sslSession    = null;
            
            try
            {
                sslConnection = sslFactory.CreateConnection(userName, password);
                sslSession    = sslConnection.CreateSession(false, Session.AUTO_ACKNOWLEDGE);
                sslConnection.Close();
            }
            catch (EMSException e)
            {
                Console.Error.WriteLine("**EMSException occurred while creating SSL connection");
                Console.Error.WriteLine("  Most likely the server is not running, the exception trace follows:");
                Console.Error.WriteLine(e.StackTrace);
                Environment.Exit(0); // Can't lookup anything anyway
            }

            // Let's create a connection to the server and a session to verify
            // that the server is running so we can continue with our lookup operations.
            Connection connection = null;
            Session    session    = null;
            
            try
            {
                connection = factory.CreateConnection(userName, password);
                session    = connection.CreateSession(false, Session.AUTO_ACKNOWLEDGE);
            }
            catch (EMSException e)
            {
                Console.Error.WriteLine("**EMSException occurred while creating connection");
                Console.Error.WriteLine("  Most likely the server is not running, the exception trace follows:");
                Console.Error.WriteLine(e.StackTrace);
                Environment.Exit(0); // Can't lookup anything anyway
            }
            
            // Lookup topic 'topic.sample' which must exist in the topics
            // configuration file; if not successfull then most likely such
            // topic does not exist in the config file.
            Topic sampleTopic = null;
            try
            {
                sampleTopic = (Topic) lcxt.Lookup("topic.sample");
                Console.WriteLine("OK - successfully did lookup topic 'topic.sample'");
            }
            catch (NamingException ne)
            {
                Console.Error.WriteLine(ne.StackTrace);
                Console.Error.WriteLine("**NamingException occurred while lookup the topic topic.sample");
                Console.Error.WriteLine("  Most likely such topic does not exist in your configuration.");
                Console.Error.WriteLine("  Exception message follows:");
                Console.Error.WriteLine(ne.Message);
                Environment.Exit(0);
            }
            
            // Lookup queue 'queue.sample' which must exist in the queues
            // configuration file; if not successfull then it does not exist.
            TIBCO.EMS.Queue sampleQueue = null;
            try
            {
                sampleQueue = (TIBCO.EMS.Queue) lcxt.Lookup("queue.sample");
                Console.WriteLine("OK - successfully did lookup queue 'queue.sample'");
            }
            catch (NamingException ne)
            {
                Console.Error.WriteLine("**NamingException occurred while lookup the queue queue.sample");
                Console.Error.WriteLine("  Most likely such topic does not exist in your configuration.");
                Console.Error.WriteLine("  Exception message follows:");
                Console.Error.WriteLine(ne.Message);
                Environment.Exit(0);
            }
            
            // Try to lookup a topic when a topic *and* a queue with the
            // same name exist in the configuration. The sample configuration
            // has both topic and queue with the name 'sample' used here. If
            // everything is OK, we will get an exception, then try to lookup
            // those objects using fully-qualified names.
            try
            {
                Object object_Renamed = lcxt.Lookup("sample");
                
                // If the sample config was not altered, we should not get to this
                // line because lookup() should throw an exception.
                Console.WriteLine("Object named 'sample' has been looked up as: " + object_Renamed);
            }
            catch (NamingException ne)
            {
                Console.Error.WriteLine("OK - NamingException occurred while lookup the object named 'sample'");
                Console.Error.WriteLine("     This is the CORRECT BEHAVIOUR if the exception message below");
                Console.Error.WriteLine("     specifies name conflict situation. Exception message follows:");
                Console.Error.WriteLine("     " + ne.Message);
                
                // Let's look them up using name qualifiers.
                try
                {
                    Topic t = (Topic) lcxt.Lookup("$topics.sample");
                    Console.WriteLine("OK - successfully did lookup topic '$topics.sample'");
                    TIBCO.EMS.Queue q = (TIBCO.EMS.Queue) lcxt.Lookup("$queues.sample");
                    Console.WriteLine("OK - successfully did lookup queue '$queues.sample'");
                }
                catch (NamingException nex)
                {
                    // These may not be configured: Print a warning and continue.
                    Console.Error.WriteLine("**NamingException occurred while lookup the topic or queue 'sample'");
                    Console.Error.WriteLine("  Exception message follows:");
                    Console.Error.WriteLine("  " + nex.Message);
                }
            }
            
            // Now let's try to lookup a topic which definitely does not
            // exist. A lookup() operation will throw an exception and then we
            // will try to create such a topic explicitly with the createTopic()
            // method.
            String dynamicName = "topic.does.not.exist." + (DateTime.Now.Ticks - 621355968000000000) / 10000;
            Topic dynamicTopic = null;
            try
            {
                dynamicTopic = (Topic) lcxt.Lookup(dynamicName);
                
                // If the sample config was not altered, we should not get to
                // this line because lookup() should throw an exception.
                Console.Error.WriteLine("**Error: dynamic topic " + dynamicName + " found by lookup");
            }
            catch (NamingException)
            {
                // This is OK.
                Console.Error.WriteLine("OK - could not lookup dynamic topic " + dynamicName);
            }
            
            // Try to create it.
            try
            {
                dynamicTopic = session.CreateTopic(dynamicName);
                Console.WriteLine("OK - successfully created dynamic topic " + dynamicName);
            }
            catch (EMSException e)
            {
                // A few reasons are possible here. Maybe there is no > entry 
                // in the topics configuration or the server security is
                // turned on. If the original sample configuration is used, this
                // should work.
                Console.Error.WriteLine("**EMSException occurred while creating topic " + dynamicName);
                Console.Error.WriteLine("  Please verify your topics configuration and security settings.");
                Console.Error.WriteLine(e.StackTrace);
                Environment.Exit(0);
            }
            
            connection.Close();
            
            Console.WriteLine("OK - Done.");
        }
        catch (EMSException e)
        {
            Console.Error.WriteLine(e.StackTrace);
            Environment.Exit(0);
        }
    }
    
    public static void Main(String[] args) 
    {
        csLookupSSL t = new csLookupSSL(args);
    }
    
    void usage() 
    {
        Console.WriteLine("\nUsage: csLookupSSL [options]");
        Console.WriteLine("");
        Console.WriteLine("  where options are:");
        Console.WriteLine("");
        Console.WriteLine("  -provider  <provider URL> - Naming service provider URL, default is local server");
        Console.WriteLine("  -user      <user name>    - user name, default is null");
        Console.WriteLine("  -password  <password>     - password, default is null");
        Console.WriteLine("  -ssl_identity <identity>  - to be used to create a SSL connection to the EMS server");
        Console.WriteLine("  -ssl_password <password>  - password for the ssl_identity");
        Console.WriteLine("  -ssl_target_hostname <h>  - [REQUIRED] This is actually the CN of the server certificate. Normally, \n" +
                          "                              the target hostname is used as the CN of the server certificate");

        Environment.Exit(0);
    }
    
    void parseArgs(String[] args) 
    {
        int i = 0;
        
        while (i < args.Length)
        {
            if (args[i].CompareTo("-provider") == 0)
            {
                if ((i + 1) >= args.Length)
                    usage();
                providerUrl = args[i + 1];
                i += 2;
            }
            else if (args[i].CompareTo("-user") == 0)
            {
                if ((i + 1) >= args.Length)
                    usage();
                userName = args[i + 1];
                i += 2;
            }
            else if (args[i].CompareTo("-ssl_identity") == 0)
            {
                if ((i + 1) >= args.Length)
                    usage();
                ssl_identity = args[i + 1];
                i += 2;
            }
            else if (args[i].CompareTo("-ssl_password") == 0)
            {
                if ((i + 1) >= args.Length)
                    usage();
                ssl_password = args[i + 1];
                i += 2;
            }
            else if (args[i].CompareTo("-ssl_target_hostname") == 0)
            {
                if ((i + 1) >= args.Length)
                    usage();
                ssl_target_hostname = args[i + 1];
                i += 2;
            }
            else if (args[i].CompareTo("-password") == 0)
            {
                if ((i + 1) >= args.Length)
                    usage();
                password = args[i + 1];
                i += 2;
            }
            else if (args[i].CompareTo("-help") == 0)
            {
                usage();
            }
            else
            {
                Console.Error.WriteLine("Unrecognized parameter: " + args[i]);
                usage();
            }
        }
    }
}
