/*
 * Copyright (c) 2010-2017 TIBCO Software Inc.
 * Licensed under a BSD-style license. Refer to [LICENSE]
 * For more information, please contact:
 * TIBCO Software Inc., Palo Alto, California, USA
 */

/*
 * This is a sample Java replier program which is used with the TibRequest program
 * to demonstrate the basic use of inboxes.
 */

package com.tibco.ftl.samples;

import java.util.*;
import com.tibco.ftl.*;

public class TibReply implements SubscriberListener
{
    int       count          = 1;
    int       received       = 0;
    String    appName        = "tibreply";
    String    realmServer    = "http://localhost:8080";
    String    endpointName   = "tibreply-endpoint";
    String    trcLevel       = null;
    String    user           = "guest";
    String    password       = "guest-pw";
    String    identifier     = null;
    String    secondary      = null;
    String    clientLabel    = null;

    Publisher pub;
    Message   replyMsg;

    String   usageString    =
        "TibReply [options] url\n" +
        "Default url is " + realmServer + "\n" +
        "  where options can be:\n" +
        "    -a, --application <name>\n" +
        "    -cl, --client-label <string>\n" +
        "    -c, --count <int>\n" +
        "    -e, --endpoint <name>\n" +
        "    -h, --help\n" +
        "    -id,--identifier <string>\n" +
        "    -p, --password <string>\n" +
        "    -s, --secondary <string>\n" +
        "    -t, --trace <level>\n" +
        "           where <level> can be:\n" +
        "                 off, severe, warn, info, debug, verbose\n" +
        "    -u, --user <string>\n";

    public TibReply(String[] args)
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

        System.out.print("Invoked as: TibReply ");
        for (i = 0; i < argc; i++)
            System.out.print(args[i]+" ");
        System.out.println("");
    }

    public void messagesReceived(List<Message> messages, EventQueue eventQueue)
    {
        Inbox   inbox = null;
        Message msg;
        String  fieldName;
        int     fieldType;
        MessageIterator iter;

        try
        {
            received++;
            System.out.printf("received request %d \n", received);
            msg = messages.get(0);
    
            // use a message iterator to print fields that have been set
            // in the message
            iter = msg.iterator();
            while (iter.hasNext())
            {
                // retrieve the field reference from the iterator to access
                // fields.
                MessageFieldRef ref = (MessageFieldRef)iter.next();
                fieldName = ref.getFieldName();
                fieldType = msg.getFieldType(fieldName);
                
                System.out.println("Name: " + fieldName);
                System.out.println("    Type: " + 
                                   msg.getFieldTypeString(fieldType));
                System.out.print  ("    Value:  ");
                
                switch (fieldType)
                {
                    case Message.TIB_FIELD_TYPE_LONG:
                        System.out.println(msg.getLong(ref));
                        break;
                    case Message.TIB_FIELD_TYPE_STRING:
                        System.out.println(msg.getString(ref));
                        break;
                    case Message.TIB_FIELD_TYPE_OPAQUE:
                        System.out.println("opaque, " +
                                           msg.getOpaque(ref).length +
                                           " bytes");
                        break;
                    case Message.TIB_FIELD_TYPE_INBOX:
                        inbox = msg.getInbox(ref);
                        System.out.println("(inbox)");
                        break;
                    default:
                        System.out.println("Unhandled type");
                        break;
                }
            }
            iter.destroy();

            // set by name since performance is not demonstrated here
            replyMsg.setString("My-String", "reply");

            System.out.printf("sending reply # %d \n", received);
            pub.sendToInbox(inbox, replyMsg);
        }
        catch(FTLException exp)
        {
            exp.printStackTrace();
        }
    }

    public int reply() throws FTLException
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
        if (secondary != null)
            props.set(Realm.PROPERTY_STRING_SECONDARY_SERVER, secondary);

        if (clientLabel != null)
            props.set(Realm.PROPERTY_STRING_CLIENT_LABEL, clientLabel);
        else
            props.set(Realm.PROPERTY_STRING_CLIENT_LABEL, this.getClass().getSimpleName());

        realm = FTL.connectToRealmServer(realmServer, appName, props);
        props.destroy();

        sub = realm.createSubscriber(endpointName, null);
        pub = realm.createPublisher(endpointName);

        // Create a queue and add subscriber to it
        queue = realm.createEventQueue();
        queue.addSubscriber(sub, this);
        
        replyMsg = realm.createMessage("Format-1");
        System.out.printf("waiting for request(s)\n");
        while (received < count)
            queue.dispatch();

        // Clean up
        queue.removeSubscriber(sub);
        queue.destroy();
        sub.close();
        pub.close();
        replyMsg.destroy();
        realm.close();

        return 0;
    }
    
    public static void main(String[] args)
    {
        TibReply s  = new TibReply(args);
        try
        {
            s.reply();
        }
        catch (Throwable e)
        {
            e.printStackTrace();
        }
    } 
}
