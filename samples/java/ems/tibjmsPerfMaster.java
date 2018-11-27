/* 
 * Copyright (c) 2005-$Date: 2016-12-13 17:00:37 -0600 (Tue, 13 Dec 2016) $ TIBCO Software Inc. 
 * Licensed under a BSD-style license. Refer to [LICENSE]
 * For more information, please contact:
 * TIBCO Software Inc., Palo Alto, California, USA
 * 
 * $Id: tibjmsPerfMaster.java 90180 2016-12-13 23:00:37Z olivierh $
 * 
 */

/*
 * The tibjmsPerfMaster class publishes instructions to the
 * tibjmsPerfSlave class and invokes the performance message
 * producer. At the end of the run the tibjmsPerfSlave class
 * sends the results back to the tibjmsPerfMaster class. The
 * tibjmsPerfSlave and tibjmsPerfMaster communicate over a
 * specified control topic. 
 *  
 * Usage:  java tibjmsPerfMaster [options]
 *
 *  where options are:
 *
 *   -server       <url>         EMS server URL. Default is
 *                               "tcp://localhost:7222".
 *   -user         <username>    User name. Default is null.
 *   -password     <password>    User password. Default is null.
 *   -control      <topic-name>  Control topic name over which
 *                               the slave and master communicate.
 *                               Default is "topic.control".
 *   -adminuser    <name>        Admin name. Default is "admin".
 *   -adminpwd     <password>    Admin password. Default is null
 *   -topic        <topic-name>  Topic name. Default is "topic.sample".
 *   -queue        <queue-name>  Queue name. No default.
 *   -size         <num bytes>   Message payload size in bytes. Default is 100.
 *   -count        <num msgs>    Number of messages to send. Default is 10k.
 *   -time         <seconds>     Number of seconds to run. Default is 0 (forever).
 *   -delivery     <mode>        Delivery mode. Default is NON_PERSISTENT.
 *                               Other values: PERSISTENT and RELIABLE.
 *   -prodthrds    <num threads> Number of message producer threads. Default is 1.
 *   -prodconns    <num conns>   Number of message producer connections. Default is 1.
 *   -prodtxnsize  <num msgs>    Number of nessages per producer transaction. Default is 0.
 *   -consthrds    <num threads> Number of message consumer threads. Default is 1.
 *   -consconns    <num conns>   Number of message consumer connections. Default is 1.
 *   -constxnsize  <num msgs>    Number of messages per consumer transaction. Default is 0.
 *   -durable      <name>        Durable subscription name.
 *   -selector     <selector>    Message selector for consumer threads. No default.
 *   -ackmode      <mode>        Message acknowledge mode. Default is AUTO.
 *                               Other values: DUPS_OK, CLIENT EXPLICIT_CLIENT,
 *                               EXPLICIT_CLIENT_DUPS_OK and NO.
 *   -rate         <msg/sec>     Message rate for producer threads.
 *   -flowcontrol  <num bytes>   EMS server flow control setting.
 *   -prefetch     <num msgs>    EMS server queue prefetch setting.
 *   -channel      <name>        Multicast channel name for topics.
 *   -payload      <file name>   File containing message payload.
 *   -output       <file name>   Output file. Default is "perf.txt".
 *   -factory      <lookup name> Lookname for connection factory.
 *   -storeName    <storeName>   Name of the store, if uniquestores is specified then
 *                               this will be the prefix and storenames generated based
 *                               on number of prodthrds, if uniquestores is not
 *                               specified then this is used as store property for 
 *                               destination/destinations.
 *   -failsafe                   Enable EMS server failsafe setting.
 *   -uniquedests                Each producer/consumer thread uses a unique destination.
 *   -uniquestores               Each destination has unique store
 *   -xa                         use xa transactions.
 *   -compression                Enable message compression.
 *   -C                          Use the C executable instead of the java class. 
 */

import java.util.*;
import javax.jms.*;
import java.io.*;

