/*
 * Copyright (c) 2013-$Date: 2015-07-31 16:05:02 -0700 (Fri, 31 Jul 2015) $ TIBCO Software Inc.
 * Licensed under a BSD-style license. Refer to [LICENSE]
 * For more information, please contact:
 * TIBCO Software Inc., Palo Alto, California, USA
 */

using System;
using System.Threading;
using System.Collections;

using TIBCO.EFTL;

public class Publisher
{
    public static IConnection connection = null;

    public static String url = "ws://localhost:9191/channel";
    public static String username = null;
    public static String password = null;
    public static String destination = "sample";
    public static int count = 10;
    public static int rate = 1;

    public static CountdownEvent latch = new CountdownEvent(1);
    public static Hashtable options = new Hashtable();
        
    private static void Usage() 
    {
        Console.WriteLine();
        Console.WriteLine("usage: Publisher [options] url");
        Console.WriteLine();
        Console.WriteLine("options:");
        Console.WriteLine("  -u, --username <username>");
        Console.WriteLine("  -p, --password <password>");
        Console.WriteLine("  -d, --destination <destination>");
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

    public void Publish(String destination, int count, int rate)
    {
        for (int i = 1; i <= count; i++)
        {
            IMessage msg = connection.CreateMessage();

            // Publish messages with a destination field.
            //
            // When connected to an EMS channel the destination field must
            // be present and set to the EMS topic on which to publish the 
            // message.
            //
            msg.SetString(MessageConstants.FIELD_NAME_DESTINATION, destination);

            // Populate additional fields
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
            } else if (args[i].Equals("--destination") || args[i].Equals("-d")) {
                if (i+1 < args.Length && !args[i+1].StartsWith("-")) {
                    destination = args[++i];
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
                    client.Publish(destination, count, rate);
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
