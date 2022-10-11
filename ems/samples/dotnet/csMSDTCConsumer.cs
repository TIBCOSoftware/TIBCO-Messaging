/* 
 * Copyright (c) 2001-$Date: 2016-12-13 17:00:37 -0600 (Tue, 13 Dec 2016) $ TIBCO Software Inc. 
 * Licensed under a BSD-style license. Refer to [LICENSE]
 * For more information, please contact:
 * TIBCO Software Inc., Palo Alto, California, USA
 * 
 * $Id: csMSDTCConsumer.cs 90180 2016-12-13 23:00:37Z olivierh $
 * 
 */

/// <summary>
/// This is a simple test of a client participating in an MSDTC txn.
/// This demonstrates a simple single phase commit involving a single resource
/// manager.
/// 
/// <para>
/// The client creates a EMSMSDTCConnection followed by a EMSDTCSession.
/// It then creates a consumer on this EMSDTCSession. when the first message
/// gets consumer by the consumer, the EMS .NET Client library will 
/// automatically detect the ambient transaction and for the session it 
/// enlists a resource manager with MSDTC. (The act of enlistment involves
/// handing a ResourceEnlistment Object to MSDTC).
/// 
/// When the Transaction is commited, MSDTC then calls into this
/// ResourceEnlistment object to complete the transaction.
///
/// NOTE: The entire process of enlistment with MSDTC is done by the
/// EMS .NET Client library and it's not transparent to the application.
/// </para>
///
/// <para>
///  Usage:  csMSDTCConsumer  [options]
/// 
///   where options are:
/// 
///    -server    <server-url>  Server URL.
///                             If not specified this sample assumes a
///                             serverUrl of "tcp://localhost:7222"
///    -user      <user-name>   User name. Default is null.
///    -password  <password>    User password. Default is null.
///    -queue     <queue-name>  Queue name. Default is "queue.sample"
///    -count     <numberof_msgs> Number of Msgs in a txn, default it 10
///    -numTxns   <numTxns>     Number of transactions, default is 10
///    -clientID  <clientID>    clientId, default is "client.cons.msdtc"
/// 
/// </para>
/// </summary>

using System;
using System.Threading;
using System.Transactions;
using TIBCO.EMS;

class csMSDTCConsumer
{
    string      serverUrl       = null;
    string      userName        = null;
    string      password        = null;

    string      queueName       = "queue.sample";
    string      clientID        = "client.cons.msdtc";
    int         count           = 10;
    int         numTxns         = 10;

    EMSDTCConnectionFactory factory     = null;
    EMSDTCConnection        connection  = null;
    EMSDTCSession           session     = null;
    MessageConsumer         msgConsumer = null;

    private void receiveMsgs(MessageConsumer cons)
    {
        // receive messages, messages received are now part of this
        // msdtc txn.
        for (int i = 0; i < count; i++)
        {
            Message msg = cons.Receive();
            if (msg != null) 
            {
                Console.WriteLine("### Received Msg: " + i);
                Console.WriteLine(msg.ToString());
            }
        }
    }

