/*
 * Copyright (c) 2013-2017 TIBCO Software Inc.
 * Licensed under a BSD-style license. Refer to [LICENSE]
 * For more information, please contact:
 * TIBCO Software Inc., Palo Alto, California, USA
 */

 /*
 * This is a sample java program which can be used (with the TibThruRecv program)
 * to measure basic throughput between the publisher and subscriber.
 *
 * NOTE: This application is multi-threaded using a busy wait in senderThread's start function,
 *       and should not be constrained to a single logical processor.
 */

package com.tibco.ftl.samples;

import java.text.*;
import java.util.*;

import com.tibco.ftl.*;

public class TibThruSend implements Runnable
{
    String realmServer              = null;
    String defaultUrl               = "http://localhost:8080";
    String defaultSecureUrl         = "https://localhost:8080";
    String appName                  = "tiblatsend";
    String endpointName             = "tiblatsend-sendendpoint";
    String recvEndpointName         = "tiblatsend-recvendpoint";
    String durableName              = null;
    int messageCount                = 5000000;
    int payloadSize                 = 16;
    int warmupCount                 = 100;
    int batchSize                   = 100;
    String trcLevel                 = null;
    String user                     = "guest";
    String password                 = "guest-pw";
    boolean trustAll                = false;
    String trustFile                = null;
    String clientLabel              = null;
    boolean inline_mode             = false;
    boolean report_mode             = true;
    String identifier               = null;
    boolean singleSend              = false;
    double pingInterval             = 1.0;
    EventTimer pongTimer            = null;

    byte[] payload                  = null;
    boolean running                 = true;
    boolean received                = false;
    boolean reportReceived          = false;
    String reportBuf                = null;
    int currentLimit                = 0;
    int numSamples                  = 0;
    Message[] sendMsgs              = null;
    Message signalMsg               = null;
    Publisher pub                   = null;
    Subscriber sub                  = null;
    Subscriber upAdvisorySub        = null;
    Subscriber downAdvisorySub      = null;
    MessageFieldRef dataRef         = null;
    EventQueue queue                = null;
    long sendStartTime              = 0;
    long sendStopTime               = 0;
    boolean storeAvailable          = true;
    boolean pongRemoved             = false;

    String          usageString =
            "Usage: TibThruSend [options] url\n" +
            "Default url is " + defaultUrl + "\n" +
            "   -a,  --application <name>        Application name\n" +
            "   -b,  --batchsize <int>           Batch size\n" +
            "   -cl, --client-label <string>     Set client label\n" +
            "   -c,  --count <int>               Send n messages.\n" +
            "   -d,  --durableName               Durable name for receive side.\n" +
            "   -et, --tx-endpoint <name>        Transmit endpoint name\n" +
            "   -er, --rx-endpoint <name>        Receive endpoint name\n" +
            "   -h,  --help                      Print this help.\n" +
            "   -i,  --inline                    Put EventQueue in INLINE mode.\n" +
            "   -id, --identifier                Choose instance, eg, \"rdma\"\n" +
            "   -p,  --password <string>         Password for realm server.\n" +
            "        --ping <float>              Initial ping timeout value (in seconds).\n" +
            "   -s,  --size <int>                Send payload of n bytes.\n" +
            "   -t,  --trace <level>             Set trace level\n" +
            "           where <level> can be:\n" +
            "                 off, severe, warn, info, debug, verbose\n" +
            "        --trustall\n" +
            "        --trustfile <file>\n" +
            "   -u,  --user <string>             User for realm server.\n" +
            "   -w,  --warmup <int>              Warmup for n batches (Default 100)\n" +
            "        --single                    Use multiple calls to Send instead of one call to SendMessages.\n";

    public TibThruSend(String[] args)
    {
        System.out.printf("#\n# %s\n#\n# %s\n#\n",
                          this.getClass().getName(),
                          FTL.getVersionInformation());
        
        parseArgs(args);
    }

    public void usage()
    {
        System.out.println(usageString);
        System.exit(0);
    }

