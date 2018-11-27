/*
 * Copyright (c) 2010-2018 TIBCO Software Inc.
 * Licensed under a BSD-style license. Refer to [LICENSE]
 * For more information, please contact:
 * TIBCO Software Inc., Palo Alto, California, USA
 */

/*
 * This is a sample of a basic java FTL subscriber program which receives the
 * requested number of messages and then cleans up and exits.
 */

package com.tibco.ftl.samples;

import java.util.*;

import com.tibco.ftl.*;

public class TibRecvEx implements SubscriberListener
{
    int      count          = 1;
    int      recved         = 0;
    String   appName        = "tibrecv";
    String   realmServer    = null;
    String   defaultUrl     = "http://localhost:8080";
    String   defaultSecureUrl = "https://localhost:8080";
    String   match          = null;
    String   subEndpoint    = "tibrecv-endpoint";
    String   trcLevel       = null;
    String   user           = "guest";
    String   password       = "guest-pw";
    String   identifier     = null;
    String   durableName    = null;
    String   secondary      = null;
    boolean  explicitAck    = false;
    boolean  trustAll       = false;
    String   trustFile      = null;
    boolean  unsubscribe    = false;
    String   clientLabel    = null;

    // MessageFiledRef objects could also be used instead for faster access
    final String   myLong         = "My-Long";
    final String   myString       = "My-String";
    final String   myOpaque       = "My-Opaque";

    List<Message>   ackableMsgs = new LinkedList<Message>();
    
    String   usageString    =
        "TibRecvEx [options] url\n" +
        " Default url is " + defaultUrl + "\n" +
        "  where options can be:\n" +
        "    -a, --application <name>\n" +
        "    -cl, --client-label <string>\n" +
        "    -c, --count <int>\n" +
        "    -d, --durablename <name>\n" +
        "    -e, --endpoint <name>\n" +
        "    -h, --help\n" +
        "    -id,--identifier <string>\n" +
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
        "    -us, --unsubscribe\n" +
        "    -u, --user <string>\n" +
        "    -x, --explicitAck \n";

    public TibRecvEx(String[] args)
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
            s = aux.getString(i, "--application", "-a");
            if (s != null) {
                appName = s;
                i++;
                continue;
            }
            s = aux.getString(i, "--durablename", "-d");
            if (s != null) {
                durableName = s;
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
                subEndpoint = s;
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
            if (aux.getFlag(i, "--explicitAck", "-x")) {
                explicitAck = true;
                continue;
            }
            if (aux.getFlag(i, "--unsubscribe", "-us")) {
                unsubscribe = true;
                continue;
            }
            if (args[i].startsWith("-", 0)) {
                System.out.println("invalid option: "+args[i]);
                usage();
            }
            realmServer = args[i];
        }

        System.out.print("Invoked as: TibRecvEx ");
        for (i = 0; i < argc; i++)
            System.out.print(args[i]+" ");
        System.out.println("");
    }

    private void printOpaque(byte []opaque)
    {
        String outputString;

        System.out.print(" type opaque:  size " + opaque.length);

        String s = new String(opaque);
        
        if (s.length() > 60)
            outputString = s.substring(0, 60) + "<truncated>";
        else
            outputString = s;
        
         System.out.print(" data: " + outputString);
         if (s != outputString)
             System.out.print("<truncated>");
         System.out.println("\n");
    }
    
    public void printMessage(
        Message         msg) throws FTLException
    {
        int fieldCount = 0;
        System.out.println("message:");
        
        if (msg.isFieldSet(myLong))
        {
            System.out.println(" type long: " + msg.getLong(myLong));
            fieldCount++;
        }
        if(msg.isFieldSet(myString))
        {
            System.out.println(" type string: " + msg.getString(myString));
            fieldCount++;
        }
        if (msg.isFieldSet(myOpaque))
        {
            byte[] opaque = msg.getOpaque(myOpaque);
            printOpaque(opaque);
            fieldCount++;
        }
        
        if (fieldCount != 3)
        {
            System.out.println(" " + msg);
        }        
    }

    public void messagesReceived(List<Message> messages, EventQueue eventQueue)
    {
        int i;
        int msgNum = messages.size();
         
        for (i = 0;  i < msgNum;  i++) 
        {
            try
            {
                //
                // Do not destroy message as it is owned by the library.  If
                // you need to pass the messages to something outside this
                // callback, use Message.mutableCopy() and pass the copies.
                // 
                recved++;
                System.out.printf("received message %d \n", recved);
                printMessage(messages.get(i));               

                if (explicitAck)
                    ackableMsgs.add(messages.get(i).mutableCopy());
            }
            catch(FTLException exp)
            {
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
        
        // Create a content matcher, if specified
        if (match != null)
            cm = realm.createContentMatcher(match);

        props = null;
        props = FTL.createProperties();
        
        if (explicitAck)
            props.set(Subscriber.PROPERTY_BOOL_EXPLICIT_ACK, explicitAck);

        if (durableName != null)
            props.set(Subscriber.PROPERTY_STRING_DURABLE_NAME, durableName);

        // Create a subscriber
        sub = realm.createSubscriber(subEndpoint, cm, props);

        // Create a data loss advisory matcher
        am = realm.createContentMatcher("{\""+Advisory.FIELD_NAME+
                                        "\":\""+Advisory.NAME_DATALOSS+"\"}");

        // Create an advisory subscriber
        adv = realm.createSubscriber(Advisory.ENDPOINT_NAME, am);
        am.destroy();

        // Create a queue and add subscriber to it
        props = FTL.createProperties();
        queue = realm.createEventQueue(props);
        props.set(EventQueue.PROPERTY_INT_DISCARD_POLICY, EventQueue.DISCARD_NEW);
        props.set(EventQueue.PROPERTY_INT_DISCARD_POLICY_MAX_EVENTS, 1000);
        queue.addSubscriber(sub, this);
        queue.addSubscriber(adv, new AdvisoryListener());
        props.destroy();
        props = null;
        
        // Begin dispatching messages
        System.out.printf("waiting for message(s)\n");
        while (recved < count)
        {
            queue.dispatch();
            while (ackableMsgs.size() > 0)
            {
                Message msg = ackableMsgs.remove(0);
                msg.acknowledge();
                msg.destroy();
            }
        }

        // Clean up
        queue.removeSubscriber(sub);
        queue.removeSubscriber(adv);
        queue.destroy();
        sub.close();
        adv.close();
        if (cm != null)
            cm.destroy();
        if (unsubscribe && durableName != null)
            realm.unsubscribe(subEndpoint, durableName);
        realm.close();

        return 0;
    }
    
    public static void main(String[] args)
    {
        TibRecvEx s  = new TibRecvEx(args);
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
