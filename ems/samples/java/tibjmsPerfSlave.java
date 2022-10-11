/* 
 * Copyright (c) 2005-$Date: 2016-12-13 17:00:37 -0600 (Tue, 13 Dec 2016) $ TIBCO Software Inc. 
 * Licensed under a BSD-style license. Refer to [LICENSE]
 * For more information, please contact:
 * TIBCO Software Inc., Palo Alto, California, USA
 * 
 * $Id: tibjmsPerfSlave.java 90180 2016-12-13 23:00:37Z olivierh $
 * 
 */

/*
 * The tibjmsPerfSlave class receives instructions from the
 * tibjmsPerfMaster class to invoke the performance message
 * consumer. At the end of the run the tibjmsPerfSlave class
 * sends the results back to the tibjmsPerfMaster class. The
 * tibjmsPerfSlave and tibjmsPerfMaster communicate over a
 * specified control topic. 
 *  
 * Usage:  java tibjmsPerfSlave [options]
 *
 *  where options are:
 *
 *   -server    <url>         EMS server URL. Default is
 *                            "tcp://localhost:7222".
 *   -user      <username>    User name. Default is null.
 *   -password  <password>    User password. Default is null.
 *   -control   <topic-name>  Control topic name over which
 *                            the slave and master communicate.
 *                            Default is "topic.control".
 *   -C                       Use the C executable instead of the java class. 
 */

import java.util.*;
import java.io.*;
import java.net.*;

import javax.jms.*;

public class tibjmsPerfSlave implements MessageListener, Runnable
{
    /*
     * constants
     */
    private static final String C_EXECUTABLE = "tibemsMsgConsumerPerf";
    
    /*
     * parameters
     */
    private String serverUrl = "tcp://localhost:7222";
    private String control = "topic.control";
    private String userName = null;
    private String password = null;
    private boolean useC = false;

    /*
     * variables
     */
    private MessageProducer msgProducer;
    private Thread runThread;
    private String[] runArgs;
    private long elapsedTime;
    private int receiveCount;

    /**
     * Constructor
     * 
     * @param args the command line arguments
     */
    public tibjmsPerfSlave(String[] args)
    {
        parseArgs(args);

        printStartBanner();
        
        try
        {
            initializeControls();
        }
        catch (JMSException e)
        {
            e.printStackTrace();
            System.exit(-1);
        }
    }

    /**
     * Message listener callback to process messages
     * received from the master program.
     * 
     * @param msg the received message
     */
    public void onMessage(Message msg)
    {
        if (msg instanceof MapMessage)
        {
            MapMessage mapMsg = (MapMessage)msg;
            try
            {
                String cmd = mapMsg.getString("cmd");
                if ("instructions".equals(cmd))
                {
                    processInstructions(mapMsg);
                }
                else if ("getperf".equals(cmd))
                {
                    processGetPerformance(mapMsg);
                }
                else
                {
                    System.err.println("Received unexpected command:");
                    System.err.println(msg);
                }
            }
            catch (JMSException e)
            {
                e.printStackTrace();
            }
        }
        else
        {
            System.err.println("Received unexpected message:");
            System.err.println(msg);
        }
    }
    
    /**
     * Print the start banner.
     */
    private void printStartBanner()
    {
        System.err.println();
        System.err.println("------------------------------------------------------------------------");
        System.err.println("tibjmsPerfSlave program");
        System.err.println("------------------------------------------------------------------------");
        System.err.println("Server....................... " + serverUrl);
        System.err.println("User......................... " + userName);
        System.err.println("Control Topic................ " + control);
        if (useC)
            System.err.println("Executable................... " + C_EXECUTABLE);
        System.err.println("------------------------------------------------------------------------");
        System.err.println();
    }

    /**
     * Initialize the control channel on which to receive
     * instructions from the master control program.
     */
    private void initializeControls() throws JMSException
    {
        // lookup the connection factory
        ConnectionFactory factory = 
            new com.tibco.tibjms.TibjmsConnectionFactory(serverUrl);
        
        // create the connection
        Connection connection = factory.createConnection(userName, password);
        
        // create the session
        Session session = connection.createSession();
        
        // create a message producer
        msgProducer = session.createProducer(null);
        
        // create the control topic
        Destination destination = session.createTopic(control);
        
        // create the message consumer on the control topic
        MessageConsumer consumer = session.createConsumer(destination);
        
        // set the message listener
        consumer.setMessageListener(this);
        
        // start receiving messages
        connection.start();
    }