    public void parseArgs(String args[])
    {
        int i           = 0;
        int argc        = args.length;
        String s        = null;
        int n           = 0;
        TibAux aux      = new TibAux(args, usageString);

        for (i = 0; i < argc; i++)
        {
            s = aux.getString(i, "--application", "-a");
            if (s != null) {
                appName = s;
                i++;
                continue;
            }
            n = aux.getInt(i, "--batchsize", "-b");
            if (n > -1) {
                batchSize = n;
                i++;
                continue;
            }
            s = aux.getString(i, "--client-label", "-cl");
            if (s != null) {
                clientLabel = s;
                i++;
                continue;
            }
            n = aux.getInt(i, "--count", "-c");
            if (n > -1) {
                messageCount = n;
                i++;
                continue;
            }
            s = aux.getString(i, "--tx-endpoint", "-et");
            if (s != null) {
                endpointName = s;
                i++;
                continue;
            }
            s = aux.getString(i, "--rx-endpoint", "-er");
            if (s != null) {
                recvEndpointName = s;
                i++;
                continue;
            }
            if (aux.getFlag(i, "--help", "-h")) {
                usage();
            }
            s = aux.getString(i, "--identifier", "-id");
            if (s != null) {
                identifier = s;
                i++;
                continue;
            }
            s = aux.getString(i, "--user", "-u");
            if (s != null) {
                user = s;
                i++;
                continue;
            }
            s = aux.getString(i, "--password", "-p");
            if (s != null) {
                password = s;
                i++;
                continue;
            }
            s = aux.getString(i, "--trace", "-t");
            if (s != null) {
                trcLevel = s;
                i++;
                continue;
            }
            if (aux.getFlag(i, "--trustall", "--trustall")) {
                if (trustFile != null)
                {
                    System.out.println("cannot specify both --trustall and --trustfile");
                    usage();
                }
                trustAll = true;
                continue;
            }
            s = aux.getString(i, "--trustfile", "--trustfile");
            if (s != null)
            {
                trustFile = s;
                if (trustAll)
                {
                    System.out.println("cannot specify both --trustall and --trustfile");
                    usage();
                }
                i++;
                continue;
            }
            double d = aux.getDouble(i, "--ping", "--ping");
            if (d > -1.0) {
                pingInterval = d;
                i++;
                continue;
            }
            n = aux.getInt(i, "--size", "-s");
            if (n > -1) {
                payloadSize = n;
                i++;
                continue;
            }
            s = aux.getString(i, "--durable", "-d");
            if (s != null) {
                durableName = s;
                i++;
                continue;
            }
            if (aux.getFlag(i, "--trustall", "--trustall")) {
                if (trustFile != null)
                {
                    System.out.println("cannot specify both --trustall and --trustfile");
                    usage();
                }
                trustAll = true;
                continue;
            }
            s = aux.getString(i, "--trustfile", "--trustfile");
            if (s != null)
            {
                trustFile = s;
                if (trustAll)
                {
                    System.out.println("cannot specify both --trustall and --trustfile");
                    usage();
                }
                i++;
                continue;
            }
            n = aux.getInt(i, "--warmup", "-w");
            if (n > -1) {
                warmupCount = n;
                i++;
                continue;
            }
            if (aux.getFlag(i, "--inline", "-i")) {
                inline_mode = true;
                continue;
            }
            if (aux.getFlag(i, "--no-receiver-report", "-norpt")) {
                report_mode = false;
                continue;
            }
            if (aux.getFlag(i, "--single", "-single")) {
                singleSend = true;
                continue;
            }
            if (args[i].startsWith("-", 0)) {
                System.out.println("invalid option: "+args[i]);
                usage();
            }
            realmServer = args[i];
        }

        if (payloadSize < 8)
        {
            System.err.println("Payload size must be at least 8\n");
            System.exit(1);
        }

        if (batchSize > messageCount)
        {
            System.err.println("Count must be greater than or equal to batch size\n");
            System.exit(1);
        }
        
        System.out.print("Invoked as: TibThruSend ");
        for (i = 0; i < argc; i++)
            System.out.print(args[i]+" ");
        System.out.println("");
    }

    public boolean protectedSend(Publisher pub,Message msg)
    {
        long  stopTime;
        boolean retVal = false;
        
        try
        {
            pub.send(msg);
            retVal = true;
        }
        catch(Exception exp)
        {
            // If send failed, we'll keep retrying for 60 seconds.
            stopTime = System.currentTimeMillis()+(60*1000);
            do
            {
                try
                {
                    // Try 10 times per second.
                    Thread.sleep(100);
            
                    // Don't bother trying if we're using a tibstore and it is unavailable.
                    if (storeAvailable)
                    {
                        pub.send(msg);
                        retVal = true;
                        break;//break if we are ok
                    }
                }
                catch(Exception ex)
                {
                    //keep going
                }
            } while (System.currentTimeMillis() < stopTime );   
        }
        
        return retVal;
    }

