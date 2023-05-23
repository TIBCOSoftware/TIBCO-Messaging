/*
 * Copyright (c) 2013-$Date: 2015-07-30 12:57:32 -0700 (Thu, 30 Jul 2015) $ Cloud Software Group, Inc.
 * All Rights Reserved.
 */

using System;
using System.Threading;
using System.Collections;

using TIBCO.EFTL;

public class StopSubscriber 
{
    static IConnection connection = null;

    static String url = "ws://localhost:8585/channel";
    static String username = "user";
    static String password = "password";
    static String clientId = "sample-dotnet-client";
    static String durable = "sample-durable";
    static String matcher = null;
    static String subId = null;
    static int count = 10;
    static int received = 0;
    static bool clientAcknowledge = false;

    static CountdownEvent latch = new CountdownEvent(1);
    static Hashtable options = new Hashtable();

    private static void Usage() 
    {
        Console.WriteLine();
        Console.WriteLine("usage: StopSubscriber [options] url");
        Console.WriteLine();
        Console.WriteLine("options:");
        Console.WriteLine("  -u, --username <username>]");
        Console.WriteLine("  -p, --password <password>]");
        Console.WriteLine("  -i, --clientId <client id>");
        Console.WriteLine("  -n, --name <durable name>");
        Console.WriteLine("  -c, --count <count>");
        Console.WriteLine("      --clientAcknowledge");
        Console.WriteLine();
        System.Environment.Exit(1);
    }

    public class ConnectionListener: IConnectionListener
    {
        public void OnConnect(IConnection connection)
        {
            Console.WriteLine("Connected to eFTL server at: " + url);

            StopSubscriber.connection = connection;

            StopSubscriber.latch.Signal();
        }

        public void OnDisconnect(IConnection connection, int code, String reason)
        {
            Console.WriteLine("Disconnected from eFTL server: " + reason);

            StopSubscriber.latch.Signal();
        }

        public void OnError(IConnection connection, int code, String reason)
        {
            Console.WriteLine("Connection error: " + reason);

            StopSubscriber.latch.Signal();
        }

        public void OnReconnect(IConnection connection)
        {
            Console.WriteLine("Reconnected to eFTL server at: " + url);
        }
    } 

    public class SubscriptionListener: ISubscriptionListener
    {
        public void OnError(String subscriptionId, int code, String reason)
        {
            Console.WriteLine("Error subscribing to {0}: {1}", matcher, reason);
        }

        public void OnMessages(IMessage[] messages)
        {
            foreach (IMessage msg in messages)
            {
                received++;
                Console.WriteLine("Received message " + msg);

                if (clientAcknowledge)
                    connection.Acknowledge(msg);

                if (received == 5)
                {
                    Console.WriteLine("Stopping subscription");
                    try
                    {
                        connection.StopSubscription(subId);
                    }
                    catch (Exception e) 
                    {
                        Console.WriteLine(e.ToString());
                        return;
                    }
                    
                    Thread.Sleep(3000);
                    Console.WriteLine("Starting subscription");

                    try
                    {
                        connection.StartSubscription(subId);
                    }
                    catch (Exception e) 
                    {
                        Console.WriteLine(e.ToString());
                        return;
                    }
                }
            }

            if (received >= count)
                StopSubscriber.latch.Signal();
        }

        public void OnSubscribe(String subscriptionId)
        {
            subId = subscriptionId;
            Console.WriteLine("Subscribed to {0}", (matcher != null ? matcher : "null matcher"));
        }
    }

    public StopSubscriber(String url, String clientId, String username, String password) 
    {
        if (clientId != null) 
            options.Add(EFTL.PROPERTY_CLIENT_ID, clientId);

        if (username != null) 
            options.Add(EFTL.PROPERTY_USERNAME, username);

        if (password != null) 
            options.Add(EFTL.PROPERTY_PASSWORD, password);
        
        // Asynchronously connect to the eFTL server.
        EFTL.Connect(url, options, new ConnectionListener());

        try {
            latch.Wait();
        } catch (Exception) {}
    }

