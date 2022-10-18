/*
 * Copyright (c) 2013-2017 TIBCO Software Inc.
 * Licensed under a BSD-style license. Refer to [LICENSE]
 * For more information, please contact:
 * TIBCO Software Inc., Palo Alto, California, USA
 */

/*
 * This is a sample java responder program which is used with the TibThruSend
 * program to measure basic throughput between the publisher and subscriber.
 */

package com.tibco.ftl.samples;

import java.text.*;
import java.util.*;

import com.tibco.ftl.*;

public class TibThruRecv
{
    String   appName        = "tiblatrecv";
    String   realmServer    = null;
    String   defaultUrl     = "http://localhost:8080";
    String   defaultSecureUrl = "https://localhost:8080";
    String   endpointName   = "tiblatrecv-sendendpoint";
    String   recvEndpointName = "tiblatrecv-recvendpoint";
    String   durableName    = null;
    String   trcLevel       = null;
    String   user           = "guest";
    String   password       = "guest-pw";
    String   identifier     = null;
    boolean  inline_mode    = false;
    boolean  doEcho         = true;
    boolean  doAdvisory     = true;
    boolean  trustAll       = false;
    String   trustFile      = null;
    String   clientLabel    = null;

    Publisher pub           = null;
    boolean   running       = true;
    MessageFieldRef dataRef = null;
    Message reportMessage   = null;
    long    start;
    long    count;
    long    bytes;
    TibAuxStatRecord blockSizeStats = new TibAuxStatRecord();

    String   usageString    =
        "TibThruRecv [options] url\n" +
        "Default url is " + defaultUrl + "\n" +
        "  where options can be:\n" +
        "    -a,  --application <name>          Application name\n" +
        "    -cl, --client-label <string>       Set client label\n" +
        "    -d,  --durableName <string>        Durable name for receive side\n" +
        "    -et, --tx-endpoint <name>          Transmit endpoint name\n" +
        "    -er, --rx-endpoint <name>          Receive endpoint name\n" +
        "    -h,  --help                        Print this help\n" +
        "    -id, --identifier <string>         Choose instance, eg, \"rdma\"\n" +
        "    -p,  --password <string>           Password for realm server\n" +
        "    -n,  --no-echo                     Do not echo messages.\n" +
        "    -na, --no-advisory                 Do not create a subscriber for dataloss advisories\n" +
        "    -i,  --inline                      Put EventQueue in INLINE mode.\n" +
        "    -t,  --trace <level>               Set trace level\n" +
        "            where <level> can be:\n" +
        "                  off, severe, warn, info, debug, verbose\n" +
        "        --trustall\n" +
        "        --trustfile <file>\n" +
        "    -u,  --user <string>               User for realm server\n";


    public TibThruRecv(String[] args)
    {
        System.out.printf("#\n# %s\n#\n# %s\n#\n",
                          this.getClass().getName(),
                          FTL.getVersionInformation());        
        
        parseArgs(args);
    }
    
    public void usage()
    {
        System.out.printf(usageString);
        System.exit(0);
    }

    public void parseArgs(String args[])
    {
        int i           = 0;
        int argc        = args.length;
        String s        = null;
        TibAux aux      = new TibAux(args, usageString);

        for (i = 0; i < argc; i++)
        {
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
            s = aux.getString(i, "--password", "-p");
            if (s != null) {
                password = s;
                i++;
                continue;
            }
            s = aux.getString(i, "--durable", "-d");
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
            s = aux.getString(i, "--user", "-u");
            if (s != null) {
                user = s;
                i++;
                continue;
            }
            if (aux.getFlag(i, "--inline", "-i")) {
                inline_mode = true;
                continue;
            }
            if (aux.getFlag(i, "--no-echo", "-n")) {
                doEcho = false;
                continue;
            }
            if (aux.getFlag(i, "--no-advsiory", "-na")) {
                doAdvisory = false;
                continue;
            }
            if (args[i].startsWith("-", 0)) {
                System.out.println("invalid option: "+args[i]);
                usage();
            }
            realmServer = args[i];
        }

        System.out.print("Invoked as: TibThruRecv ");
        for (i = 0; i < argc; i++)
            System.out.print(args[i]+" ");
        System.out.println("");
    }

    public String getResult(long stop)
    {
        StringBuilder result = new StringBuilder();
        NumberFormat nf = new DecimalFormat(" ##0.00E00 ");
        double interval = (stop-start) * 1.0e-9;
        
        result.append("Received ");
        result.append(nf.format((double)count));
        result.append(" messages in ");
        result.append(nf.format(interval));
        result.append(" seconds. (");
        result.append(nf.format(count/interval));
        result.append(" messages per second)\n");

        result.append("Received ");
        result.append(nf.format((double)bytes));
        result.append(" bytes in ");
        result.append(nf.format(interval));
        result.append(" seconds. (");
        result.append(nf.format(bytes/interval));
        result.append(" bytes per second)\n");
        
        result.append("Messages per callback: ");
        result.append(blockSizeStats.toString(1.0));
        result.append("\n");
        
        return result.toString();
    }