    protected boolean protectedSendMessages(Publisher pub, Message[] msgs)
    {
        long  stopTime;
        boolean retVal = false;
        
        try
        {
            pub.send(msgs);
            retVal = true;
        }
        catch(Exception exp)
        {
            // If send failed, we'll keep retrying for 60 seconds.
            stopTime = System.currentTimeMillis()+(60*1000);
            do
            {
                try
                {
                    // Try 10 times per second.
                    Thread.sleep(100);
            
                    // Don't bother trying if we're using a tibstore and it is unavailable.
                    if (storeAvailable)
                    {
                        pub.send(msgs);
                        retVal = true;
                        break;//break if we are ok
                    }
                }
                catch(Exception ex)
                {
                    //keep going
                }
            } while (System.currentTimeMillis() < stopTime );   
        }
        
        return retVal;
    }

    public void sendBatch()
    {
        int i;
        boolean success = true;
        
        if (singleSend)
        {
            for (i = 0;  i < batchSize;  i++)
            {
                success = success && protectedSend(pub, sendMsgs[i]);
            }
        }
        else
        {
            success = protectedSendMessages(pub, sendMsgs);
        }
        
        if(!success) System.exit(-1);
    }

    public byte[] convertToBinary(long i)
    {
        byte[] retVal = new byte[8];
        int index,shift;

        for(index = 0, shift = 0; index < 8; index++)
        {
            retVal[index] = (byte) ((i >>> shift) & 0xFF);
            shift += 8;
        }

        return retVal;
    }
    
    public void run()
    {
        long signalNum;
        long batchNum;
        double sendTime;
        
        try
        {
            if (pongTimer!=null)
            {
                // Send initial ping.
                received = false;
                signalNum = -4;
                signalMsg.setOpaque(dataRef, convertToBinary(signalNum));
                
                if(!protectedSend(pub, signalMsg)) System.exit(-1);
                
                while (running && !received)
                {
                    Thread.yield();
                }
            }
        
            for (batchNum = 0;  batchNum < warmupCount;  batchNum++)
            {
                while (running && !received)
                {
                    Thread.yield();
                }
                received = false;
                sendBatch();
            }
        
            signalNum = -1;
            signalMsg.setOpaque(dataRef, convertToBinary(signalNum));
            if(!protectedSend(pub, signalMsg)) System.exit(-1);
        
            sendStartTime = System.nanoTime();
            for (batchNum = 0;  batchNum < currentLimit;  batchNum++)
            {
                while (running && !received) 
                {
                    // Note: a thread yield is required here 
                    //       for single-processor constrained use.
                    Thread.yield();
                }
                received = false;
                sendBatch();
            }
            sendStopTime = System.nanoTime();
        
            if (report_mode)
                signalNum = -3;
            else
                signalNum = -2;
    
            signalMsg.setOpaque(dataRef, convertToBinary(signalNum));
            if(!protectedSend(pub, signalMsg)) System.exit(-1);
        
            NumberFormat nf = new DecimalFormat(" ##0.00E00 ");
            
            sendTime = (sendStopTime-sendStartTime)*1.0e-9;
            System.out.print("Sent ");
            System.out.print(nf.format(batchSize * numSamples));
            System.out.print(" messages in ");
            System.out.print(nf.format(sendTime));
            System.out.print(" seconds. (");
            System.out.print(nf.format((batchSize * numSamples)/sendTime));
            System.out.println(" msgs/sec)");
        
            if (report_mode)
            {
                while (running && !reportReceived) { /* wait for receiver report */ }
            }
            
            running = false;

            queue.removeSubscriber(sub);
            queue.removeSubscriber(upAdvisorySub);
            queue.removeSubscriber(downAdvisorySub);
            queue.destroy();
        }
        catch(Exception exp)
        {
            exp.printStackTrace();
            System.exit(-1);
        }
    }

    public void pongTimeoutCB()
    {
        // Resend the initial ping message if we haven't received a pong.
        if (pongTimer != null)
        {
            if (trcLevel!=null) System.out.printf("Resending initial message\n");
            protectedSend(pub, signalMsg);
        }
    }

    public void onUpAdvisory(List<Message> messages, EventQueue eventQueue)
    {
        storeAvailable = true;
        System.out.printf("Setting store availability to true.\n");
    }
    
