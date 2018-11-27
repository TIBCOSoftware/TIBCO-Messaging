/*
 * Copyright (c) 2010-2017 TIBCO Software Inc.
 * Licensed under a BSD-style license. Refer to [LICENSE]
 * For more information, please contact:
 * TIBCO Software Inc., Palo Alto, California, USA
 */

/*
 * This is a sample of a basic java program which demonstrates the TibMap API that
 * gets the requested number of keys and their values that are stored in 'tibmap' at the 
 * persistence server. If keyList is not specified then it iterates over all the keys in 
 * the tibmap.
 */

package com.tibco.ftl.samples;

import java.util.*;
import com.tibco.ftl.*;
import com.tibco.ftl.exceptions.*;

public class TibMapGet implements NotificationHandler
{
    String          appName     = "tibmapget";
    String          realmServer = null;
    String          mapEndpoint = "tibmapget-endpoint";
    String          trcLevel    = null;
    String          user        = "guest";
    String          password    = "guest-pw";
    String          identifier  = null;
    String          secondary   = null;
    String          mapName     = "tibmap";
    static final String DEFAULT_URL = "http://localhost:8080";
    static final String DEFAULT_SECURE_URL = "https://localhost:8080";
    String          keyList     = null;
    String          clientLabel = null;

    Realm           realm       = null;
    TibMap          map         = null;
    boolean         trustAll    = false;
    String          trustFile   = null;
    boolean         removeMap   = false;

    String          usageString =
        "TibMapGet [options] url\n" +
        "Default url is " + TibMapGet.DEFAULT_URL + "\n" +
        "  where options can be:\n" +
        "    -a, --application <name>\n" +
        "    -cl, --client-label <string>\n" +
        "    -e, --endpoint <name>\n" +
        "    -h, --help\n" +
        "    -id, --identifier <string>\n" +
        "    -l, --keyList <string>[,<string>]\n" +
        "    -n, --mapname <name>\n" +
        "    -p, --password <string>\n" +
        "    -r  --removemap \n" +
        "            the mapname command line arg needs to be specified as well\n" +
        "    -s, --secondary <string>\n" +
        "    -t, --trace <level>\n" +
        "           where <level> can be:\n" +
        "                 off, severe, warn, info, debug, verbose\n" +
        "        --trustall\n" +
        "        --trustfile <file>\n" +
        "    -u, --user <string>\n";

    public TibMapGet(String[] args)
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
            s = aux.getString(i, "--endpoint", "-e");
            if (s != null) {
                mapEndpoint = s;
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
            if (aux.getFlag(i, "--removemap", "-r")) {
                removeMap = true;
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

        System.out.print("Invoked as: TibMapGet ");
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
                realmServer = TibMapGet.DEFAULT_SECURE_URL;

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
                realmServer = TibMapGet.DEFAULT_URL;
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

        if (keyList != null)
        {
            StringTokenizer tok = new StringTokenizer(keyList, ",");
            try
            {
                while(tok.hasMoreElements())
                {
                    String key = (String)tok.nextElement();
                    
                    System.out.println("getting value for key " + key);
                    Message msg = map.get(key);
                    if (msg != null)
                        System.out.println("key = " + key + ", value = " + msg);
                    else
                        System.out.println("currently no key '" + key + "' in the map");
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
        }
        else
        {
            Message msg = null;
            String  key = null;
            TibMapIterator iter;

            System.out.println("iterating keys is the map");

            try 
            {
                iter = map.createIterator(null);
                while(iter.next())
                {
                    key = iter.currentKey();
                    msg = iter.currentValue();
 
                    System.out.println("key = " + key + " message = " + msg);
                }
                iter.destroy();
            }
            catch (FTLException  e)
            {
                e.printStackTrace(); 
            }
        }

        // Clean up
        map.close();

        // if removeMap is true and mapName is specified then remove the map.
        if ((mapName != null) && removeMap)
        {
            System.out.println("Removing map '" + mapName + "'");
            realm.removeMap(mapEndpoint, mapName, null);
        }

        realm.close();

        return 0;
    }
    
    public static void main(String[] args)
    {
        TibMapGet s  = new TibMapGet(args);
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