public class tibjmsPerfMaster
{
    /*
     * constants
     */
    private static final String C_EXECUTABLE = "tibemsMsgProducerPerf";

    /*
     * parameters
     */
    private String serverUrl = "tcp://localhost:7222";
    private String control = "topic.control";
    private String userName = null;
    private String password = null;
    private String adminName = "admin";
    private String adminPwd = null;
    private String topic = "topic.test";
    private String queue = null;
    private String count = "10000";
    private String runTime = null;
    private String msgSize = "100";
    private String delMode = "NON_PERSISTENT";
    private String prodThrds = null;
    private String prodConns = null;
    private String prodTxnSize = null;
    private String consThrds = null;
    private String consConns = null;
    private String consTxnSize = null;
    private String ackMode = "AUTO";
    private String durableName = null;
    private String selector = null;
    private String rate = null;
    private String payloadFile = null;
    private String outputName = "perf.txt";
    private String factoryName = null;
    private String channelName = null;
    private boolean uniqueDests = false;
    private boolean uniqueStores = false;
    private boolean xa           = false;
    private boolean compression = false;
    private boolean failsafe = false;
    private boolean useC = false;
    private int flowControl = 0;
    private int prefetch = 0;
    private String storeName = null;
    private boolean sendAsync = false;

    /*
     * variables
     */
    private tibjmsServerAdministrator serverAdmin;
    private Connection connection;
    private Session session;
    private Destination destination;
    private Destination replyDest;
    private MessageProducer msgProducer;
    private MessageConsumer msgConsumer;
    private long elapsedTime;
    private int sentCount;

    /**
     * Constructor 
     * 
     * @param args the command line arguments
     */
    public tibjmsPerfMaster(String[] args)
    {
        parseArgs(args);

        try 
        {
            checkConflictingArgs();
        } 
        catch (JMSException e) 
        {
            e.printStackTrace();
            System.exit(-1);
        }

        printStartBanner();

        try
        {
            initializeControls();

            administerServer();

            sendInstructions();

            runProducer();

            printPerformance();

            cleanup();
        }
        catch (JMSException e)
        {
            e.printStackTrace();
            System.exit(-1);
        }
    }

    /**
     * Print the start banner.
     */
    private void printStartBanner()
    {
        System.err.println();
        System.err.println("------------------------------------------------------------------------");
        System.err.println("tibjmsPerfMaster program");
        System.err.println("------------------------------------------------------------------------");
        System.err.println("Server....................... " + serverUrl);
        System.err.println("Admin........................ " + adminName);
        System.err.println("User......................... " + userName);
        System.err.println("Control Topic................ " + control);
        if (useC)
            System.err.println("Executable................... " + C_EXECUTABLE);
        System.err.println("------------------------------------------------------------------------");
        System.err.println();
    }

    /**
     * Initialize the control channel on which to publish
     * instructions to the slave control program.
     */
    private void initializeControls() throws JMSException
    {
        // lookup the connection factory
        ConnectionFactory factory = 
            new com.tibco.tibjms.TibjmsConnectionFactory(serverUrl);
        
        // create the connection
        connection = factory.createConnection(userName, password);
        
        // create the session
        session = connection.createSession();
        
        // create the control topic
        destination = session.createTopic(control);
        
        // create a message producer for the control topic
        msgProducer = session.createProducer(destination);
        
        // create a reply-to topic
        replyDest = session.createTemporaryTopic();
        
        // create a message consumer for the reply-to topic
        msgConsumer = session.createConsumer(replyDest);
        
        // start receiving messages
        connection.start();
    }

