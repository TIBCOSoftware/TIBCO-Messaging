/*
 * Copyright (c) 2010-2017 TIBCO Software Inc.
 * Licensed under a BSD-style license. Refer to [LICENSE]
 * For more information, please contact:
 * TIBCO Software Inc., Palo Alto, California, USA
 */

/*
 * This is a sample of a basic java FTL monitoring subscriber program.
 */

package com.tibco.ftl.samples;

import java.util.*;

import com.tibco.ftl.*;

public class TibMonSub implements SubscriberListener
{
    String   realmServer    = null;
    String   defaultUrl     = "http://localhost:8080";
    String   defaultSecureUrl = "https://localhost:8080";
    String   match          = null;
    String   trcLevel       = null;
    String   user           = "guest";
    String   password       = "guest-pw";
    String   secondary      = null;
    boolean  trustAll       = false;
    String   trustFile      = null;
    String   clientLabel    = null;
    boolean  running        = true;

    String   usageString    =
        "TibMonSub [options] url\n" +
        " Default url is " + defaultUrl + "\n" +
        "  where options can be:\n" +
        "    -cl, --client-label <string>\n" +
        "    -h, --help\n" +
        "    -m, --match <content match>\n" +
        "           where <content match> is of the form:\n" +
        "                 '{\"fieldname\":<value>}'\n" +
        "           where <value> is:\n" +
        "                 \"string\" for a string field\n" +
        "                 a number such as 2147483647 for an integer field\n" +
        "    -p, --password <string>\n" +
        "    -s, --secondary <string>\n" +
        "    -t, --trace <level>\n" +
        "           where <level> can be:\n" +
        "                 off, severe, warn, info, debug, verbose\n" +
        "        --trustall\n" +
        "        --trustfile <file>\n" +
        "    -u, --user <string>\n";

    public TibMonSub(String[] args)
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
        int n           = 0;
        TibAux aux      = new TibAux(args, usageString);

