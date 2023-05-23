/*
 * Copyright (c) 2013-$Date: 2015-07-31 16:05:02 -0700 (Fri, 31 Jul 2015) $ Cloud Software Group, Inc.
 * All Rights Reserved.
 */

using System;
using System.Threading;
using System.Collections;

using TIBCO.EFTL;

public class Publisher
{
    static IConnection connection = null;

    static String url = "ws://localhost:8585/channel";
    static String username = "user";
    static String password = "password";
    static int count = 10;
    static int rate = 1;

    static CountdownEvent latch = new CountdownEvent(1);
    static Hashtable options = new Hashtable();
        
    private static void Usage() 
    {
        Console.WriteLine();
        Console.WriteLine("usage: Publisher [options] url");
        Console.WriteLine();
        Console.WriteLine("options:");
        Console.WriteLine("  -u, --username <username>");
        Console.WriteLine("  -p, --password <password>");
        Console.WriteLine("  -c, --count <count>");
        Console.WriteLine("  -r, --rate <messages per second>");
        Console.WriteLine();
        System.Environment.Exit(1);
    }

    public class ConnectionListener: IConnectionListener
    {
        public void OnConnect(IConnection connection)
        {
            Console.WriteLine("Connected to eFTL server at " + url);

            Publisher.connection = connection;
            Publisher.latch.Signal();
        }

        public void OnDisconnect(IConnection connection, int code, String reason)
        {
            Console.WriteLine("Disconnected from eFTL server: " + reason);
            Publisher.latch.Signal();
        }

        public void OnError(IConnection connection, int code, String reason)
        {
            Console.WriteLine("Connection error: " + reason);
            Publisher.latch.Signal();
        }

        public void OnReconnect(IConnection connection)
        {
            Console.WriteLine("Reconnected to eFTL server at " + url);
        }
    } 

    public class CompletionListener: ICompletionListener
    {
         public void OnCompletion(IMessage message) {
             Console.WriteLine("Published message " + message);
         }
    
         public void OnError(IMessage message, int code, String reason) {
             Console.WriteLine("Error publishing message: {0}\n", reason);
         }
    } 


    public Publisher(String url, String username, String password)
    {
        if (username != null)
            options.Add(EFTL.PROPERTY_USERNAME, username);

        if (password != null)
            options.Add(EFTL.PROPERTY_PASSWORD, password);
        
        // Asynchronously connect to the eFTL server.
        EFTL.Connect(url, options, new ConnectionListener());

        try {
            latch.Wait();
        }
        catch (Exception e) 
        {
            Console.WriteLine(e.Message);
        }
    }

    public bool IsConnected()
    {
        return connection.IsConnected();
    }

    public void Publish(int count, int rate)
    {
        for (int i = 1; i <= count; i++)
        {
            IMessage msg = connection.CreateMessage();

            // Publish "hello" messages. 
            //
            // When connected to an EMS channel the destination field must
            // be present and set to the EMS topic on which to publish the 
            // message.
            //
            msg.SetString("type", "hello");
            msg.SetString("text", "This is a sample eFTL message");
            msg.SetLong("long", (long) i);
            msg.SetDateTime("time", new DateTime());
            
            // Asynchronously publish the message.
            connection.Publish(msg, new CompletionListener());

            try
            {
                Thread.Sleep(1000/(rate > 0 ? rate : 1));
            } catch (Exception){}
        }
    }

    public void Close()
    {
        // Asynchronously disconnect from the eFTL server.
        connection.Disconnect();
    }

    public static void Main (string []args) 
    {
        for (int i = 0; i < args.Length; i++) 
        {
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
            } else if (args[i].Equals("--count") || args[i].Equals("-c")) {
                if (i+1 < args.Length && !args[i+1].StartsWith("-")) {
                    count = Int32.Parse(args[++i]);
                } else {
                    Usage();
                }
            } else if (args[i].Equals("--rate") || args[i].Equals("-r")) {
                if (i+1 < args.Length && !args[i+1].StartsWith("-")) {
                    rate = Int32.Parse(args[++i]);
                } else {
                    Usage();
                }
            } else if (args[i].StartsWith("-")) {
                Usage();
            } else {
                url = args[i];
            }
        }

        Console.Write("#\n# Publisher\n#\n# {0}\n#\n", EFTL.GetVersion());

        try
        {
            Publisher client = new Publisher(url, username, password);
    
            try {
                if (client.IsConnected()) {
                    client.Publish(count, rate);
                }
            } finally {
                client.Close();
            }
        } 
        catch (Exception e)
        {
            Console.WriteLine(e.ToString());
        }
    }
}
