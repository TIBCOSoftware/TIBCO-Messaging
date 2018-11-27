/*
 * Copyright (c) 2010-2017 TIBCO Software Inc.
 * Licensed under a BSD-style license. Refer to [LICENSE]
 * For more information, please contact:
 * TIBCO Software Inc., Palo Alto, California, USA
 */

/*
 * This is a sample of a basic java program which demonstrates the TibMap API that
 * sets the requested number of keys with corresponding values in the 'tibmap' at the persistence server.
 */

package com.tibco.ftl.samples;

import java.util.*;
import com.tibco.ftl.*;
import com.tibco.ftl.exceptions.*;

public class TibMapSet implements NotificationHandler
{
    String          appName     = "tibmapset";
    String          realmServer = "http://localhost:8080";
    String          mapEndpoint = "tibmapset-endpoint";
    String          formatName  = "Format-1";
    String          trcLevel    = null;
    String          user        = "guest";
    String          password    = "guest-pw";
    String          identifier  = null;
    String          secondary   = null;
    String          mapName     = "tibmap";
    static final String DEFAULT_URL = "http://localhost:8080";
    static final String DEFAULT_SECURE_URL = "https://localhost:8080";
    String          keyList     = "key1,key2,key3";
    String          clientLabel = null;

    Realm           realm       = null;
    TibMap          map         = null;
    Message         msg         = null;
    int             count       = 0;
    boolean         trustAll    = false;
    String          trustFile   = null;
    

    String          usageString =
        "TibMapSet [options] url\n" +
        "Default url is " + realmServer + "\n" +
        "  where options can be:\n" +
        "    -a, --application <name>\n" +
        "    -c, --count <int>\n" +
        "    -cl, --client-label <string>\n" +
        "    -e, --endpoint <name>\n" +
        "    -f, --format <name>\n" +
        "    -h, --help\n" +
        "    -id, --identifier <string>\n" +
        "    -l,  --keyList <string>[,<string>]\n" +
        "    -n,  --mapname <name>\n" +
        "    -p, --password <string>\n" +
        "    -s, --secondary <string>\n" +
        "    -t, --trace <level>\n" +
        "           where <level> can be:\n" +
        "                 off, severe, warn, info, debug, verbose\n" +
        "        --trustall\n" +
        "        --trustfile <file>\n" +
        "    -u, --user <string>\n";

    public TibMapSet(String[] args)
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
            s = aux.getString(i, "--endpoint", "-e");
            if (s != null) {
                mapEndpoint = s;
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
            s = aux.getString(i, "--identifier", "-id");
            if (s != null) {
                identifier = s;
                i++;
                continue;
            }
            s = aux.getString(i, "--keyList", "-l");
            if (s != null) {
                keyList = s;
                i++;
                continue;
            }
            s = aux.getString(i, "--mapname", "-n");
            if (s != null) {
                mapName = s;
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

        System.out.print("Invoked as: TibMapSet ");
        for (i = 0; i < argc; i++)
            System.out.print(args[i]+" ");
        System.out.println("");
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

    public void initMessage(String key) throws FTLException
    {
        // Create sender object and message to be sent
        if (msg == null)
            msg = realm.createMessage(formatName);
        else
            msg.clearAllFields();

        msg.setLong("My-Long", ++count);
    }

    public int work() throws FTLException
    {
        TibProperties   props;
        
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

        if (trustAll == true || trustFile != null)
        {
            if (realmServer == null)
                realmServer = TibMapSet.DEFAULT_SECURE_URL;

            if (trustAll)
                props.set(Realm.PROPERTY_LONG_TRUST_TYPE, Realm.HTTPS_CONNECTION_TRUST_EVERYONE);
            else
            {
                props.set(Realm.PROPERTY_LONG_TRUST_TYPE, Realm.HTTPS_CONNECTION_USE_SPECIFIED_TRUST_FILE);
                props.set(Realm.PROPERTY_STRING_TRUST_FILE, trustFile);
            }
        }
        else
        {
            if (realmServer == null)
                realmServer = TibMapSet.DEFAULT_URL;
        }

        if (clientLabel != null)
            props.set(Realm.PROPERTY_STRING_CLIENT_LABEL, clientLabel);
        else
            props.set(Realm.PROPERTY_STRING_CLIENT_LABEL, this.getClass().getSimpleName());

        realm = FTL.connectToRealmServer(realmServer, appName, props);
        props.destroy();

        // Set this object as the NotificationHandler to receive notifications
        realm.setNotificationHandler(this);
        map = realm.createMap(mapEndpoint, mapName, null);

        StringTokenizer tok = new StringTokenizer(keyList);
        try
        {
            while(tok.hasMoreElements())
            {
                String key = (String)tok.nextToken(",");
                System.out.println("setting value for key " + key);
                initMessage(key);

                map.set(key, msg);
            }
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
        map.close();
        realm.close();

        return 0;
    }
    
    public static void main(String[] args)
    {
        TibMapSet s  = new TibMapSet(args);
        try
        {
            s.work();
        }
        catch (Throwable e)
        {
            e.printStackTrace();
        }
    }
}