    public void onDownAdvisory(List<Message> messages, EventQueue eventQueue)
    {
        storeAvailable = false;
        System.out.printf("Setting store availability to false.\n");
    }

    public void onMsg(List<Message> messages, EventQueue eventQueue)
    {
        try
        {
            received = true;
        
            for(int i = 0,max=messages.size();  i < max;  i++)
            {
                if (messages.get(i).isFieldSet("receiverReport") )
                {
                    reportBuf = messages.get(i).getString("receiverReport");
                    reportReceived = true;
                }
            }
        
            if (reportReceived == false && messages.size() != 1) {
                System.out.printf("Received %d messages instead of one.\n", messages.size());
                running = false;
            }
        }
        catch(Exception exp)
        {
            exp.printStackTrace();
            System.exit(-1);
        }
    }

    public void onPongMsg(List<Message> messages, EventQueue eventQueue)
    {
        try
        {
            if (pongTimer!=null)
            {
                queue.destroyTimer(pongTimer);
                pongTimer = null;
            }
        
            // Change our subscription before starting the real test interval.
            queue.removeSubscriber(sub);
            
            pongRemoved = true;
            
            if (messages.size() != 1)
            {
                System.out.printf("Received %d messages instead of one.\n", messages.size());
                running = false;
            }
        }
        catch(Exception exp)
        {
            exp.printStackTrace();
            System.exit(-1);
        }
    }

