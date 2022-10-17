/* 
 * Copyright (c) 2001-$Date: 2016-12-13 17:00:37 -0600 (Tue, 13 Dec 2016) $ TIBCO Software Inc. 
 * Licensed under a BSD-style license. Refer to [LICENSE]
 * For more information, please contact:
 * TIBCO Software Inc., Palo Alto, California, USA
 * 
 * $Id: csSelector.cs 90180 2016-12-13 23:00:37Z olivierh $
 * 
 */

/// <summary>
/// This sample demonstrates the use of Selectors.
///
/// This program creates a queue sender which sends a few messages
/// into a queue, each message has an integer property set to the
/// sequential number of the message. Three asynchronous queue
/// receivers are created each with selectors such that each
/// message satisfies only a single receiver's selector.
///
/// This sample supports the '-noselector' option which allows to
/// disable use of selectors and see what difference does it make.
///
/// Usage:  csSelector [options]
///
///    where options are:
///
///    -server    <server-url>  Server URL.
///                             If not specified this sample assumes a
///                             serverUrl of "tcp://localhost:7222"
///    -user      <user-name>   User name. Default is null.
///    -password  <password>    User password. Default is null.
///    -queue     <queue-name>  Queue name. Default is "queue.sample.selector".
///    -noselector              Use it to see the difference when run 
///                             with and without selectors.
///
/// </summary>

using System;
using System.Threading;
using TIBCO.EMS;

public class csSelector 
{
    String serverUrl = null;
    String userName = null;
    String password = null;

    String queueName = "queue.sample.selector";
    bool noselector = false;

    class MyMessageListener : IMessageListener 
    {
        private int receiverNumber;

        public MyMessageListener(int receiverNumber) {

            this.receiverNumber = receiverNumber;
        }

        public void OnMessage(Message message) {
            try {
                Console.WriteLine("Receiver " + receiverNumber +
                                  " received message " + message.GetIntProperty("count"));
            } catch (EMSException e) {
                Console.Error.WriteLine("Exception in csSelector: " + e.Message);
                Console.Error.WriteLine(e.StackTrace);
                Environment.Exit(0);
            }
        }
    }

    public csSelector(String[] args) 
    {
        ParseArgs(args);

        try {
            tibemsUtilities.initSSLParams(serverUrl,args);
        }
        catch (Exception e)
        {
            System.Console.WriteLine("Exception: "+e.Message);
            System.Console.WriteLine(e.StackTrace);
            System.Environment.Exit(-1);
        }

        Console.WriteLine("\n------------------------------------------------------------------------");
        Console.WriteLine("csSelector SAMPLE");
        Console.WriteLine("------------------------------------------------------------------------");
        Console.WriteLine("Server....................... " + ((serverUrl != null)?serverUrl:"localhost"));
        Console.WriteLine("User......................... " + ((userName != null)?userName:"(null)"));
        Console.WriteLine("Queue........................ " + queueName);
        Console.WriteLine("------------------------------------------------------------------------\n");


        if (!noselector) {
            Console.WriteLine("\n*** Also try to run this sample with the -noselector");
            Console.WriteLine("*** option to see the difference it makes.");
        }

        try {
            ConnectionFactory factory = new TIBCO.EMS.ConnectionFactory(serverUrl);

            Connection connection = factory.CreateConnection(userName, password);

            Session receive_session = connection.CreateSession(false, Session.AUTO_ACKNOWLEDGE);
            Session send_session = connection.CreateSession(false, Session.AUTO_ACKNOWLEDGE);

            Queue queue = send_session.CreateQueue(queueName);

            // Start the connection so we can drain the queue and then proceed.
            connection.Start();

            // drain the queue
            MessageConsumer receiver = receive_session.CreateConsumer(queue);

            int drain_count = 0;
            Console.WriteLine("\nDraining the queue " + queueName);

            // read queue until empty
            while (receiver.Receive(1000) != null) {
                drain_count++;
            }
            Console.WriteLine("Drained " + drain_count + " messages from the queue");

            // close receiver to prevent any queue messages to be delivered
            receiver.Close();

            // create receivers with selectors
            Console.WriteLine("");
            if (!noselector)
                Console.WriteLine("Creating receivers with selectors:\n");
            else
                Console.WriteLine("Creating receivers without selectors:\n");
            Thread.Sleep(500);

            int receiver_count = 3;
            for (int i = 0; i < receiver_count; i++) {
                String selector = null;

                if (!noselector)
                    selector = "count >= " + (i * 4) + " AND count < " + (i * 4 + 4);

                receiver = receive_session.CreateConsumer(queue, selector);

                if (!noselector)
                    Console.WriteLine("Created receiver " + i + " with selector: \"" + selector + "\"");
                else
                    Console.WriteLine("Created receiver " + i);

                receiver.MessageListener = new MyMessageListener(i);
                Thread.Sleep(500);
            }

            // create sender
            MessageProducer sender = send_session.CreateProducer(queue);

            Message message = null;

            int message_number = 0;

            // send 12 messages into queue
            Console.WriteLine("");
            Console.WriteLine("Sending 12 messages into the queue:\n");

            Thread.Sleep(200);

            for (int i = 0; i < 12; i++) {
                message = send_session.CreateMessage();
                message.SetIntProperty("count", message_number);
                sender.Send(message);
                Thread.Sleep(500);
                message_number++;
            }

            // wait for some time while all messages received
            Thread.Sleep(1000);

            connection.Close();
        } catch (EMSException e) {
            Console.Error.WriteLine("Exception in csSelector: " + e.Message);
            Console.Error.WriteLine(e.StackTrace);
            Environment.Exit(0);
        } catch (ThreadInterruptedException e) {
            Console.Error.WriteLine("Exception in csSelector: " + e.Message);
            Console.Error.WriteLine(e.StackTrace);
            Environment.Exit(0);
        }

    }