    /**
     * Configure the EMS server(s).
     */
    private void administerServer() throws JMSException
    {
        // connect to the EMS server(s) as an admin client
        serverAdmin = new tibjmsServerAdministrator(serverUrl,
                                                    adminName,
                                                    adminPwd); 

        // if flow control is not enabled then it won't be used
        if (flowControl > 0 && !serverAdmin.isFlowControlEnabled())
        {
            System.err.println("Warning: flow control is not enabled");
            flowControl = 0;
        }
        if (topic != null)
        {
            if (uniqueStores == false)
            {
                serverAdmin.createTopic(uniqueDests ? topic + ".>" : topic, 
                   failsafe, flowControl, channelName, storeName);
            }
            else
            {
                serverAdmin.createTopics(topic, flowControl, storeName,
                    uniqueStores, Integer.parseInt(prodThrds));
            }
        }
        if (queue != null)
        {
            // check if the queue is a routed queue
            String homeServer = null;
            int delim = queue.indexOf('@');
            if (delim != -1)
            {
                homeServer = queue.substring(delim + 1);
                queue = queue.substring(0, delim);
            }
            
            if (uniqueStores == false)
            {
                serverAdmin.createQueue(uniqueDests ? queue + ".>" : queue, homeServer, 
                   failsafe, flowControl, prefetch, storeName);
            }
            else
            {
                serverAdmin.createQueues(queue, flowControl, prefetch, storeName,
                    uniqueStores, Integer.parseInt(prodThrds));
            }
        }
    }

    /**
     * Send instructions to the slave program.
     */
    private void sendInstructions() throws JMSException
    {
        // create the "instructions" message
        MapMessage request = session.createMapMessage();

        request.setString("cmd", "instructions");
        request.setJMSReplyTo(replyDest);
        if (topic != null)
            request.setString("topic", topic);
        if (queue != null)
            request.setString("queue", queue);
        if (count != null)
            request.setString("count", count);
        if (runTime != null)
            request.setString("time", runTime);
        if (uniqueDests)
            request.setBoolean("uniquedests", uniqueDests);
        if (consThrds != null)
            request.setString("threads", consThrds);
        if (consConns != null)
            request.setString("connections", consConns);
        if (consTxnSize != null)
            request.setString("txnsize", consTxnSize);
        if (ackMode != null)
            request.setString("ackmode", ackMode);
        if (durableName != null)
            request.setString("durable", durableName);
        if (selector != null)
            request.setString("selector", selector);
        if (factoryName != null)
            request.setString("factory", factoryName);
        if (xa)
            request.setBoolean("xa", xa);
        
        // publish the request message
        msgProducer.send(request);
        
        // wait for all slave programs to respond
        int numReceivers = serverAdmin.getNumberOfReceivers(destination);
        while (numReceivers-- > 0)
        {
            System.err.println("Waiting for response from " + (numReceivers + 1) + " slave program(s)");
            msgConsumer.receive(120000);
        }
    }