    /**
     * Process instructions from master program.
     * 
     * @param msg request message from master program
     */
    private void processInstructions(MapMessage request) throws JMSException
    {
        // create the command line arguments
        List<String> argsList = new ArrayList<String>();
        String tmpstr;

        if (serverUrl != null)
        {
            argsList.add("-server");
            argsList.add(serverUrl);
        }

        tmpstr = request.getString("topic");
        if (tmpstr != null)
        {
            argsList.add("-topic");
            argsList.add(tmpstr);
        }

        tmpstr = request.getString("queue");
        if (tmpstr != null)
        {
            argsList.add("-queue");
            argsList.add(tmpstr);
        }

        tmpstr = request.getString("count");
        if (tmpstr != null)
        {
            argsList.add("-count");
            argsList.add(tmpstr);
        }

        tmpstr = request.getString("time");
        if (tmpstr != null)
        {
            argsList.add("-time");
            argsList.add(tmpstr);
        }

        tmpstr = request.getString("threads");
        if (tmpstr != null)
        {
            argsList.add("-threads");
            argsList.add(tmpstr);
        }

        tmpstr = request.getString("connections");
        if (tmpstr != null)
        {
            argsList.add("-connections");
            argsList.add(tmpstr);
        }

        tmpstr = request.getString("ackmode");
        if (tmpstr != null)
        {
            argsList.add("-ackmode");
            argsList.add(tmpstr);
        }

        if (request.getBoolean("uniquedests"))
        {
            argsList.add("-uniquedests");
        }
        
        tmpstr = request.getString("txnsize");
        if (tmpstr != null)
        {
            argsList.add("-txnsize");
            argsList.add(tmpstr);
        }

        tmpstr = request.getString("durable");
        if (tmpstr != null)
        {
            argsList.add("-durable");
            argsList.add(tmpstr + new Random().nextInt());
        }

        tmpstr = request.getString("selector");
        if (tmpstr != null)
        {
            argsList.add("-selector");
            argsList.add(tmpstr);
        }

        tmpstr = request.getString("factory");
        if (tmpstr != null)
        {
            argsList.add("-factory");
            argsList.add(tmpstr);
        }
        
        if (request.getBoolean("xa"))
        {
            argsList.add("-xa");
        }

        runArgs = new String[argsList.size()];
        argsList.toArray(runArgs);

        // interrupt a currently running message consumer thread
        if (runThread != null && runThread.isAlive())
            runThread.interrupt();
        
        // start the performance message consumer thread
        runThread = new Thread(this);
        runThread.start();
        
        // send reply to master program
        System.err.println("Sending reply to instructions");
        msgProducer.send(request.getJMSReplyTo(), request);
    }

    /**
     * Invoke the performance message consumer.
     * 
     * @param args arguments to pass to the message consumer
     */
    public void run()
    {
        try
        {
            if (useC)
            {
                // invoke the C executable
                String line, lastLine = null;
                StringBuffer buf = new StringBuffer();
                
                for (int i = 0; i < runArgs.length; i++)
                {
                    buf.append(' ');
                    if (runArgs[i].indexOf(' ') != -1)
                        buf.append('\"' + runArgs[i] + '\"');
                    else
                        buf.append(runArgs[i]);
                }
                
                Process p = Runtime.getRuntime().exec(C_EXECUTABLE + buf.toString());
                BufferedReader input = 
                    new BufferedReader(
                            new InputStreamReader(p.getInputStream()));
                while ((line = input.readLine()) != null)
                {
                    System.err.println(line);
                    lastLine = line;
                }
                input.close();
                
                // format: "<receiveCount> times took <elapsedTime> milliseconds ..."
                StringTokenizer st = new StringTokenizer(lastLine);
                receiveCount = Integer.parseInt(st.nextToken());
                st.nextToken();
                st.nextToken();
                elapsedTime = Long.parseLong(st.nextToken());
            }
            else
            {
                // invoke the java class
                tibjmsMsgConsumerPerf t = new tibjmsMsgConsumerPerf(runArgs);
                elapsedTime = t.getElapsedTime();
                receiveCount = t.getReceiveCount();
            }
        }
        catch (Exception ex)
        {
            ex.printStackTrace();
            elapsedTime = 0;
            receiveCount = 0;
        }
    }

    /**
     * Process get performance request from master program.
     * 
     * @param msg request message from master program
     */
    private void processGetPerformance(MapMessage request) throws JMSException
    {
        // wait for the thread to exit
        try 
        {
            runThread.join(120000);
        }
        catch (InterruptedException e) { }

        request.clearBody();
        
        request.setString("-server", serverUrl);
        request.setString("-user", userName);
        request.setString("host", getHostName());
        request.setLong("elapsed", elapsedTime);
        request.setInt("recvCount", receiveCount);
        
        System.err.println("Sending reply to performance request");
        msgProducer.send(request.getJMSReplyTo(), request);
    }

    /**
     * Get the localhost name.
     * 
     * @return the localhost name
     */
    private String getHostName()
    {
        try 
        {
            return InetAddress.getLocalHost().getHostName();
        }
        catch (UnknownHostException e)
        {
            return "(unknown)";
        }
    }

    /**
     * Print usage and exit.
     */
    private void usage()
    {
        System.err.println();
        System.err.println("Usage: java tibjmsPerfSlave [options]");
        System.err.println();
        System.err.println("  where options are:");
        System.err.println();
        System.err.println("    -server    <url>         - Server URL. Default is \"tcp://localhost:7222\".");
        System.err.println("    -user      <username>    - User name. Default is null.");
        System.err.println("    -password  <password>    - User password. Default is null.");
        System.err.println("    -control   <topic-name>  - Control topic name. Default is \"topic.control\".");
        System.err.println("    -C                       - Use the C executable tibemsMsgConsumerPerf.");
        
        System.exit(0);
    }

    /**
     * Parse command line arguments.
     * 
     * @param args command line arguments
     */
    private void parseArgs(String[] args)
    {
        int i=0;

        while (i < args.length)
        {
            if (args[i].equals("-server"))
            {
                if ((i+1) >= args.length) usage();
                serverUrl = args[i+1];
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
            else if (args[i].equals("-control"))
            {
                if ((i+1) >= args.length) usage();
                control = args[i+1];
                i += 2;
            }
            else if (args[i].equals("-C"))
            {
                useC = true;
                i += 1;
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
        tibjmsPerfSlave t = new tibjmsPerfSlave(args);
    }
}