    public csMSDTCConsumer(string[] args) 
    {
        ParseArgs(args);

        try 
        {
            tibemsUtilities.initSSLParams(serverUrl,args);
        }
        catch (Exception e)
        {
            System.Console.WriteLine("Exception: "+e.Message);
            System.Console.WriteLine(e.StackTrace);
            System.Environment.Exit(-1);
        }

        Console.WriteLine("\n------------------------------------------------------------------------");
        Console.WriteLine("csMSDTConsumer");
        Console.WriteLine("------------------------------------------------------------------------");
        Console.WriteLine("Server....................... " + ((serverUrl != null)?serverUrl:"localhost"));
        Console.WriteLine("User......................... " + ((userName != null)?userName:"(null)"));
        Console.WriteLine("Queue........................ " + queueName);
        Console.WriteLine("Count........................ " + count);
        Console.WriteLine("Number of Txns............... " + numTxns);
        Console.WriteLine("Client ID.................... " + ((clientID != null)?clientID:"(null)"));
        Console.WriteLine("------------------------------------------------------------------------\n");
        
        try {
            factory = new TIBCO.EMS.EMSDTCConnectionFactory(serverUrl);
            // set the clientID before creating the connection, since clientID is required
            factory.SetClientID(clientID);

            connection = factory.CreateEMSDTCConnection(userName,password);
            session    = connection.CreateEMSDTCSession();

            // start the connection
            connection.Start();

            Console.WriteLine("Receive Msgs from queue: "+ queueName);

            // Use createQueue to create the queue
            Queue queue = session.CreateQueue(queueName);

            // create the consumer 
            msgConsumer = session.CreateConsumer(queue);

            for(int i=0; i<numTxns;)
            {
                try 
                {
                    // demonstrates the use of Transaction scope.
                    using (TransactionScope scope = new TransactionScope(TransactionScopeOption.RequiresNew))
                    {
                        // receive msgs
                        this.receiveMsgs(msgConsumer);
                        
                        // the application can do some other work on another resource
                        // manager as part of this msdtc txn. When the transaction is
                        // commited the DTC will coordinate with the resource manager's
                        // and run a two phase commit protocol (if there is more than
                        // one resource) and based on the outcome from other RM's 
                        // involved, commit or rollback the txn. 
                        // For example one can create another connection to another 
                        // EMS server another session and send msgs to another queue 
                        // on this new EMS server as part of the same MSDTC 
                        // distributed txn.
                        //
                        // In this example, since there is only one RM, it will result
                        // is single phase commit.
                        
                        // commit the transaction.
                        scope.Complete();
                    }
                    
                    i++;
                }
                catch (Exception e)
                {
                    Console.Error.WriteLine(e.StackTrace);
                    break;
                }
            }

            Console.WriteLine("Closing connection ..");
            // close the connection
            connection.Close();
        } 

        catch(EMSException e) 
        {
            Console.Error.WriteLine("Exception caught: " + e.Message);
            Console.Error.WriteLine(e.StackTrace);
            if (e.LinkedException != null)
                Console.Error.WriteLine(e.LinkedException);

            Environment.Exit(0);
        }
    }

    public static void Main(string[] args) 
    {
        csMSDTCConsumer t = new csMSDTCConsumer(args);
    }

    private void Usage() 
    {
        Console.WriteLine("\nUsage: csMSDTCConsumer [options]");
        Console.WriteLine("");
        Console.WriteLine("   where options are:");
        Console.WriteLine("");
        Console.WriteLine(" -server   <server URL> - Server URL, default is local server");
        Console.WriteLine(" -user     <user name>  - user name, default is null");
        Console.WriteLine(" -password <password>   - password, default is null");
        Console.WriteLine(" -queue    <queue-name> - queue name, default is \"queue.sample\"");
        Console.WriteLine(" -clientID <client-id>  - clientID, default is \"client.cons.msdtc\"");
        Console.WriteLine(" -count    <numbermsgs> - numberOfMsgs, default is 10");
        Console.WriteLine(" -numTxns  <numTxns>    - numberOfTransactions, default is 10");
        Console.WriteLine("                          default is \"subscriber\"");
        Console.WriteLine(" -help-ssl              - help on ssl parameters");
        Environment.Exit(0);
    }

    private void ParseArgs(string[] args) 
    {
        int i=0;

        while(i < args.Length) 
        {
            if (args[i].CompareTo("-server")==0) 
            {
                if ((i+1) >= args.Length) {
                    Usage();
                }
                serverUrl = args[i+1];
                i += 2;
            } 
            else 
            if (args[i].CompareTo("-queue")==0) 
            {
                if ((i+1) >= args.Length) {
                    Usage();
                }
                queueName = args[i+1];
                i += 2;
            }
            else 
            if (args[i].CompareTo("-user")==0) 
            {
                if ((i+1) >= args.Length) 
                {
                    Usage();
                }
                userName = args[i+1];
                i += 2;
            }
            else 
            if (args[i].CompareTo("-password")==0) 
            {
                if ((i+1) >= args.Length) {
                    Usage();
                }
                password = args[i+1];
                i += 2;
            }
            else 
            if (args[i].CompareTo("-count")==0) 
            {
                if ((i+1) >= args.Length) {
                    Usage();
                }
                String c = args[i+1];
                count = System.Convert.ToInt32(c);
                i += 2;
            }
            else 
            if (args[i].CompareTo("-numTxns")==0) 
            {
                if ((i+1) >= args.Length) {
                    Usage();
                }
                String c = args[i+1];
                numTxns = System.Convert.ToInt32(c);
                i += 2;
            }
            else 
            if (args[i].CompareTo("-clientID")==0) 
            {
                if ((i+1) >= args.Length) {
                    Usage();
                }
                clientID = args[i+1];
                i += 2;
            }
            else
            if (args[i].CompareTo("-help")==0) 
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
                Console.WriteLine("Unrecognized parameter: "+args[i]);
                Usage();
            }
        }
    }
}

