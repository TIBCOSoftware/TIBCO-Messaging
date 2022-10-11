/*
 * Copyright (c) 2010-2017 TIBCO Software Inc.
 * Licensed under a BSD-style license. Refer to [LICENSE]
 * For more information, please contact:
 * TIBCO Software Inc., Palo Alto, California, USA
 */

/*
 * This is a sample java program which can be used (with the TibLatRecv program)
 * to measure basic latency between the publisher and subscriber.
 */

package com.tibco.ftl.samples;

import java.text.*;
import java.util.*;
import java.io.*;
import com.tibco.ftl.*;

public class TibLatSend implements SubscriberListener, EventTimerListener
{
    String appName = "tiblatsend";
    String realmServer = "http://localhost:8080";
    String endpointName = "tiblatsend-sendendpoint";
    String recvEndpointName = "tiblatsend-recvendpoint";
    String durableName = null;
    boolean explicitAck    = false;
    String trcLevel = null;
    String user = "guest";
    String password = "guest-pw";
    String identifier = null;
    String csvFileName = null;
    String clientLabel = null;
    FileOutputStream out;
    PrintStream p;
    int messageCount = 5000000;
    int sampleInterval = 0;
    int payloadSize = 16;
    int warmupCount = 10000;
    double pingInterval  = 1.0;
    boolean quiet = false;
    boolean csvFile = false;
    volatile EventTimer pongTimer = null;

    String usageString = "Usage: TibLatSend [options] url\n"
            + "Default url is "
            + realmServer
            + "\n"
            + " where options can be:\n"
            + "   -a,  --application <name>       Application name\n"
            + "   -c,  --count <int>              Send n messages\n"
            + "   -cl, --client-label <string>    Set client label\n"
            + "   -d,  --durableName <name>       Durable name for receive side.\n"
            + "   -et, --tx-endpoint <name>       Transmit endpoint name\n"
            + "   -er, --rx-endpoint <name>       Receive endpoint name\n"
            + "   -h,  --help                     Print this help\n"
            + "   -id,  --identifier              Choose instance, eg, \"rdma\"\n"
            + "   -i,  --interval <int>           Sample latency every n messages\n"
            + "   -o,  --out <file name>          Output samples to csv file\n"
            + "   -p,  --pasword <string>         Password for realm server\n"
            + "        --ping <float>             Ping retry interval (in seconds)\n"
            + "   -q,  --quiet                    Don't print intermediate results\n"
            + "   -s,  --size <int>               Send payload of n bytes.\n"
            + "   -t,  --trace <level>            Set trace level\n"
            + "           where <level> can be:\n"
            + "                 off, severe, warn, info, debug, verbose\n"
            + "   -u,  --user <string>            User for realm server\n"
            + "   -w,  --warmup <int>             Warmup for n msgs (Default "
            +                                     warmupCount + ")\n"
            + "   -x,  --explicit-ack             Use explicit acks.\n";

    boolean running = false;
    int messageProgress = 0;
    int currentLimit = 0;
    Message msg = null;
    Publisher pub = null;
    int sampleCountdown = 0;
    int numSamples = 0;
    int nextSample = 0;

    TibAuxStatRecord latencyStats = new TibAuxStatRecord();

    long[] samples = null;

    public TibLatSend(String[] args)
    {
        System.out.printf("#\n# %s\n#\n# %s\n#\n", this.getClass().getName(),
                          FTL.getVersionInformation());

        parseArgs(args);
    }

    public void usage()
    {
        System.out.printf(usageString);
        System.exit(0);
    }