        for (i = 0; i < argc; i++)
        {
            s = aux.getString(i, "--client-label", "-cl");
            if (s != null) {
                clientLabel = s;
                i++;
                continue;
            }
            if (aux.getFlag(i, "--help", "-h")) {
                usage();
            }
            s = aux.getString(i, "--match", "-m");
            if (s != null) {
                match = s;
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

        System.out.print("Invoked as: TibMonSub ");
        for (i = 0; i < argc; i++)
            System.out.print(args[i]+" ");
        System.out.println("");
    }

    public void processMessageFields(
        Message         msg,
        long            indent) throws FTLException
    {
        boolean addSeparator = false;
        MessageIterator it = msg.iterator();

        while (indent-- > 0)
        {
            System.out.print("    ");
        }
        while (it.hasNext())
        {
            MessageFieldRef ref = it.next();
            int fieldType = (int) msg.getFieldType(ref);
            switch (fieldType)
            {
                case Message.TIB_FIELD_TYPE_LONG_ARRAY:
                case Message.TIB_FIELD_TYPE_DOUBLE_ARRAY:
                case Message.TIB_FIELD_TYPE_STRING_ARRAY:
                case Message.TIB_FIELD_TYPE_MESSAGE_ARRAY:
                case Message.TIB_FIELD_TYPE_DATETIME_ARRAY:
                    continue;
                default:
                    break;
            }
            String fieldName = ref.getFieldName();
            if (addSeparator)
            {
                System.out.print(", ");
            }
            addSeparator = true;
            System.out.print(fieldName);
            System.out.print("=");
            switch (fieldType)
            {
                case Message.TIB_FIELD_TYPE_LONG:
                    System.out.format("%d", msg.getLong(ref));
                    break;
                case Message.TIB_FIELD_TYPE_STRING:
                    System.out.print(msg.getString(ref));
                    break;
                default:
                    System.out.print("Unsupported field type");
                    break;
            }
        }
        System.out.println();
    }

    public void processFTLMetricMessage(
        Message         msg) throws FTLException
    {
        Message[] metrics;

        if (!msg.isFieldSet(Monitoring.FIELD_METRICS))
        {
            return;
        }
        processMessageFields(msg, 0);
        metrics = msg.getMessageArray(Monitoring.FIELD_METRICS);
        for (Message metricMsg : metrics)
        {
            processMessageFields(metricMsg, 1);
        }
    }

    public void processFTLEventMessage(
        Message         msg) throws FTLException
    {
        processMessageFields(msg, 0);
    }

    public void messagesReceived(List<Message> messages, EventQueue eventQueue)
    {
        int i;
        int msgNum = messages.size();
         
        for (i = 0;  i < msgNum;  i++) 
        {
            try
            {
                Message msg = messages.get(i);
                if (msg.isFieldSet(Monitoring.FIELD_MSG_TYPE))
                {
                    long msgType = msg.getLong(Monitoring.FIELD_MSG_TYPE);
                    if (msgType == Monitoring.MessageType.METRICS)
                    {
                        System.out.print("Metrics: ");
                        processFTLMetricMessage(msg);
                    }
                    else if (msgType == Monitoring.MessageType.SERVER_METRICS)
                    {
                        System.out.print("Server metrics: ");
                        processFTLMetricMessage(msg);
                    }
                    else if (msgType == Monitoring.MessageType.EVENT)
                    {
                        System.out.print("Event: ");
                        processFTLEventMessage(msg);
                    }
                }
            }
            catch(FTLException exp)
            {
                running = false;
                exp.printStackTrace();
            }
        }
    }
    
    public int recv() throws FTLException
    {
        TibProperties   props;
        Realm           realm;
        ContentMatcher  cm = null;
        ContentMatcher  am;
        Subscriber      sub;
        Subscriber      adv;
        EventQueue      queue;
        
        // Set global trace to specified level
        if (trcLevel != null)
            FTL.setLogLevel(trcLevel);
        
        // Get a single realm per process
        props = FTL.createProperties();
        props.set(Realm.PROPERTY_STRING_USERNAME, user);
        props.set(Realm.PROPERTY_STRING_USERPASSWORD, password);
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

        realm = FTL.connectToRealmServer(realmServer, null, props);
        props.destroy();
        
        // Create a content matcher, if specified
        if (match != null)
            cm = realm.createContentMatcher(match);

        // Create a subscriber
        sub = realm.createSubscriber(Monitoring.ENDPOINT_NAME, cm, null);

        // Create a data loss advisory matcher
        am = realm.createContentMatcher("{\""+Advisory.FIELD_NAME+
                                        "\":\""+Advisory.NAME_DATALOSS+"\"}");

        // Create an advisory subscriber
        adv = realm.createSubscriber(Advisory.ENDPOINT_NAME, am);
        am.destroy();

        // Create a queue and add subscriber to it
        queue = realm.createEventQueue();
        queue.addSubscriber(sub, this);
        queue.addSubscriber(adv, new AdvisoryListener());
        
        // Begin dispatching messages
        System.out.printf("waiting for message(s)\n");
        while (running)
        {
            queue.dispatch();
        }

        // Clean up
        queue.removeSubscriber(sub);
        queue.removeSubscriber(adv);
        queue.destroy();
        sub.close();
        adv.close();
        if (cm != null)
            cm.destroy();
        realm.close();

        return 0;
    }
    
    public static void main(String[] args)
    {
        TibMonSub s  = new TibMonSub(args);
        try
        {
            s.recv();
        }
        catch (Throwable e)
        {
            e.printStackTrace();
        }
    } 

    private class AdvisoryListener implements SubscriberListener
    {
        public void messagesReceived(List<Message> messages, EventQueue eventQueue)
        {
            int msgNum = messages.size();

            for (int i = 0;  i < msgNum;  i++)
            {
                Message msg = messages.get(i);
                try
                {
                    System.out.println("advisory:");

                    System.out.println("Name: " + msg.getString(Advisory.FIELD_NAME));

                    System.out.println("    Severity: " +
                                       msg.getString(Advisory.FIELD_SEVERITY));
                    System.out.println("    Module: " +
                                       msg.getString(Advisory.FIELD_MODULE));
                    System.out.println("    Reason: " +
                                       msg.getString(Advisory.FIELD_REASON));
                    System.out.println("    Timestamp: " +
                                       msg.getDateTime(Advisory.FIELD_TIMESTAMP));
                    System.out.println("    Aggregation Count: " +
                                       msg.getLong(Advisory.FIELD_AGGREGATION_COUNT));
                    System.out.println("    Aggregation Time: " +
                                       msg.getDouble(Advisory.FIELD_AGGREGATION_TIME));

                    if (msg.isFieldSet(Advisory.FIELD_ENDPOINTS))
                    {
                        System.out.println("    Endpoints:");
                        String[] sarray = msg.getStringArray(Advisory.FIELD_ENDPOINTS);
                        for (int j = 0; j < sarray.length; j++)
                            System.out.println("      " + sarray[j]);
                    }

                    if (msg.isFieldSet(Advisory.FIELD_SUBSCRIBER_NAME))
                    {
                        System.out.println("  Subscriber Name: " +
                                           msg.getString(Advisory.FIELD_SUBSCRIBER_NAME));
                    }
                }
                catch(FTLException exp)
                {
                    exp.printStackTrace();
                }
            }
        }
    }
}