    /**
     * Invoke the message producer.
     */
    private void runProducer()
    {
        // create the command line arguments
        List<String> argsList = new ArrayList<String>();
        
        if (serverUrl != null)
        {
            argsList.add("-server");
            argsList.add(serverUrl);
        }

        if (userName != null)
        {
            argsList.add("-user");
            argsList.add(userName);
        }

        if (password != null)
        {
            argsList.add("-password");
            argsList.add(password);
        }
        
        if (topic != null)
        {
            argsList.add("-topic");
            argsList.add(topic);
        }

        if (queue != null)
        {
            argsList.add("-queue");
            argsList.add(queue);
        }

        if (msgSize != null)
        {
            argsList.add("-size");
            argsList.add(msgSize);
        }

        if (count != null)
        {
            argsList.add("-count");
            argsList.add(count);
        }

        if (runTime != null)
        {
            argsList.add("-time");
            argsList.add(runTime);
        }

        if (delMode != null)
        {
            argsList.add("-delivery");
            argsList.add(delMode);
        }

        if (uniqueDests)
            argsList.add("-uniquedests");

        if (compression)
            argsList.add("-compression");

        if (xa)
            argsList.add("-xa");

        if (prodThrds != null)
        {
            argsList.add("-threads");
            argsList.add(prodThrds);
        }

        if (prodConns != null)
        {
            argsList.add("-connections");
            argsList.add(prodConns);
        }

        if (prodTxnSize != null)
        {
            argsList.add("-txnsize");
            argsList.add(prodTxnSize);
        }

        if (rate != null)
        {
            argsList.add("-rate");
            argsList.add(rate);
        }
        
        if (payloadFile != null)
        {
            argsList.add("-payload");
            argsList.add(payloadFile);
        }

        if (factoryName != null)
        {
            argsList.add("-factory");
            argsList.add(factoryName);
        }
        
        if (factoryName != null)
        {
            argsList.add("-storeName");
            argsList.add(storeName);
        }
        
        if (sendAsync)
        {
            argsList.add("-async");
        }

        String args[] = new String[argsList.size()];
        argsList.toArray(args);
        
        // run the performance message producer
        if (useC)
        {
            // invoke the C executable
            String line, lastLine = null;
            StringBuffer argsBuf = new StringBuffer();

            for (int i = 0; i < args.length; i++)
            {
                argsBuf.append(' ');
                argsBuf.append(args[i]);
            }
            
            try 
            {
                Process p = Runtime.getRuntime().exec(C_EXECUTABLE + argsBuf.toString());
                BufferedReader input = 
                    new BufferedReader(
                        new InputStreamReader(p.getInputStream()));
                while ((line = input.readLine()) != null)
                {
                    System.err.println(line);
                    lastLine = line;
                }
                input.close();

                // format: "<sentCount> times took <elapsedTime> milliseconds ..."
                StringTokenizer st = new StringTokenizer(lastLine);
                sentCount = Integer.parseInt(st.nextToken());
                st.nextToken();
                st.nextToken();
                elapsedTime = Long.parseLong(st.nextToken());
            }
            catch (Exception ex)
            {
                ex.printStackTrace();
            }
        }
        else
        {
            // invoke the java class
            tibjmsMsgProducerPerf t = new tibjmsMsgProducerPerf(args);
            elapsedTime = t.getElapsedTime();
            sentCount = t.getSentCount();
        }
    }

    /**
     * Print the performance results.
     */
    private void printPerformance() throws JMSException
    {
        // create the "get performance" message
        MapMessage request = session.createMapMessage();
        
        request.setString("cmd", "getperf");
        request.setJMSReplyTo(replyDest);
        
        // publish the request message
        msgProducer.send(request);

        // wait for all slave programs to respond
        int numReceivers = serverAdmin.getNumberOfReceivers(destination);
        while (numReceivers-- > 0)
        {
            System.err.println("Waiting for performance results from " +
                (numReceivers + 1) + " slave program(s)");

            MapMessage reply = (MapMessage)msgConsumer.receive(300000);

            String replyHostName = reply.getString("host");
            int replyRecvCount = reply.getInt("recvCount");
            long replyElapsed = reply.getLong("elapsed");
            
            try
            {
                int i = 0;
                String lines[] = new String[50];
                File f = new File(outputName);
                if (!f.exists())
                {
                    f.createNewFile();
                    initializeOutput(lines);
                }
                else
                {
                    mergeOutput(lines);
                }
                
                FileWriter fw = new FileWriter(outputName, false);
                
                writeLine(fw, lines[i++], topic != null ? "topic" : "queue");
                writeLine(fw, lines[i++], uniqueDests ? "parallel" : "single");
                writeLine(fw, lines[i++], msgSize);
                writeLine(fw, lines[i++], prodThrds != null ? prodThrds : "1");
                writeLine(fw, lines[i++], prodConns != null ? prodConns : "1");
                writeLine(fw, lines[i++], prodTxnSize != null ? prodTxnSize : "0");
                writeLine(fw, lines[i++], delMode);
                writeLine(fw, lines[i++], replyHostName);
                writeLine(fw, lines[i++], consThrds != null ? consThrds : "1");
                writeLine(fw, lines[i++], consConns != null ? consConns : "1");
                writeLine(fw, lines[i++], consTxnSize != null ? consTxnSize : "0");
                writeLine(fw, lines[i++], durableName != null);
                writeLine(fw, lines[i++], selector != null);
                writeLine(fw, lines[i++], acknowledgeModeToString(ackMode));
                writeLine(fw, lines[i++], failsafe ? "failsafe" : "default");
                writeLine(fw, lines[i++], flowControl !=0 ? String.valueOf(flowControl/1024) : "none");
                writeLine(fw, lines[i++], prefetch != 0 ? String.valueOf(prefetch) : "default");
                writeLine(fw, lines[i++], compression);
                writeLine(fw, lines[i++], rate != null ? rate : "unlimited");
                
                int perf = (int)(((double)sentCount)/(((double)(elapsedTime))/1000.0));
                
                writeLine(fw, lines[i++], sentCount);
                writeLine(fw, lines[i++], elapsedTime/1000.0);
                writeLine(fw, lines[i++], perf);
                writeLine(fw, lines[i++], (perf*Integer.parseInt(msgSize)/1024));
                
                perf = (int)(((double)replyRecvCount)/(((double)(replyElapsed))/1000.0));
                
                writeLine(fw, lines[i++], replyRecvCount);
                writeLine(fw, lines[i++], replyElapsed/1000.0);
                writeLine(fw, lines[i++], perf);
                writeLine(fw, lines[i++], (perf*Integer.parseInt(msgSize)/1024));
                
                fw.flush();
                fw.close();
            }
            catch (IOException e)
            {
                e.printStackTrace();
                System.exit(0);
            }
        }
    }

