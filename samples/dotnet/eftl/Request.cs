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

public class Request
{
    static String url = "ws://localhost:8585/channel";
    static String username = "user";
    static String password = "password";

    public class RequestListener: IRequestListener
    {
        IConnection connection;

        public RequestListener(IConnection connection)
        {
            this.connection = connection;
        }

        public void OnReply(IMessage reply) 
        {
            Console.WriteLine("Reply message " + reply);

            // Disconnect from the server.
            connection.Disconnect();
        }
    
        public void OnError(IMessage message, int code, String reason) 
        {
            Console.WriteLine("Request error: " + reason);

            // Disconnect from the server.
            connection.Disconnect();
        }
    } 

    public class ConnectionListener: IConnectionListener
    {
        public void OnConnect(IConnection connection)
        {
            Console.WriteLine("Connected to server at " + url);

            try
            {
                // Create request message.
                IMessage request = connection.CreateMessage();
                request.SetString("type", "request");
                request.SetString("text", "This is a sample request message");

                // Asynchronously publish the request with a 10 second timeout.
                connection.SendRequest(request, 10.0, new RequestListener(connection));
            } 
            catch (Exception e)
            {
                Console.WriteLine(e.ToString());
            }
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
