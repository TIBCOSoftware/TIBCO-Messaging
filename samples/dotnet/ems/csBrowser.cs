/* 
 * Copyright (c) 2001-$Date: 2016-12-13 17:00:37 -0600 (Tue, 13 Dec 2016) $ TIBCO Software Inc. 
 * Licensed under a BSD-style license. Refer to [LICENSE]
 * For more information, please contact:
 * TIBCO Software Inc., Palo Alto, California, USA
 * 
 * $Id: csBrowser.cs 90180 2016-12-13 23:00:37Z olivierh $
 * 
 */

/// <summary>
/// This sample demonstrates the use of QueueBrowser. It uses the
/// generic connection factory, connection and session classes,
/// unlike csQueueBrowser, which uses the queue specific classes.
///
/// Notice that TIBCO EMS implements dynamic queue browsers.
/// This means that the QueueBrowser can dynamically receive new
/// messages added to the queue. If MoveNext() method of the
/// Enumerator returned by the QueueBrowser class returns false,
/// the application can wait and try to call it later. If the queue
/// being browsed has received new messages, the MoveNext() method
/// will return true and the application can browse new messages.
/// If MoveNext() returns false, the application can choose
/// to quit browsing or can wait for more messages to be delivered
/// into the queue.
///
/// After all queue messages have been delivered to the queue
/// browser, the messaging system waits for some time and then
/// tries to query for new messages. This happens behind the scene,
/// user application can try to call MoveNext() at any time,
/// the internal engine will only actually query the queue every fixed
/// interval. The messaging system queries the queue not more often
/// than every 5 seconds but the length of that interval is subject
/// to change without notice.
///
/// Usage:  csBrowser [options]
///
///    where options are:
///
///    -server    <server-url>  Server URL.
///                             If not specified this sample assumes a
///                             serverUrl of "tcp://localhost:7222"
///    -user      <user-name>   User name. Default is null.
///    -password  <password>    User password. Default is null.
///    -queue     <queue-name>  Queue name.  Default is "queue.sample.browser".
///
/// </summary>

using System;
using System.Collections;
using System.Threading;
using TIBCO.EMS;

public class csBrowser 
{
    string serverUrl = null;
    string userName = null;
    string password = null;
    string queueName = "queue.sample.browser";

    public csBrowser(string[] args) 
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
        Console.WriteLine("csBrowser SAMPLE");
        Console.WriteLine("------------------------------------------------------------------------");
        Console.WriteLine("Server....................... " + ((serverUrl != null)?serverUrl:"localhost"));
        Console.WriteLine("User......................... " + ((userName != null)?userName:"(null)"));
        Console.WriteLine("Queue........................ " + queueName);
        Console.WriteLine("------------------------------------------------------------------------\n");

        try {
            ConnectionFactory factory = new TIBCO.EMS.ConnectionFactory(serverUrl);

            Connection connection = factory.CreateConnection(userName, password);
            Session session = connection.CreateSession(false, Session.AUTO_ACKNOWLEDGE);
            TIBCO.EMS.Queue queue = session.CreateQueue(queueName);
            MessageProducer producer = session.CreateProducer(queue);
            Message message = null;

            connection.Start();

            // drain the queue
            MessageConsumer consumer = session.CreateConsumer(queue);

            int drain_count = 0;

            Console.WriteLine("Draining the queue " + queueName);

            // read queue until empty
            while (consumer.Receive(1000) != null) {
                drain_count++;
            }
            Console.WriteLine("Drained " + drain_count + " messages from the queue");

            // close consumer to prevent any queue messages from being delivered
            consumer.Close();

            int message_number = 0;

            // send 5 messages into queue
            Console.WriteLine("Sending 5 messages into queue.");
            for (int i = 0; i < 5; i++) {
                message_number++;
                message = session.CreateMessage();
                message.SetIntProperty("msg_num", message_number);
                producer.Send(message);
            }

            // create browser and browse what is there in the queue
            Console.WriteLine("--- Browsing the queue.");

            QueueBrowser browser = session.CreateBrowser(queue);

            IEnumerator msgs = browser.GetEnumerator();

            int browseCount = 0;

            while (msgs.MoveNext()) {
                message = (Message) msgs.Current;
                Console.WriteLine("Browsed message: number=" + message.GetIntProperty("msg_num"));
                browseCount++;
            }

            Console.WriteLine("--- No more messages in the queue.");

            // send 5 more messages into queue
            Console.WriteLine("Sending 5 more messages into queue.");
            for (int i = 0; i < 5; i++) {
                message_number++;
                message = session.CreateMessage();
                message.SetIntProperty("msg_num", message_number);
                producer.Send(message);
            }

            // try to browse again, if no success for some time then quit

            // notice that we will NOT receive new messages instantly. It
            // happens because QueueBrowser limits the frequency of query
            // requests sent into the queue after the queue was
            // empty. Internal engine only queries the queue every so many
            // seconds, so we'll likely have to wait here for some time.

            int attemptCount = 0;
            while (!msgs.MoveNext()) {
                attemptCount++;
                Console.WriteLine("Waiting for messages to arrive, count=" + attemptCount);
                Thread.Sleep(1000);
                if (attemptCount > 30) {
                    Console.WriteLine("Still no messages in the queue after " + attemptCount + " seconds");
                    Environment.Exit(0);
                }
            }

            // got more messages, continue browsing
            Console.WriteLine("Found more messages. Continue browsing.");
            do {
                message = (Message) msgs.Current;
                Console.WriteLine("Browsed message: number=" + message.GetIntProperty("msg_num"));
            }  while (msgs.MoveNext());

            // close all and quit
            browser.Close();

            connection.Close();
        } catch (EMSException e) {
            Console.Error.WriteLine("Exception in csBrowser: " + e.Message);
            Console.Error.WriteLine(e.StackTrace);
            Environment.Exit(0);
        } catch (ThreadInterruptedException e) {
            Console.Error.WriteLine("Exception in csBrowser: " + e.Message);
            Console.Error.WriteLine(e.StackTrace);
            Environment.Exit(0);
        }

    }

    public static void Main(string[] args) {

        csBrowser t = new csBrowser(args);
    }

    private void Usage() {
        Console.WriteLine("\nUsage: csBrowser [options]");
        Console.WriteLine("");
        Console.WriteLine("  where options are:");
        Console.WriteLine("");
        Console.WriteLine("  -server   <server-url>  - Server URL, default is local server");
        Console.WriteLine("  -user     <user-name>   - user name, default is null");
        Console.WriteLine("  -password <password>    - password, default is null");
        Console.WriteLine("  -queue    <queue-name>  - queue name, default is \"queue.sample.browser\"");
        Console.WriteLine("   -help-ssl              - help on ssl parameters");
        Environment.Exit(0);
    }

    private void  ParseArgs(string[] args) 
    {
        int i = 0;

        while (i < args.Length) {
            if (args[i].CompareTo("-server") == 0) 
            {
                if ((i + 1) >= args.Length)
                    Usage();
                serverUrl = args[i+1];
                i += 2;
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
            if (args[i].CompareTo("-queue") == 0) 
            {
                if ((i + 1) >= args.Length)
                    Usage();
                queueName = args[i+1];
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
