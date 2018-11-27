/* 
 * Copyright (c) 2001-$Date: 2016-12-13 17:00:37 -0600 (Tue, 13 Dec 2016) $ TIBCO Software Inc. 
 * Licensed under a BSD-style license. Refer to [LICENSE]
 * For more information, please contact:
 * TIBCO Software Inc., Palo Alto, California, USA
 * 
 * $Id: csUFOLookup.cs 90180 2016-12-13 23:00:37Z olivierh $
 * 
 */


/// <summary>
/// Example of how to lookup an administered objects from a TIBCO EMS
/// client.
///
/// There are three types of administered objects:
///   - ConnectionFactory
///   - Topic
///   - Queue
///
/// TIBCO EMS provides the ability to lookup those administered objects
/// using the TIBCO EMS server as a naming service provider.
///
/// Note that TIBCO EMS' lookup interface only allows lookup of
/// static topics and queues. Static topics and queues are those
/// which have direct entries in the topics and queues configuration
/// files. On the contrary, dynamic topics and queues are created
/// by the applications using wildcard matching and the rules of
/// properties inheritance and do not have explicit entries in the
/// configuration files.
///
/// Also note that a name lookup of a topic or queue may fail if the
/// configuration contains a topic and a queue with the same name.
/// In this case, the application should use name qualifiers such as
/// $topics and $queues as to qualify as demonstrated by this sample.
///
/// This sample assumes the use of the original sample configuration
/// files distributed with TIBCO EMS software. If the configuration
/// has been altered, this sample may not work as expected.
///
///
/// Usage:  csUFOLookup [-provider <providerUrl>]
///
///    where options are:
///
///    -provider   Provider URL used for looking up administered
///                objects.
///                If not specified this sample assumes a providerUrl
///                of "tibjmsnaming://localhost:7222".
///
/// </summary>

using System;
using System.Collections;
using TIBCO.EMS.UFO;

public class csUFOLookup
{
    const String defaultProviderURL = "tibjmsnaming://localhost:7222";
    
    String providerUrl = null;
    String userName = null;
    String password = null;

    public csUFOLookup(String[] args)
    {
        parseArgs(args);
        if (providerUrl == null)
            providerUrl = defaultProviderURL;
            
        // Print parameters
        Console.WriteLine("\n------------------------------------------------------------------------");
        Console.WriteLine("csUFOLookup SAMPLE");
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
                
            LookupContext lcxt = new LookupContext(env);
                
            // Lookup connection factory which must exist in the factories
            // config file.
            ConnectionFactory factory = (ConnectionFactory) lcxt.Lookup("ConnectionFactory");
            Console.WriteLine("OK - successfully did lookup ConnectionFactory");
                
            // Let's create a connection to the server and a session to verify
            // that the server is running so we can continue with our lookup
            // operations.
            Connection connection = null;
            Session    session    = null;
                
            try
            {
                connection = factory.CreateConnection(userName, password);
                session    = connection.CreateSession(false, Session.AUTO_ACKNOWLEDGE);
            }
            catch (TIBCO.EMS.EMSException e)
            {
                Console.Error.WriteLine("**EMSException occurred while creating connection");
                Console.Error.WriteLine("  Most likely the server is not running, the exception trace follows:");
                Console.Error.WriteLine(e.StackTrace);
                Environment.Exit(0); // Can't lookup anything anyway
            }
                
            // Lookup topic 'topic.sample' which must exist in the topics
            // configuration file; if not successfull then most likely such
            // topic does not exist in the config files.
            Topic sampleTopic = null;
            try
            {
                sampleTopic = (Topic) lcxt.Lookup("topic.sample");
                Console.WriteLine("OK - successfully did lookup topic 'topic.sample'");
            }
            catch (TIBCO.EMS.NamingException ne)
            {
                Console.Error.WriteLine(ne.StackTrace);
                Console.Error.WriteLine("**NamingException occurred while looking up the topic topic.sample");
                Console.Error.WriteLine("  Most likely such topic does not exist in your configuration.");
                Console.Error.WriteLine("  Exception message follows:");
                Console.Error.WriteLine(ne.Message);
                Environment.Exit(0);
            }
                
            // Lookup queue 'queue.sample' which must exist in the queues
            // configuration file; if not successfull then it does not exist.
            TIBCO.EMS.UFO.Queue sampleQueue = null;
            try
            {
                sampleQueue = (TIBCO.EMS.UFO.Queue) lcxt.Lookup("queue.sample");
                Console.WriteLine("OK - successfully did lookup queue 'queue.sample'");
            }
            catch (TIBCO.EMS.NamingException ne)
            {
                Console.Error.WriteLine("**NamingException occurred while looking up the queue queue.sample");
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
            catch (TIBCO.EMS.NamingException ne)
            {
                Console.Error.WriteLine("OK - NamingException occurred while looking up the object named 'sample'");
                Console.Error.WriteLine("     This is the CORRECT BEHAVIOUR if the exception message below");
                Console.Error.WriteLine("     specifies name conflict situation. Exception message follows:");
                Console.Error.WriteLine("     " + ne.Message);
                    
                // Let's look them up using name qualifiers.
                try
                {
                    Topic t = (Topic) lcxt.Lookup("$topics.sample");
                    Console.WriteLine("OK - successfully did lookup topic '$topics.sample'");
                    TIBCO.EMS.UFO.Queue q = (TIBCO.EMS.UFO.Queue) lcxt.Lookup("$queues.sample");
                    Console.WriteLine("OK - successfully did lookup queue '$queues.sample'");
                }
                catch (TIBCO.EMS.NamingException nex)
                {
                    // These may not be configured: Print a warning and continue.
                    Console.Error.WriteLine("**NamingException occurred while looking up the topic or queue 'sample'");
                    Console.Error.WriteLine("  Exception message follows:");
                    Console.Error.WriteLine("  " + nex.Message);
                }
            }
                
            // Now let's try to lookup a topic which definitely does not
            // exist. A lookup() operation will throw an exception and then we
            // will try to create such topic explicitly with the createTopic()
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
            catch (TIBCO.EMS.NamingException)
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
            catch (TIBCO.EMS.EMSException e)
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
        catch (TIBCO.EMS.EMSException e)
        {
            Console.Error.WriteLine(e.StackTrace);
            Environment.Exit(0);
        }
    }
    
    public static void Main(String[] args)
    {
        csUFOLookup t = new csUFOLookup(args);
    }
    
    void usage()
    {
        Console.WriteLine("\nUsage: csUFOLookup [options]");
        Console.WriteLine("");
        Console.WriteLine("  where options are:");
        Console.WriteLine("");
        Console.WriteLine("  -provider  <provider URL> - Naming service provider URL, default is local server");
        Console.WriteLine("  -user      <user name>    - user name, default is null");
        Console.WriteLine("  -password  <password>     - password, default is null");
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

