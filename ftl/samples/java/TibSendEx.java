/*
 * Copyright (c) 2010-2017 TIBCO Software Inc.
 * Licensed under a BSD-style license. Refer to [LICENSE]
 * For more information, please contact:
 * TIBCO Software Inc., Palo Alto, California, USA
 */

/*
 * This is a sample of a basic java FTL publisher program which sends the
 * requested number of messages at the desired delay between the sends.
 * It uses a timer for the delays.
 */

package com.tibco.ftl.samples;

import com.tibco.ftl.*;
import com.tibco.ftl.exceptions.*;

public class TibSendEx implements EventTimerListener, NotificationHandler
{
    int             count       = 1;
    int             delay       = 1000;
    String          appName     = "tibsend";
    String          realmServer = null;
    String          defaultUrl  =  "http://localhost:8080";
    String          defaultSecureUrl = "https://localhost:8080";
    String          pubName     = "tibsend-endpoint";
    String          formatName  = "Format-1";
    String          trcLevel    = null;
    String          user        = "guest";
    String          password    = "guest-pw";
    String          identifier  = null;
    String          secondary   = null;
    boolean         keyedOpaque = false;
    boolean         trustAll    = false;
    String          trustFile   = null;
    String          clientLabel = null;
    boolean         finished    = false;
    Realm           realm       = null;
    Publisher       pub         = null;
    Message         msg         = null;
    int             sent        = 0;

    // MessageFiledRef objects could also be used instead for faster access
    final String    myLong      = "My-Long";
    final String    myString   = "My-String";
    final String    myOpaque   = "My-Opaque";
    final String    myKey      = "_key";
    final String    myData     = "_data";
    
    String          usageString =
        "TibSendEx [options] url\n" +
        "Default url is " + defaultUrl + "\n" +
        "  where options can be:\n" +
        "    -a, --application <name>\n" +
        "    -cl, --client-label <string>\n" +
        "    -c, --count <int>\n" +
        "    -d, --delay <int> (in millis)\n" +
        "    -e, --endpoint <name>\n" +
        "    -f, --format <name>\n" +
        "    -h, --help\n" +
        "    -id, --identifier <string>\n" +
        "    -ko, --keyedopaque\n" +
        "    -p, --password <string>\n" +
        "    -s, --secondary <string>\n" +
        "    -t, --trace <level>\n" +
        "           where <level> can be:\n" +
        "                 off, severe, warn, info, debug, verbose\n" +
        "        --trustall\n" +
        "        --trustfile <file>\n" +
        "    -u, --user <string>\n";

    public TibSendEx(String[] args)
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
            s = aux.getString(i, "--client-label", "-cl");
            if (s != null) {
                clientLabel = s;
                i++;
                continue;
            }
            n = aux.getInt(i, "--count", "-c");
            if (n > -1) {
                count = n;
                i++;
                continue;
            }
            n = aux.getInt(i, "--delay", "-d");
            if (n > -1) {
                delay = n;
                i++;
                continue;
            }
            s = aux.getString(i, "--endpoint", "-e");
            if (s != null) {
                pubName = s;
                i++;
                continue;
            }
            s = aux.getString(i, "--format", "-f");
            if (s != null)
            {
                formatName = s;
                i++;
                continue;
            }
            if (aux.getFlag(i, "--help", "-h")) {
                usage();
            }
            if (aux.getFlag(i, "--keyedopaque", "-ko")) {
                keyedOpaque = true;
                continue;
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
            if (args[i].startsWith("-", 0)) {
                System.out.println("invalid option: "+args[i]);
                usage();
            }
            realmServer = args[i];
        }

        if (keyedOpaque) {
            formatName = Message.TIB_BUILTIN_MSG_FMT_KEYED_OPAQUE;
        }
            

        System.out.print("Invoked as: TibSendEx ");
        for (i = 0; i < argc; i++)
            System.out.print(args[i]+" ");
        System.out.println("");
    }

    public void timerFired(EventTimer timer,EventQueue eventQueue)
    {
        byte[]          buf = "opaque".getBytes();
        
        try
        {
            msg.clearAllFields();

            if (keyedOpaque) {
                msg.setString(myKey, Integer.toString((sent++ % 5)));
                msg.setOpaque(myData, buf);
            }
            else {
                msg.setLong(myLong, ++sent);
                msg.setString(myString, pubName);
                msg.setOpaque(myOpaque, buf);
            }
            
            System.out.printf("sending message %d\n", sent);
        
            pub.send(msg);
        }
        catch (FTLException ex)
        {
            System.err.println("caught exception: "+ex.getMessage());
            ex.printStackTrace();
            System.exit(0);
        }

        if (sent == count)
            finished = true;
    }
    
    public void onNotification(int type, String reason)
    {
        if (type == NotificationHandler.CLIENT_DISABLED)
        {
            System.out.println("application administratively disabled: " + reason);
            System.out.println("exiting");
            System.exit(1);
        }
        else
        {
            System.out.println("notification type " + type + ": " + reason);
        }
    }

    public int send() throws FTLException
    {
        TibProperties   props;
        EventQueue      queue;
        EventTimer      timer;
        
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

        // Set this object as the NotificationHandler to receive notifications
        realm.setNotificationHandler(this);

        // Create sender object and message to be sent
        pub = realm.createPublisher(pubName);
        msg = realm.createMessage(formatName);

        // Start a timer to do the sends
        queue = realm.createEventQueue();
        timer = queue.createTimer((1.0*delay)/1000.0, this);

        try
        {
            while (!finished)
                queue.dispatch();
        }
        catch (FTLClientShutdownException ftle)
        {
            // Something unusual happened and the Realm was shutdown. Give the
            // notification handler a chance to run so that we can log if this
            // is an administrative action.
            try
            {
                Thread.sleep(1000);
            }
            catch (InterruptedException ie)
            {
                // ignore
            }

            ftle.printStackTrace();
        }

        // Clean up
        msg.destroy();
        queue.destroyTimer(timer);
        queue.destroy();
        pub.close();
        realm.close();

        return 0;
    }
    
    public static void main(String[] args)
    {
        TibSendEx s  = new TibSendEx(args);
        try
        {
            s.send();
        }
        catch (Throwable e)
        {
            e.printStackTrace();
        }
    }
}