    public void start() throws Exception
    {
        Realm realm             = null;
        TibProperties props     = null;
        Thread senderThread     = null;
        ContentMatcher upAdvisoryMatcher;
        ContentMatcher downAdvisoryMatcher;
        
        numSamples = messageCount / batchSize;
    
        // Set global trace to specified level
        if (trcLevel != null)
        {
            FTL.setLogLevel(trcLevel);
        }
    
        // Get a single realm per process
        props = FTL.createProperties();
        props.set(Realm.PROPERTY_STRING_USERNAME, user);
        props.set(Realm.PROPERTY_STRING_USERPASSWORD, password);
        if(identifier!=null)
            props.set(Realm.PROPERTY_STRING_APPINSTANCE_IDENTIFIER, identifier);

        if (trustAll || trustFile != null)
        {
            if (realmServer == null)
                realmServer = defaultSecureUrl;

            if (trustAll)
            {
                props.set(Realm.PROPERTY_LONG_TRUST_TYPE, Realm.HTTPS_CONNECTION_TRUST_EVERYONE);
            }
            else
            {
                props.set(Realm.PROPERTY_LONG_TRUST_TYPE, Realm.HTTPS_CONNECTION_USE_SPECIFIED_TRUST_FILE);
                props.set(Realm.PROPERTY_STRING_TRUST_FILE, trustFile);
            }
        }
        else
        {
            if (realmServer == null)
                realmServer = defaultUrl;
        }

        if (clientLabel != null)
            props.set(Realm.PROPERTY_STRING_CLIENT_LABEL, clientLabel);
        else
            props.set(Realm.PROPERTY_STRING_CLIENT_LABEL, this.getClass().getSimpleName());

        realm = FTL.connectToRealmServer(realmServer, appName, props);
        props.destroy();
    
        dataRef = realm.createMessageFieldRef(Message.TIB_BUILTIN_MSG_FMT_OPAQUE_FIELDNAME);
    
        props = FTL.createProperties();
        props.set(EventQueue.PROPERTY_BOOL_INLINE_MODE, inline_mode);

        queue = realm.createEventQueue(props);
        props.destroy();

        props = FTL.createProperties();
        if (durableName != null)
            props.set(Subscriber.PROPERTY_STRING_DURABLE_NAME, durableName);
        sub = realm.createSubscriber(recvEndpointName, null, props);
        props.destroy();
        
        if(pingInterval != 0)
            queue.addSubscriber(sub, new SubscriberListener()
            {
                public void messagesReceived(List<Message> messages, EventQueue eventQueue)
                {
                    onPongMsg(messages,eventQueue);
                }
            });
        else
            queue.addSubscriber(sub, new SubscriberListener()
            {
                public void messagesReceived(List<Message> messages, EventQueue eventQueue)
                {
                    onMsg(messages,eventQueue);
                }
            });
    
        upAdvisoryMatcher = realm.createContentMatcher(
                "{\""+ Advisory.FIELD_ADVISORY + "\":1," +
                " \"" + Advisory.FIELD_NAME + "\":\"" + Advisory.NAME_RESOURCE_AVAILABLE + "\"," +
                " \"" + Advisory.FIELD_REASON + "\":\"" + Advisory.REASON_PERSISTENCE_STORE_AVAILABLE + "\"}");
        
        upAdvisorySub = realm.createSubscriber(Advisory.ENDPOINT_NAME, upAdvisoryMatcher);
        upAdvisoryMatcher.destroy();

        queue.addSubscriber(upAdvisorySub, new SubscriberListener()
        {
            public void messagesReceived(List<Message> messages, EventQueue eventQueue)
            {
                onUpAdvisory(messages,eventQueue);
            }
        });
        
        downAdvisoryMatcher = realm.createContentMatcher(
                "{\""+ Advisory.FIELD_ADVISORY + "\":1," +
                " \"" + Advisory.FIELD_NAME + "\":\"" + Advisory.NAME_RESOURCE_UNAVAILABLE + "\"," +
                " \"" + Advisory.FIELD_REASON + "\":\"" + Advisory.REASON_PERSISTENCE_STORE_UNAVAILABLE + "\"}");
        
        downAdvisorySub = realm.createSubscriber(Advisory.ENDPOINT_NAME, downAdvisoryMatcher);
        downAdvisoryMatcher.destroy();

        queue.addSubscriber(downAdvisorySub, new SubscriberListener()
        {
            public void messagesReceived(List<Message> messages, EventQueue eventQueue)
            {
                onDownAdvisory(messages,eventQueue);
            }
        });
    
        pub = realm.createPublisher(endpointName);
    
        payload = new byte[payloadSize];
        sendMsgs = new Message[batchSize];
        
        for (int i = 0;  i < batchSize;  i++)
        {
            sendMsgs[i] = realm.createMessage(Message.TIB_BUILTIN_MSG_FMT_OPAQUE);
            sendMsgs[i].setOpaque(dataRef, payload);
        }
        
        byte[] oneBytes = convertToBinary(1);
        for(int i=0,max=oneBytes.length;i<max;i++)
        {
            payload[i] = oneBytes[i];
        }

        sendMsgs[0].setOpaque(dataRef, payload);
        
        signalMsg = realm.createMessage(Message.TIB_BUILTIN_MSG_FMT_OPAQUE);
        
        System.out.printf("\nSender Report:\n");
        System.out.printf("Requested %d messages.\n", messageCount);
        System.out.printf("Sending %d messages (%d batches of %d) with payload size %d\n", 
               batchSize * numSamples,
               numSamples, batchSize, payloadSize);
        System.out.flush();
    
        currentLimit = numSamples;
        received = true;
    
        if (pingInterval != 0)
        {
            pongTimer = queue.createTimer(pingInterval, new EventTimerListener()
            {
                @Override
                public void timerFired(EventTimer timer, EventQueue eventQueue)
                {
                    pongTimeoutCB();
                }
            });
        }
        
        senderThread = new Thread(this);
        senderThread.start();
        
        while(running)
        {
            try
            {
                queue.dispatch();
                if (pongRemoved)
                {
                    queue.addSubscriber(sub, new SubscriberListener()
                        {
                            public void messagesReceived(List<Message> messages, EventQueue eventQueue)
                                {
                                    onMsg(messages,eventQueue);
                                }
                        });
            
                    received = true;
                    pongRemoved = false;
                }

            }
            catch(Exception exp)
            {
                exp.printStackTrace();
            }
        }
        
        senderThread.join();
        
        if(report_mode == true)
        {
            System.out.printf("\nReceiver Report:\n%s\n",reportBuf);
        }
    
        System.out.flush();
    
        sub.close();
        upAdvisorySub.close();
        downAdvisorySub.close();
        pub.close();
    
        if (sendMsgs!=null)
        {
            for(int i = 0;  i < batchSize;  i++)
            {
                sendMsgs[i].destroy();
                sendMsgs[i] = null;
            }
        }
        
        signalMsg.destroy();
        dataRef.destroy();
        realm.close();
    }
    
    public static void main(String[] args)
    {
        TibThruSend s  = new TibThruSend(args);
        try
        {
            s.start();
        }
        catch (Throwable e)
        {
            e.printStackTrace();
            System.exit(-1);
        }
    }
}
