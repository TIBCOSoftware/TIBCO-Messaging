/*
 * Copyright (c) 2010-2017 TIBCO Software Inc.
 * Licensed under a BSD-style license. Refer to [LICENSE]
 * For more information, please contact:
 * TIBCO Software Inc., Palo Alto, California, USA
 */

/*
 * This is a sample Java request program which is used with the TibReply program
 * to demonstrate the basic use of inboxes.
 */

package com.tibco.ftl.samples;

import com.tibco.ftl.*;

import java.util.*;

public class TibRequest implements SubscriberListener, EventTimerListener
{
    String          appName       = "tibrequest";
    String          realmServer   = "http://localhost:8080";
    String          endpointName  = "tibrequest-endpoint";
    String          trcLevel      = null;
    String          user          = "guest";
    String          password      = "guest-pw";
    String          identifier    = null;
    String          secondary     = null;
    String          clientLabel   = null;

    double          pingInterval  = 1.0;
    volatile EventTimer pongTimer = null;
    Message         msg;
    Publisher       pub;

    boolean         replyReceived = false;

    String          usageString =
        "TibRequest [options] url\n" +
        "Default url is " + realmServer + "\n" +
        "  where options can be:\n" +
        "    -a, --application <name>\n" +
        "    -cl, --client-label <string>\n" +
        "    -e, --endpoint <name>\n" +
        "    -h, --help\n" +
        "    -id,--identifier <string>\n" +
        "    -p, --password <string>\n" +
        "        --ping <float> (in seconds)\n" +
        "    -s, --secondary <string>\n" +
        "    -t, --trace <level>\n" +
        "           where <level> can be:\n" +
        "                 off, severe, warn, info, debug, verbose\n" +
        "    -u, --user <string>\n";

    public TibRequest(String[] args)
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
        double d        = 0;
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
            s = aux.getString(i, "--endpoint", "-e");
            if (s != null) {
                endpointName = s;
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
            d = aux.getDouble(i, "--ping", "--ping");
            if (d > -1.0) {
                pingInterval = d;
                i++;
                continue;
            }
            s = aux.getString(i, "--secondary", "-s");
            if (s != null) {
                secondary = s;
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

        System.out.print("Invoked as: TibRequest ");
        for (i = 0; i < argc; i++)
            System.out.print(args[i]+" ");
        System.out.println("");
    }

    public void messagesReceived(List<Message> messages, EventQueue eventQueue)
    {
        try
        {
            if (pongTimer != null)
            {
                eventQueue.destroyTimer(pongTimer);
                pongTimer = null;
            }
        }
        catch (FTLException e)
        {
            e.printStackTrace();
        }
        replyReceived = true;
        System.out.printf("received reply message:\n");
        System.out.println("  " + messages.get(0));
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
            replyReceived = true;
        }
    }

    public int request() throws FTLException
    {
        TibProperties   props;
        Realm           realm;
        EventQueue      queue;
        InboxSubscriber sub;
        Inbox           inbox;

        // Set global trace to specified level
        if (trcLevel != null)
            FTL.setLogLevel(trcLevel);

        // Get a single realm per process
        props = FTL.createProperties();
        props.set(Realm.PROPERTY_STRING_USERNAME, user);
        props.set(Realm.PROPERTY_STRING_USERPASSWORD, password);
        if (identifier != null)
            props.set(Realm.PROPERTY_STRING_APPINSTANCE_IDENTIFIER, identifier);
        if (secondary != null)
            props.set(Realm.PROPERTY_STRING_SECONDARY_SERVER, secondary);

        if (clientLabel != null)
            props.set(Realm.PROPERTY_STRING_CLIENT_LABEL, clientLabel);
        else
            props.set(Realm.PROPERTY_STRING_CLIENT_LABEL, this.getClass().getSimpleName());

        realm = FTL.connectToRealmServer(realmServer, appName, props);
        props.destroy();

        sub = realm.createInboxSubscriber(endpointName);
        inbox = sub.getInbox();
        queue = realm.createEventQueue();
        queue.addSubscriber(sub, this);

        pub = realm.createPublisher(endpointName);
        msg = realm.createMessage("Format-1");

        // set by name since performance is not demonstrated here
        msg.setString("My-String", "request");
        msg.setLong("My-Long", 10);

        byte[] opaqueBytes = (new String("payload")).getBytes(); 
        msg.setOpaque("My-Opaque", opaqueBytes);

        // put our inbox in the request message
        // set by name since performance is not demonstrated here
        msg.setInbox("My-Inbox", inbox);

        // Should we retry the initial ping until we get a pong?
        if (pingInterval != 0)
            pongTimer = queue.createTimer(pingInterval, this);

        System.out.printf("sending request message:\n");
        System.out.println("  "  + msg);

        // send the request message
        pub.send(msg);

        while (!replyReceived)
            queue.dispatch();

        // Clean up
        queue.removeSubscriber(sub);
        if (pongTimer != null)
            queue.destroyTimer(pongTimer);
        queue.destroy();
        sub.close();
        pub.close();
        msg.destroy();
        realm.close();

        return 0;
    }
    
    public static void main(String[] args)
    {
        TibRequest s  = new TibRequest(args);
        try
        {
            s.request();
        }
        catch (Throwable e)
        {
            e.printStackTrace();
        }
    }
}