    private void writeLine(Writer writer, String line, String value) throws IOException
    {
        final int WIDTH = 15;
        writer.write(line);
        writer.write(value);
        for (int i = 0; i < WIDTH - value.length(); i++)
            writer.write(' ');
        writer.write("\n");
    }
    
    private void writeLine(Writer writer, String line, int value) throws IOException 
    {
        writeLine(writer, line, String.valueOf(value));
    }
    
    private void writeLine(Writer writer, String line, double value) throws IOException 
    {
        writeLine(writer, line, String.valueOf(value));
    }
    
    private void writeLine(Writer writer, String line, boolean value) throws IOException 
    {
        writeLine(writer, line, String.valueOf(value));
    }
    
    /**
     * Initialize the output lines.
     */
    private void initializeOutput(String lines[])
    {
        int i = 0;

        lines[i++] = "Topic/Queue                \t";
        lines[i++] = "Single/Parallel Streams    \t";
        lines[i++] = "Payload size               \t";
        lines[i++] = "Producer threads           \t";
        lines[i++] = "Producer conns             \t";
        lines[i++] = "Prod txn size              \t";
        lines[i++] = "Delivery mode              \t";
        lines[i++] = "Consumer host              \t";
        lines[i++] = "Consumer threads           \t";
        lines[i++] = "Consumer conns             \t";
        lines[i++] = "Cons txn size              \t";
        lines[i++] = "Durable                    \t";
        lines[i++] = "Selector                   \t";
        lines[i++] = "Acknowledgement mode       \t";
        lines[i++] = "Failsafe storage           \t";
        lines[i++] = "Flow control kbytes        \t";
        lines[i++] = "Prefetch                   \t";
        lines[i++] = "Compression                \t";
        lines[i++] = "Rate                       \t";
        lines[i++] = "Total messages sent        \t";
        lines[i++] = "Producer elapsed secs      \t";
        lines[i++] = "Producer msgs/sec          \t";
        lines[i++] = "Producer payload kbytes/sec\t";
        lines[i++] = "Total messages received    \t";
        lines[i++] = "Consumer elapsed secs      \t";
        lines[i++] = "Consumer msgs/sec          \t";
        lines[i++] = "Consumer payload kbytes/sec\t";
    }