    public static void Main(String[] args) 
    {
        csSelector t = new csSelector(args);
    }

    void Usage() 
    {
        Console.WriteLine("\nUsage: csSelector [options]");
        Console.WriteLine("");
        Console.WriteLine("   where options are:");
        Console.WriteLine("");
        Console.WriteLine("   -server     <server URL> - Server URL, default is local server");
        Console.WriteLine("   -user       <user name>  - user name, default is null");
        Console.WriteLine("   -password   <password>   - password, default is null");
        Console.WriteLine("   -queue      <queue name> - queue name, default is \"queue.sample.selector\"");
        Console.WriteLine("   -noselector              - do not use selectors");
        Console.WriteLine("   -help-ssl                - help on ssl parameters");
        Environment.Exit(0);
    }

    void ParseArgs(String[] args) 
    {
        int i = 0;

        while (i < args.Length) 
        {
            if (args[i].CompareTo("-server") == 0) 
            {
                if ((i + 1) >= args.Length)
                    Usage();
                serverUrl = args[i+1];
                i += 2;
            }
            else
            if (args[i].CompareTo("-queue") == 0) 
            {
                if ((i + 1) >= args.Length)
                    Usage();
                queueName = args[i+1];
                i += 2;
            }
            else 
            if (args[i].CompareTo("-noselector") == 0) 
            {
                noselector = true;
                i += 1;
            }
            else 
            if (args[i].CompareTo("-user") == 0) 
            {
                if ((i + 1) >= args.Length)
                    Usage();
                userName = args[i+1];
                i += 2;
            } 
            else 
            if (args[i].CompareTo("-password") == 0) 
            {
                if ((i + 1) >= args.Length)
                    Usage();
                password = args[i+1];
                i += 2;
            }
            else 
            if (args[i].CompareTo("-help") == 0) 
            {
                Usage();
            }
            else 
            if (args[i].CompareTo("-help-ssl")==0)
            {
                tibemsUtilities.sslUsage();
            }
            else
            if(args[i].StartsWith("-ssl"))
            {
                i += 2;
            }         
            else 
            {
                Console.Error.WriteLine("Unrecognized parameter: " + args[i]);
                Usage();
            }
        }
    }
}
