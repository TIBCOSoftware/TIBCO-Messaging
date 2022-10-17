/*
 * Copyright (c) 2010-2017 TIBCO Software Inc.
 * Licensed under a BSD-style license. Refer to [LICENSE]
 * For more information, please contact:
 * TIBCO Software Inc., Palo Alto, California, USA
 */

/*
 * This is a sample java responder program which is used with the TibLatSend
 * program to measure basic latency between the publisher and subscriber.
 */

package com.tibco.ftl.samples;

import java.util.*;
import com.tibco.ftl.*;

public class TibLatRecv implements SubscriberListener
{
    String   appName        = "tiblatrecv";
    String   realmServer    = "http://localhost:8080";
    String   endpointName   = "tiblatrecv-sendendpoint";
    String   recvEndpointName = "tiblatrecv-recvendpoint";
    String   durableName    = null;
    String   trcLevel       = null;
    String   user           = "guest";
    String   password       = "guest-pw";
    String   identifier     = null;
    String   clientLabel    = null;

    String   usageString    =
        "Usage: TibLatRecv [options] url\n" +
        "Default url is " + realmServer + "\n" +
        "  where options can be:\n" +
        "    -a,  --application <name>          Application name\n" +
        "    -cl, --client-label <string>       Set client label\n" +
        "    -d,  --durableName <name>          Durable name for receive side.\n" +
        "    -et, --tx-endpoint <name>          Transmit endpoint name\n" +
        "    -er, --rx-endpoint <name>          Receive endpoint name\n" +
        "    -h,  --help                        Print this help\n" +
        "    -id, --identifier <string>         Choose instance, eg, \"rdma\"\n" +
        "    -p,  --password <string>           Password for realm server\n" +
        "    -t,  --trace <level>               Set trace level\n" +
        "            where <level> can be:\n" +
        "                  off, severe, warn, info, debug, verbose\n" +
        "    -u,  --user <string>               User for realm server\n";

    Publisher pub           = null;
    boolean   running       = true;

    public TibLatRecv(String[] args)
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
            if (args[i].startsWith("-", 0)) {
                System.out.println("invalid option: "+args[i]);
                usage();
            }
            realmServer = args[i];
        }

        System.out.print("Invoked as: TibLatRecv ");
        for (i = 0; i < argc; i++)
            System.out.print(args[i]+" ");
        System.out.println("");
    }

    public void messagesReceived(List<Message> messages, EventQueue eventQueue)
    {
        int msgNum = messages.size();

        if (msgNum != 1)
        {
            System.out.println("Received "+msgNum+" messages instead of one.");
            System.exit(1);
        }

        try
        {
            pub.send(messages.get(0));
        }
        catch (FTLException e)
        {
            e.printStackTrace();
            running = false;
        }
    }

    public int recv() throws FTLException
    {
        TibProperties   props;
        Realm           realm;
        Subscriber      sub;
        EventQueue      queue;
        
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
        sub = realm.createSubscriber(recvEndpointName, null, props);
        queue.addSubscriber(sub, this);
        props.destroy();

        // Create a publisher
        pub = realm.createPublisher(endpointName);

        // Begin dispatching messages
        while (running)
            queue.dispatch();

        // Clean up
        queue.removeSubscriber(sub);
        queue.destroy();
        sub.close();
        realm.close();

        return 0;
    }
    
    public static void main(String[] args)
    {
        TibLatRecv s  = new TibLatRecv(args);
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