    /**
     * Merge the output with any existing output.
     */
    private void mergeOutput(String lines[]) throws IOException
    {
        BufferedReader br = 
            new BufferedReader(new FileReader(outputName));

        for (int i = 0; i < lines.length; i++)
        {
            lines[i] = br.readLine();
            if (lines[i] == null)
                break;
            lines[i] = lines[i] + "\t";
        }
    }

    /**
     * Return the string representation of the acknowledge mode.
     */
    private String acknowledgeModeToString(String ackMode)
    {
        if (ackMode.equals("DUPS_OK"))
            return "DUPS";
        if (ackMode.equals("AUTO"))
            return "AUTO";
        if (ackMode.equals("CLIENT"))
            return "CLT";
        if (ackMode.equals("EXPLICIT_CLIENT"))
            return "ECLT";
        if (ackMode.equals("EXPLICIT_CLIENT_DUPS_OK"))
            return "EDUP";
        if (ackMode.equals("NO"))
            return "NO";
        return String.valueOf(ackMode);
    }

    /**
     * Print usage and exit. 
     */
    private void usage()
    {
        System.err.println();
        System.err.println("Usage: java tibjmsPerfMaster [options]");
        System.err.println();
        System.err.println("  where options are:");
        System.err.println();
        System.err.println("    -server       <url>         - Server URL. Default is \"tcp://localhost:7222\".");
        System.err.println("    -user         <username>    - User name. Default is null.");
        System.err.println("    -password     <password>    - User password. Default is null.");
        System.err.println("    -control      <topic-name>  - Control topic name. Default is \"topic.control\"");
        System.err.println("    -adminuser    <name>        - Admin name. Default is \"admin\".");
        System.err.println("    -adminpwd     <password>    - Admin password. Default is null");
        System.err.println("    -topic        <topic-name>  - Topic name. Default is \"topic.sample\".");
        System.err.println("    -queue        <queue-name>  - Queue name. No default.");
        System.err.println("    -size         <num bytes>   - Message payload size in bytes. Default is 100.");
        System.err.println("    -count        <num msgs>    - Number of messages to send. Default is 10k.");
        System.err.println("    -time         <seconds>     - Number of seconds to run. Default is 0.");
        System.err.println("    -delivery     <mode>        - Delivery mode. Default is NON_PERSISTENT.");
        System.err.println("                                  Other values: PERSISTENT and RELIABLE.");
        System.err.println("    -prodthrds    <num threads> - Number of producer threads. Default is 1.");
        System.err.println("    -prodconns    <num conns>   - Number of producer connections. Default is 1.");
        System.err.println("    -prodtxnsize  <num msgs>    - Number of nessages per producer transaction.");
        System.err.println("    -consthrds    <num threads> - Number of consumer threads. Default is 1.");
        System.err.println("    -consconns    <num conns>   - Number of consumer connections. Default is 1.");
        System.err.println("    -constxnsize  <num msgs>    - Number of messages per consumer transaction.");
        System.err.println("    -durable      <name>        - Durable subscription name. No default.");
        System.err.println("    -selector     <selector>    - Message selector for consumers. No default.");
        System.err.println("    -ackmode      <mode>        - Message acknowledge mode. Default is AUTO.");
        System.err.println("                                  Other values: DUPS_OK, CLIENT EXPLICIT_CLIENT,");
        System.err.println("                                  EXPLICIT_CLIENT_DUPS_OK and NO.");
        System.err.println("    -rate         <msg/sec>     - Message rate for producer threads. No default.");
        System.err.println("    -flowcontrol  <num bytes>   - EMS server flow control setting. No default.");
        System.err.println("    -prefetch     <num msgs>    - EMS server queue prefetch setting. No default.");
        System.err.println("    -channel      <name>        - Multicast channel name. No default.");
        System.err.println("    -payload      <file name>   - File containing message payload.");
        System.err.println("    -output       <file name>   - Output file name. Default is \"perf.txt\".");
        System.err.println("    -factory      <lookup name> - Lookup name for connection factory.");
        System.err.println("    -storeName    <store name>  - Name of the store.");
        System.err.println("    -failsafe                   - Enable EMS server failsafe setting.");
        System.err.println("    -uniquedests                - Each thread uses a unique destination.");
        System.err.println("    -uniquestores               - Each unique destination has unique store.");
        System.err.println("    -xa                         - Use XA transactions.");
        System.err.println("    -compression                - Enable message compression.");
        System.err.println("    -C                          - Use the C executable tibemsMsgConsumerPerf.");
        System.err.println("    -sendasync                  - Send asynchronously with a fcompletion listener.");
        
        System.exit(0);
    }