    public void parseArgs(String args[]) {
        int i = 0;
        int argc = args.length;
        String s = null;
        int n = 0;
        double d = 0;
        TibAux aux = new TibAux(args, usageString);

        for (i = 0; i < argc; i++) {
            s = aux.getString(i, "--application", "-a");
            if (s != null) {
                appName = s;
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
            if (aux.getFlag(i, "--explicit-Ack", "-x")) {
                explicitAck = true;
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
            n = aux.getInt(i, "--interval", "-i");
            if (n > -1) {
                sampleInterval = n;
                i++;
                continue;
            }
            s = aux.getString(i, "--out", "-o");
            if (s != null) {
                csvFileName = s;
                csvFile = true;
                i++;
                continue;
            }
            s = aux.getString(i, "--password", "-p");
            if (s != null) {
                password = s;
                i++;
                continue;
            }
            d = aux.getDouble(i, "--ping", "--ping");
            if (d > -1.0) {
                pingInterval = d;
                i++;
                continue;
            }
            if (aux.getFlag(i, "--quiet", "-q")) {
                quiet = true;
                continue;
            }
            n = aux.getInt(i, "--size", "-s");
            if (n > -1) {
                payloadSize = n;
                i++;
                continue;
            }
            s = aux.getString(i, "--durableName", "-d");
            if (s != null) {
                durableName = s;
                i++;
                continue;
            }
            s = aux.getString(i, "--trace", "-t");
            if (s != null) {
                trcLevel = s;
                i++;
                continue;
            }
            s = aux.getString(i, "--user", "-u");
            if (s != null) {
                user = s;
                i++;
                continue;
            }
            n = aux.getInt(i, "--warmup", "-w");
            if (n > -1) {
                warmupCount = n;
                i++;
                continue;
            }
            if (args[i].startsWith("-", 0)) {
                System.out.println("invalid option: " + args[i]);
                usage();
            }
            realmServer = args[i];
        }

        if (sampleInterval == 0)
            sampleInterval = messageCount;

        System.out.printf("Invoked as: TibLatSend ");
        for (i = 0; i < argc; i++)
            System.out.printf(args[i] + " ");
        System.out.printf("\n\n");

    }

    public void messagesReceived(List<Message> messages, EventQueue eventQueue)
    {
        int msgNum = messages.size();
        long stop = 0;

        if (msgNum != 1)
        {
            System.out.printf("Received %d messages instead of one.\n", msgNum);
            System.exit(1);
        }

        if (--sampleCountdown == 0)
        {
            stop = System.nanoTime();
        }

        try
        {
            if (++messageProgress < currentLimit)
                pub.send(msg);
            else
                running = false;

            if (explicitAck)
                messages.get(0).acknowledge();

            if (pongTimer != null)
            {
                eventQueue.destroyTimer(pongTimer);
                pongTimer = null;
            }
        }
        catch (FTLException e)
        {
            e.printStackTrace();
            running = false;
        }

        if (sampleCountdown == 0)
        {
            samples[nextSample++] = stop;
            sampleCountdown = sampleInterval;
        }
    }

    public void subscriberCompleted(Subscriber subscriber, EventQueue eventQueue)
    {
        System.out.println("Subscriber removed");
    }

    public void timerFired(EventTimer timer,EventQueue eventQueue)
    {
        try
        {
            if (trcLevel != null)
                System.out.printf("resending initial message\n");
            pub.send(msg);
        }
        catch (FTLException e)
        {
            e.printStackTrace();
            running = false;
        }
    }

    public int send() throws FTLException
    {
        TibProperties props;
        Realm realm;
        Subscriber sub;
        EventQueue queue;
        int i;
        NumberFormat nf = new DecimalFormat(" ##0.00E00 ");

        // Set sample buffer
        numSamples = messageCount / sampleInterval;
        samples = new long[numSamples + 1];

        // Setup CSVFile
        if (csvFile)
        {
            try
            {
                out = new FileOutputStream(csvFileName);
                p = new PrintStream(out);

                System.out
                        .printf("Producing a csv file named %s.  The columns in the file are:\n",
                                csvFileName);
                System.out
                        .printf("Sample number, latency, total time of sample, number of messages in sample,\n");
                System.out.printf("and the time offset of the sample.\n");
                System.out
                        .printf("The output can be analyzed with the analyze-tiblatsend script.\n\n");
            } catch (Throwable e)
            {
                e.printStackTrace();
            }
        }

        // Set global trace to specified level
        if (trcLevel != null)
            FTL.setLogLevel(trcLevel);

        // Get a single realm per process
        props = FTL.createProperties();
        props.set(Realm.PROPERTY_STRING_USERNAME, user);
        props.set(Realm.PROPERTY_STRING_USERPASSWORD, password);
        if (identifier != null)
            props.set(Realm.PROPERTY_STRING_APPINSTANCE_IDENTIFIER, identifier);

        if (clientLabel != null)
            props.set(Realm.PROPERTY_STRING_CLIENT_LABEL, clientLabel);
        else
            props.set(Realm.PROPERTY_STRING_CLIENT_LABEL, this.getClass().getSimpleName());

        realm = FTL.connectToRealmServer(realmServer, appName, props);
        props.destroy();

        // create a queue in inline mode for low latency
        props = FTL.createProperties();
        props.set(EventQueue.PROPERTY_BOOL_INLINE_MODE, true);
        queue = realm.createEventQueue(props);
        props.destroy();

        // Create a subscriber
        props = FTL.createProperties();
        if (durableName != null)
            props.set(Subscriber.PROPERTY_STRING_DURABLE_NAME, durableName);
        if (explicitAck)
            props.set(Subscriber.PROPERTY_BOOL_EXPLICIT_ACK, explicitAck);
        sub = realm.createSubscriber(recvEndpointName, null, props);
        queue.addSubscriber(sub, this);
        props.destroy();

        // Create a publisher
        pub = realm.createPublisher(endpointName);

        System.out.printf("Sending %d messages with payload size %d\n",
                messageCount, payloadSize);
        System.out.printf("Sampling latency every %d messages.\n\n",
                sampleInterval);

        // Create a message for sending
        byte[] data = new byte[payloadSize];
        for (i = 0; i < payloadSize; i++)
            data[i] = (byte) i;
        msg = realm.createMessage(Message.TIB_BUILTIN_MSG_FMT_OPAQUE);
        msg.setOpaque(Message.TIB_BUILTIN_MSG_FMT_OPAQUE_FIELDNAME, data);

        // Should we retry the initial ping until we get a pong?
        if (pingInterval != 0)
            pongTimer = queue.createTimer(pingInterval, this);

        if (warmupCount > 0)
        {
            messageProgress = 0;
            running = true;
            currentLimit = warmupCount;
            pub.send(msg);
            while (running)
                queue.dispatch();
        }

        messageProgress = 0;
        running = true;
        currentLimit = messageCount;
        sampleCountdown = sampleInterval;
        samples[nextSample++] = System.nanoTime();
        pub.send(msg);
        while (running)
            queue.dispatch();

        for (i = 1; i <= numSamples; i++)
        {
            double totalTime = (samples[i] - samples[i - 1]) * 1.0e-9;
            double latency = 0.5 * totalTime / (double) sampleInterval;

            latencyStats.StatUpdate(samples[i] - samples[i - 1]);

            if (!quiet)
                System.out
                        .printf("Total time: %s sec. for %11d messages      One way latency: %s sec.\n",
                                nf.format(totalTime), sampleInterval,
                                nf.format(latency));

            if (csvFile)
            {
                p.printf("%d,%s,%s,%d,%.12e\n", latencyStats.N,
                        nf.format(latency), nf.format(totalTime),
                        sampleInterval,
                        ((samples[i - 1] - samples[0]) * 1.0e-9));
            }
        }

        if (latencyStats.N > 1)
        {
            System.out
                    .printf("\n%d samples taken, each one averaging results from %d round trips.\n",
                            latencyStats.N, sampleInterval);
            System.out.printf("Aggregate Latency: ");
            latencyStats.PrintStats((0.5 * 1.0e-9) / sampleInterval);
            System.out.printf("\n\n");
        }

        if (csvFile)
        {
            p.close();
        }

        // Clean up
        queue.removeSubscriber(sub);
        if (pongTimer != null)
            queue.destroyTimer(pongTimer);
        queue.destroy();
        sub.close();
        realm.close();

        return 0;
    }

    public static void main(String[] args)
    {
        TibLatSend s = new TibLatSend(args);
        try
        {
            s.send();
        } catch (Throwable e)
        {
            e.printStackTrace();
        }
    }
}