    public bool IsConnected()
    {
       return StopSubscriber.connection.IsConnected();
    }

    public void Close() 
    {
        // Asynchronously disconnect from the eFTL server.
        StopSubscriber.connection.Disconnect();
    }

    public void Subscribe(String durable, int count)
    {
        // Set the subscription properties.
        //
        // Configure the subscription for client acknowledgments if requested.
        //
        if (clientAcknowledge)
            options.Add(EFTL.PROPERTY_ACKNOWLEDGE_MODE, AcknowledgeMode.CLIENT);

        try {
            // Create a subscription matcher for messages containing a
            // field named "type" with a value of "hello".
            //
            // When connected to an FTL channel the content matcher
            // can be used to match any field in a published message.
            // Only matching messages will be received by the
            // subscription. The content matcher can match on string
            // and integer fields, or test for the presence or absence
            // of a field by setting it's value to the boolean true or
            // false.
            //
            // When connected to an EMS channel the content matcher
            // must only contain the destination field "_dest" set to
            // the EMS topic on which to subscribe.
            //
            // To match all messages use the empty matcher "{}".
            //
            // The durable name is used to create a durable subscription.
            // When setting a durable name also set the client identifier
            // when connecting as standard durable subscriptions are
            // mapped to a specific client identifier.
            //
            matcher = "{\"type\":\"hello\"}";
            
            // Asynchronously subscribe to matching messages.
            String subscriptionId = connection.Subscribe(matcher, durable, options, new SubscriptionListener());
            
            try {
                latch.Wait();
            } catch (Exception) {}

            // Unsubscribe, this will permanently remove a durable subscription.
            connection.Unsubscribe(subscriptionId);
        }
        catch (Exception e) 
        {
            Console.WriteLine(e.ToString());
        }
    }

    public static void Main (string []args) {

        for (int i = 0; i < args.Length; i++) {
            if (args[i].Equals("--username") || args[i].Equals("-u")) {
                if (i+1 < args.Length && !args[i+1].StartsWith("-")) {
                    username = args[++i];
                } else {
                    Usage();
                }
            } else if (args[i].Equals("--password") || args[i].Equals("-p")) {
                if (i+1 < args.Length && !args[i+1].StartsWith("-")) {
                    password = args[++i];
                } else {
                    Usage();
                }
            } else if (args[i].Equals("--clientId") || args[i].Equals("-i")) {
                if (i+1 < args.Length && !args[i+1].StartsWith("-")) {
                    clientId = args[++i];
                } else {
                    Usage();
                }
            } else if (args[i].Equals("--name") || args[i].Equals("-n")) {
                if (i+1 < args.Length && !args[i+1].StartsWith("-")) {
                    durable = args[++i];
                } else {
                    Usage();
                }
            } else if (args[i].Equals("--count") || args[i].Equals("-c")) {
                if (i+1 < args.Length && !args[i+1].StartsWith("-")) {
                    count = Int32.Parse(args[++i]);
                } else {
                    Usage();
                }
            } else if (args[i].Equals("--clientAcknowledge")) {
                clientAcknowledge = true;
            } else if (args[i].StartsWith("-")) {
                Usage();
            } else {
                url = args[i];
            }
        }

        Console.Write("#\n# StopSubscriber\n#\n# {0}\n#\n", EFTL.GetVersion());

        try {
            StopSubscriber client = new StopSubscriber(url, clientId, username, password);
            
            try {

                if (client.IsConnected())
                {
                    latch.Reset();
                    client.Subscribe(durable, count);            
                }
            }
            catch (Exception) {}
            finally {
                client.Close();
            }
        }
        catch (Exception e) 
        {
            Console.WriteLine(e.ToString());
        }
    }
}