    /**
     * Cleanup server and close connection.
     */
    private void cleanup() throws JMSException
    {
        serverAdmin.cleanup(control);
        connection.close();
    }

    private void checkConflictingArgs() throws JMSException
    {
        if (failsafe && storeName != null)
            throw new JMSException("cannot specify failsafe & storename options at same time, mixed mode configuration is invalid");
        if (failsafe && uniqueStores)
            throw new JMSException("cannot specify failsafe & uniquestores options at same time, mixed mode configuration is invalid");
    }
    
    /**
     * Parse command line arguments.
     * 
     * @param args command line arguments
     */
    private void parseArgs(String[] args)
    {
        int i = 0;

        while (i < args.length)
        {
            if (args[i].equals("-server"))
            {
                if ((i+1) >= args.length) usage();
                serverUrl = args[i+1];
                i += 2;
            }
            else if (args[i].equals("-queue"))
            {
                if ((i+1) >= args.length) usage();
                queue = args[i+1];
                topic = null;
                i += 2;
            }
            else if (args[i].equals("-topic"))
            {
                if ((i+1) >= args.length) usage();
                topic = args[i+1];
                queue = null;
                i += 2;
            }
            else if (args[i].equals("-user"))
            {
                if ((i+1) >= args.length) usage();
                userName = args[i+1];
                i += 2;
            }
            else if (args[i].equals("-password"))
            {
                if ((i+1) >= args.length) usage();
                password = args[i+1];
                i += 2;
            }
            else if (args[i].equals("-adminuser"))
            {
                if ((i+1) >= args.length) usage();
                adminName = args[i+1];
                i += 2;
            }
            else if (args[i].equals("-adminpwd"))
            {
                if ((i+1) >= args.length) usage();
                adminPwd = args[i+1];
                i += 2;
            }
            else if (args[i].equals("-delivery"))
            {
                if ((i+1) >= args.length) usage();
                delMode = args[i+1];
                i += 2;
                if (!delMode.equals("PERSISTENT") &&
                    !delMode.equals("NON_PERSISTENT") &&
                    !delMode.equals("RELIABLE"))
                {
                    System.err.println("Error: invalid value of -delivery parameter");
                    usage();
                }
            }
            else if (args[i].equals("-count"))
            {
                if ((i+1) >= args.length) usage();
                count = args[i+1];
                i += 2;
            }
            else if (args[i].equals("-time"))
            {
                if ((i+1) >= args.length) usage();
                runTime = args[i+1];
                count = "0";
                i += 2;
            }
            else if (args[i].equals("-size"))
            {
                if ((i+1) >= args.length) usage();
                if (payloadFile == null)
                    msgSize = args[i+1];
                i += 2;
            }
            else if (args[i].equals("-uniquedests"))
            {
                uniqueDests = true;
                i += 1;
            }
            else if (args[i].equals("-uniquestores"))
            {
                uniqueStores = true;
                i += 1;
            }
            else if (args[i].equals("-xa"))
            {
                xa = true;
                i += 1;
            }
            else if (args[i].equals("-compression"))
            {
                compression = true;
                i += 1;
            }
            else if (args[i].equals("-prodthrds"))
            {
                if ((i+1) >= args.length) usage();
                prodThrds = args[i+1];
                i += 2;
            }
            else if (args[i].equals("-prodconns"))
            {
                if ((i+1) >= args.length) usage();
                prodConns = args[i+1];
                i += 2;
            }
            else if (args[i].equals("-prodtxnsize"))
            {
                if ((i+1) >= args.length) usage();
                prodTxnSize = args[i+1];
                i += 2;
            }
            else if (args[i].equals("-consthrds"))
            {
                if ((i+1) >= args.length) usage();
                consThrds = args[i+1];
                i += 2;
            }
            else if (args[i].equals("-consconns"))
            {
                if ((i+1) >= args.length) usage();
                consConns = args[i+1];
                i += 2;
            }
            else if (args[i].equals("-constxnsize"))
            {
                if ((i+1) >= args.length) usage();
                consTxnSize = args[i+1];
                i += 2;
            }
            else if (args[i].equals("-ackmode"))
            {
                if ((i+1) >= args.length) usage();
                ackMode = args[i+1];
                i += 2;
                if (!ackMode.equals("DUPS_OK") &&
                    !ackMode.equals("AUTO") &&
                    !ackMode.equals("CLIENT") &&
                    !ackMode.equals("EXPLICIT_CLIENT") &&
                    !ackMode.equals("EXPLICIT_CLIENT_DUPS_OK") &&
                    !ackMode.equals("NO"))
                {
                    System.err.println("Error: invalid value of -ackMode parameter");
                    usage();
                }
            }
            else if (args[i].equals("-durable"))
            {
                if ((i+1) >= args.length) usage();
                durableName = args[i+1];
                i += 2;
            }
            else if (args[i].equals("-selector"))
            {
                if ((i+1) >= args.length) usage();
                selector = args[i+1];
                i += 2;
            }
            else if (args[i].equals("-control"))
            {
                if ((i+1) >= args.length) usage();
                control = args[i+1];
                i += 2;
            }
            else if (args[i].equals("-failsafe"))
            {
                failsafe = true;
                i += 1;
            }
            else if (args[i].equals("-flowcontrol"))
            {
                if ((i+1) >= args.length) usage();
                flowControl = Integer.parseInt(args[i+1]);
                i += 2;
            }
            else if (args[i].equals("-prefetch"))
            {
                if ((i+1) >= args.length) usage();
                prefetch = Integer.parseInt(args[i+1]);
                i += 2;
            }
            else if (args[i].equals("-channel"))
            {
                if ((i+1) >= args.length) usage();
                channelName = args[i+1];
                i += 2;
            }
            else if (args[i].equals("-rate"))
            {
                if ((i+1) >= args.length) usage();
                rate = args[i+1];
                i += 2;
            }
            else if (args[i].equals("-output"))
            {
                if ((i+1) >= args.length) usage();
                outputName = args[i+1];
                i += 2;
            }
            else if (args[i].equals("-factory"))
            {
                if ((i+1) >= args.length) usage();
                factoryName = args[i+1];
                i += 2;
            }
            else if (args[i].equals("-payload"))
            {
                if ((i+1) >= args.length) usage();
                payloadFile = args[i+1];
                msgSize = String.valueOf(new File(payloadFile).length());
                i += 2;
            }
            else if (args[i].equals("-C"))
            {
                useC = true;
                i += 1;
            }
            else if (args[i].equals("-storeName"))
            {
                if ((i+1) >= args.length) usage();
                storeName = args[i+1];
                i += 2;
            }
            else if (args[i].equals("-sendasync"))
            {
                sendAsync = true;
                i += 1;
            }
            else if (args[i].equals("-help"))
            {
                usage();
            }
            else
            {
                System.err.println("Error: invalid option: " + args[i]);
                usage();
            }
        }
    }

    /**
     * main
     */
    public static void main(String[] args)
    {
        tibjmsPerfMaster t = new tibjmsPerfMaster(args);
    }
}
