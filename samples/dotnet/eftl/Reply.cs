/*
 * Copyright (c) 2013-$Date: 2015-07-31 16:05:02 -0700 (Fri, 31 Jul 2015) $ TIBCO Software Inc.
 * All Rights Reserved.
 * For more information, please contact:
 * TIBCO Software Inc., Palo Alto, California, USA
 */

using System;
using System.Threading;
using System.Collections;

using TIBCO.EFTL;

public class Reply
{
    static String url = "ws://localhost:8585/channel";
    static String username = "user";
    static String password = "password";

    public class RequestListener: ISubscriptionListener
    {
        IConnection connection;

        public RequestListener(IConnection connection)
        {
            this.connection = connection;
        }

        public void OnMessages(IMessage[] requests)
        {
            foreach (IMessage request in requests)
            {
                Console.WriteLine("Request message " + request);

                try
                {
                    // Create reply message.
                    IMessage reply = connection.CreateMessage();
                    reply.SetString("text", "This is a sample reply message");

                    // Asynchronously send the reply.
                    connection.SendReply(reply, request, null);
                } 
                catch (Exception e)
                {
                    Console.WriteLine(e.ToString());
                }
            }

            // Disconnect from the server and exit.
            connection.Disconnect();
        }
 
        public void OnSubscribe(String subscriptionId)
        {
            Console.WriteLine("Subscribed to request messages");
        }
   
        public void OnError(String subscriptionId, int code, String reason) 
        {
            Console.WriteLine("Subscription error: " + reason);

            // Disconnect from the server and exit.
            connection.Disconnect();
        }
    } 

    public class ConnectionListener: IConnectionListener
    {
        public void OnConnect(IConnection connection)
        {
            Console.WriteLine("Connected to server at " + url);

            // Asynchronously subscribe to request messages.
            connection.Subscribe("{\"type\":\"request\"}", null, null, new RequestListener(connection));
        }

        public void OnDisconnect(IConnection connection, int code, String reason)
        {
            Console.WriteLine("Disconnected from server: " + reason);
        }

        public void OnError(IConnection connection, int code, String reason)
        {
            Console.WriteLine("Connection error " + reason);
        }

        public void OnReconnect(IConnection connection)
        {
            Console.WriteLine("Reconnected to server at " + url);
        }
    } 

    public static void Main (string []args) 
    {
        if (args.Length > 0)
        {
            url = args[0];
        }

        Console.Write("#\n# Request\n#\n# {0}\n#\n", EFTL.GetVersion());

        Hashtable options = new Hashtable();

        if (username != null)
            options.Add(EFTL.PROPERTY_USERNAME, username);

        if (password != null)
            options.Add(EFTL.PROPERTY_PASSWORD, password);

        try
        {
            // Asynchronously connect to the server.
            EFTL.Connect(url, options, new ConnectionListener());
        } 
        catch (Exception e)
        {
            Console.WriteLine(e.ToString());
        }
    }
}