    //convert bytes to a long - java order - requires at least 8 bytes
    public long convertToLong(byte[] bytes)
    {
        long retVal = 0;
        long i;
        int shift;
        int index;

        for(index = 0, shift = 0; index < 8; index++)
        {
            i = (long) bytes[index];
            if(i < 0) i = -(i ^ 0xff) - 1;

            retVal += (i << shift);
            shift += 8;
        }
        
        return retVal;
    }
    
    public void onMsg(List<Message> messages, EventQueue eventQueue)
    {
        int n = messages.size();//block count
        String result = null;
        
        try
        {
            for(int i = 0,msgNum = messages.size();  i < msgNum;  i++)
            {
                byte[] buf = messages.get(i).getOpaque(dataRef);
                int flag = (int) convertToLong(buf);
                switch(flag)
                {
                    case 0:
                        count++;
                        bytes += buf.length;
                        break;
                    case -1:
                        start = System.nanoTime();
                        count = 0;
                        bytes = 0;
                        blockSizeStats.setN(0);
                        n--;
                        break;
                    case -2:
                        result = getResult(System.nanoTime());
                        System.out.printf("%s\n",result); System.out.flush();
                        n--;
                        break;
                    case -3:
                        result = getResult(System.nanoTime());
                        System.out.printf("%s\n",result); System.out.flush();
    
                        //Format and send back the report to the sender
                        reportMessage.setString("receiverReport", result);
                        pub.send(reportMessage);
                        n--;
                        break;
                    case -4:
                        if (doEcho) // Send pong for initial ping.
                            pub.send(messages.get(i));
                        n--;
                        break;
                    default:
                        if (doEcho)
                            pub.send(messages.get(i));
                        count++;
                        bytes += buf.length;
                }
            }
        }
        catch(Exception exp)
        {
            exp.printStackTrace();
            running = false;
        }
        
        if(n != 0) blockSizeStats.StatUpdate(n);
    }

    public void onAdvisory(List<Message> messages, EventQueue eventQueue)
    {
        try
        {
            for (int i = 0,msgNum = messages.size(); i < msgNum; i++)
            {
                System.out.printf("received advisory\n");
                System.out.println(messages.get(i));
            }
        }
        catch(Exception exp)
        {
            // any exception that happens in the callback does not
            // propagate to the caller of the tibEventQueue_Dispatch
            exp.printStackTrace();
        }
    }

    public void recv() throws Exception
    {
        Realm realm   = null;
        TibProperties props   = null;
        EventQueue queue   = null;
        Subscriber subs    = null;
        Subscriber adv     = null;
        ContentMatcher am      = null;

        // Set global trace to specified level
        if (trcLevel != null)
            FTL.setLogLevel(trcLevel);

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
        reportMessage = realm.createMessage("ReportMsgFmt");
        
        props = FTL.createProperties();
        props.set(EventQueue.PROPERTY_BOOL_INLINE_MODE, inline_mode);

        queue = realm.createEventQueue(props);
        props.destroy();

        if (doAdvisory)
        {
            // Create a data loss advisory matcher
            String matcher = "{\""+Advisory.FIELD_NAME+"\":\""+Advisory.NAME_DATALOSS+"\"}";
            
            am = realm.createContentMatcher(matcher);
            // Create an advisory subscriber
            adv = realm.createSubscriber(Advisory.ENDPOINT_NAME, am);

            queue.addSubscriber(adv, new SubscriberListener()
            {
                public void messagesReceived(List<Message> messages, EventQueue eventQueue)
                {
                    onAdvisory(messages,eventQueue);
                }
            });
        }

        props = FTL.createProperties();
        if (durableName != null)
            props.set(Subscriber.PROPERTY_STRING_DURABLE_NAME, durableName);
        subs = realm.createSubscriber(recvEndpointName, null, props);
        props.destroy();
        
        queue.addSubscriber(subs, new SubscriberListener()
        {
            public void messagesReceived(List<Message> messages, EventQueue eventQueue)
            {
                onMsg(messages,eventQueue);
            }
        });

        pub = realm.createPublisher(endpointName);

        while(running)
        {
            try
            {
                queue.dispatch();
            }
            catch(Exception exp)
            {
                exp.printStackTrace();
            }
        }

        pub.close();
        
        if(queue!=null && subs!=null) queue.removeSubscriber(subs);
        if(queue!=null && adv!=null) queue.removeSubscriber(adv);
        
        subs.close();
        if(adv!=null) adv.close();
        if(am!=null) am.destroy();
        queue.destroy();
        dataRef.destroy();
        reportMessage.destroy();
        realm.close();
    }
    
    public static void main(String[] args)
    {
        TibThruRecv s  = new TibThruRecv(args);
        try
        {
            s.recv();
        }
        catch (Throwable e)
        {
            e.printStackTrace();
        }
    } 
}
