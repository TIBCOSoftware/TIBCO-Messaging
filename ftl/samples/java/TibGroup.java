/*
 * Copyright (c) 2011-2017 TIBCO Software Inc.
 * Licensed under a BSD-style license. Refer to [LICENSE]
 * For more information, please contact:
 * TIBCO Software Inc., Palo Alto, California, USA
 */

/*
 * This is a sample of a basic java FTL GROUP membership program which joins a group
 * and displays changes to the group membership status for a given amount of time.
 * After that, it leaves the group, cleans up and exits.
 */

package com.tibco.ftl.samples;

import java.util.*;

import com.tibco.ftl.*;
import com.tibco.ftl.group.*;
import com.tibco.ftl.exceptions.*;

public class TibGroup
{
    
    String   appName        = "tibrecv";
    String   realmServer    = "http://localhost:8080";
    String   trcLevel       = null;
    String   user           = "guest";
    String   password       = "guest-pw";
    String   identifier     = null;
    String   secondary      = null;
    String   groupName      = "group";
    double   duration       = 10;
    double   activation     = 0;
    boolean  hasDescriptor       = false;
    boolean  observerOnly            = false;
    String   memberDescriberString  = null;
    String   clientLabel    = null;
    String   usageString    =
        "Usage: TibGroup [options] url\n" +
        "Default url is " + realmServer + "\n" +
        "  where options can be:\n" +
        "    -a, --application <name>\n" +
        "    -cl, --client-label <string>\n" +
        "    -h, --help\n" +
        "    -id,--identifier <string>\n" +
        "    -s, --secondary <string>\n" +
        "    -t, --trace <level>\n" +
        "    -u, --user <string>\n" +
        "    -g, --group <string>\n" +
        "    -d, --duration <seconds>\n" +
        "    -i, --interval <seconds>\n" +
        "    -md, --memberDescriptor <string> - the string that identifies this member to other members,\n" +
        "    *                    If the string value is not provided then this member is anonymous\n" +
        "    -o,  --observerOnly - this member is joining as observer, thus it doesn't receive an ordinal and doesn't generate up/down events\n" +
        "  where --trace can be:\n" +
        "    off, severe, warn, info, debug, verbose\n";
    
    public TibGroup(String[] args)
    {
        System.out.printf("#\n# %s\n#\n# (FTL) %s\n# (FTL GROUP) %s\n#\n",
                          this.getClass().getName(),
                          FTL.getVersionInformation(),
                          GroupFactory.getVersionInformation());
        
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
            s = aux.getString(i, "--group", "-g");
            if (s != null) {
                groupName = s;
                i++;
                continue;
            }
            n = aux.getInt(i, "--duration", "-d");
            if (n != -1) {
                duration = n;
                i++;
                continue;
            }
            n = aux.getInt(i, "--interval", "-i");
            if (n != -1) {
                activation = (double)n;
                i++;
                continue;
            }

            if (aux.getFlag(i, "--memberDescriptor", "-md"))
            {
                if (!observerOnly)
                {
                    hasDescriptor = true;
                }
                else
                {
                    System.out.println("invalid option: "+args[i]+" conflicting with justObserve");
                    usage();
                    System.exit(0);
                }

                if((s = (aux.getString(i, "--memberDescriptor", "-md")))!= null)
                {
                    if (!s.startsWith("-"))
                    {
                        memberDescriberString = s;
                        i++;
                    }
                }
                continue;
            }
            if (aux.getFlag(i, "--observerOnly", "-o"))
            {
                observerOnly = true;
                continue;                
            }
            if (args[i].startsWith("-", 0)) {
                System.out.println("invalid option: "+args[i]);
                usage();
            }
            realmServer = args[i];
        }

        System.out.print("Invoked as: TibGroup ");
        for (i = 0; i < argc; i++)
            System.out.print(args[i]+" ");
        System.out.println("");
    }

    
    public int run() throws FTLException
    {
        TibProperties   realmProps;
        TibProperties   groupProps;
        Realm           realm;
        ContentMatcher  am;
        ContentMatcher  sm;
        Subscriber      adv;
        Subscriber      sAdv = null;
        EventQueue      queue;
        Group           group;
        long            start;
        Message         descriptorMsg;
        
        // Set global trace to specified level
        if (trcLevel != null)
            FTL.setLogLevel(trcLevel);
        
        // Get a single realm per process
        realmProps = FTL.createProperties();
        realmProps.set(Realm.PROPERTY_STRING_USERNAME, user);
        realmProps.set(Realm.PROPERTY_STRING_USERPASSWORD, password);
        if (identifier != null)
            realmProps.set(Realm.PROPERTY_STRING_APPINSTANCE_IDENTIFIER, identifier);
        if (secondary != null)
            realmProps.set(Realm.PROPERTY_STRING_SECONDARY_SERVER, secondary);

        if (clientLabel != null)
            realmProps.set(Realm.PROPERTY_STRING_CLIENT_LABEL, clientLabel);
        else
            realmProps.set(Realm.PROPERTY_STRING_CLIENT_LABEL, this.getClass().getSimpleName());

        realm = FTL.connectToRealmServer(realmServer, appName, realmProps);
        realmProps.destroy();

        // Create a group advisory matcher
        am = realm.createContentMatcher("{\""+Advisory.FIELD_NAME+
                                        "\":\""+Group.ADVISORY_NAME_ORDINAL_UPDATE+"\"}");

        // Create an advisory subscriber
        adv = realm.createSubscriber(Advisory.ENDPOINT_NAME, am);
        am.destroy();

        // Create a queue and add subscriber to it
        queue = realm.createEventQueue();
        queue.addSubscriber(adv, new AdvisoryListener());

        // Join the specified group
        groupProps = FTL.createProperties();
        if (activation > 0)
             groupProps.set(Group.PROPERTY_DOUBLE_ACTIVATION_INTERVAL, activation);

        if (observerOnly)
        {
            groupProps.set(Group.PROPERTY_BOOLEAN_OBSERVER,true);
        }
        else if (hasDescriptor)
        {
            if (memberDescriberString == null)
            {
                descriptorMsg = null;
                groupProps.set(Group.PROPERTY_MESSAGE_MEMBER_DESCRIPTOR, descriptorMsg);                
            }
            else
            {
                descriptorMsg = realm.createMessage("identifier_format");
                descriptorMsg.setString("my_descriptor_string", memberDescriberString);
                groupProps.set(Group.PROPERTY_MESSAGE_MEMBER_DESCRIPTOR,descriptorMsg);                
            }
        }

        if (observerOnly || hasDescriptor)
        {
            sm = realm.createContentMatcher("{\""+Advisory.FIELD_NAME+
                                        "\":\""+Group.ADVISORY_NAME_GROUP_STATUS+"\"}");
            sAdv = realm.createSubscriber(Advisory.ENDPOINT_NAME, sm);
            queue.addSubscriber(sAdv, new StatusListener());     
        }

        group = GroupFactory.join(realm, groupName, groupProps);
        groupProps.destroy();

        System.out.printf("joined group: '%s'\n", group.getName());
        // Begin dispatching messages
        System.out.printf("waiting for %f seconds\n", duration);
        start = new Date().getTime()/1000;
        while (duration > 0)
        {
            queue.dispatch(duration);
            duration -= (new Date().getTime()/1000) - start;
            start = new Date().getTime()/1000;
        }
        System.out.printf("done waiting\n");

        // Leave the Group
        group.leave();

        // Clean up
        queue.removeSubscriber(adv);
        if (sAdv != null)
            queue.removeSubscriber(sAdv);

        queue.destroy();

        adv.close();
        if (sAdv != null)
            sAdv.close();

        realm.close();

        return 0;
    }
    
    public static void main(String[] args)
    {
        TibGroup s  = new TibGroup(args);
        try
        {
            s.run();
        }
        catch (Throwable e)
        {
            e.printStackTrace();
        }
    }

    private class StatusListener implements SubscriberListener
    {
        public void messagesReceived(List<Message> messages, EventQueue eventQueue)
        {
            int i;
            int msgNum = messages.size();

            for (i = 0;  i < msgNum;  i++)
            {
                try
                {
                    Message msg = messages.get(i);
                    System.out.println("group status advisory:");

                    System.out.println("Name: " + msg.getString(Advisory.FIELD_NAME));
                    System.out.println("    Severity: " +
                                       msg.getString(Advisory.FIELD_SEVERITY));
                    System.out.println("    Module: " +
                                       msg.getString(Advisory.FIELD_MODULE));
                    System.out.println("    group: " +
                                       msg.getString(Group.ADVISORY_FIELD_GROUP));

                    Message   memberDesc = null;
                    if (serverConnectionValid(msg))
                    {
                        Message[] membersStatusList = getMembersStatusList(msg);
                        if (membersStatusList.length == 0)
                        {
                            System.out.println("No other members have joined the group yet");
                        }

                        for(int members = 0; members < membersStatusList.length; members++)
                        {
                            Group.GroupMemberEvent statusEvent = getMemberStatusEvent(membersStatusList[members]);
                            System.out.println("Members status: " + valueOf(statusEvent));

                            memberDesc = getMembersDescriptor(membersStatusList[members]);
                            if (memberDesc != null)
                            {
                                if (memberDesc.isFieldSet("my_descriptor"))
                                {
                                    System.out.println("Member ID "+ memberDesc.getDouble("my_descriptor"));
                                }
                                if (memberDesc.isFieldSet("my_descriptor_string"))
                                {
                                    System.out.println("Member String ID "+ memberDesc.getString("my_descriptor_string"));
                                }
                            }
                            else
                            {
                                System.out.println("This member is anonymous");
                            }
                        }
                    }
                    else
                    {
                        System.out.println("Connection to server LOST -- reset MEMBER LIST!!!!!\n");
                    }
                }
                catch(FTLException exp)
                {
                    exp.printStackTrace();
                }
            }
        }
    }
  /*
   * Group Member Status is passed from group server to members as a long field
   * of the group members status message. This function does the conversion 
   * from long to GroupMemberStatus
   * 
   */
    private static Group.GroupMemberEvent valueOf(long memberStatus) throws FTLException
    {
        Group.GroupMemberEvent result = null;
        switch((int)memberStatus)
        {
            case 0:
                result = Group.GroupMemberEvent.JOINED;
                break;
            case 1:
                result = Group.GroupMemberEvent.LEFT;
                break;
            case 2:
                result = Group.GroupMemberEvent.DISCONNECTED;
                break;
            default:
                break;
        }
        if (result == null)
        {
            throw new FTLInvalidValueException("Invalid member status value " + memberStatus);
        }
        return result;
    }

    /*
     * This call returns the GroupMemberStatus  name string
     */
    public static String valueOf(Group.GroupMemberEvent memberStatus) throws FTLException
    {
        String result = null;
        switch(memberStatus)
        {
            case JOINED:
                result = "JOINED";
                break;
            case LEFT:
                result = "LEFT";
                break;
            case DISCONNECTED:
                result = "DISCONNECTED";
                break;
        }
        return result;
    }

/*
 * This call returns the GroupMemberStatus
 */
    public static Group.GroupMemberEvent getMemberStatusEvent(Message statusMsg) throws FTLException
    {
        Group.GroupMemberEvent result = null;
        if (statusMsg.isFieldSet(Group.FIELD_GROUP_MEMBER_EVENT))
        {
           result = valueOf(statusMsg.getLong(Group.FIELD_GROUP_MEMBER_EVENT));
        }
        else
        {
            throw new FTLInvalidFormatException("This method requires a group status message!");
        }
        return result;
    }

/*
 * This call returns list of status messages from a status advisory message
 */
    public static Message[] getMembersStatusList(Message statusAdvMsg) throws FTLException
    {
        Message[] result = null;
        if (statusAdvMsg.isFieldSet(Group.FIELD_GROUP_MEMBER_STATUS_LIST))
        {
            result = statusAdvMsg.getMessageArray(Group.FIELD_GROUP_MEMBER_STATUS_LIST);
        }
        else
        {
            throw new FTLInvalidFormatException("This method requires a group status advisory message!");
        }
        return result;
    }

/*
 * Group Member Server connection status is passed from group library
 * as a long field of status advisory message. This function does the conversion 
 * from long to Group.GroupMemberServerConnection 
 */

    public static Group.GroupMemberServerConnection
    serverConnectionValueOf(long serverConnStatus) throws FTLException
    {
        Group.GroupMemberServerConnection result = null;
        switch((int)serverConnStatus)
        {        
            case 0:
                result = Group.GroupMemberServerConnection.UNAVAILABLE;
                break;
            case 1:
                result = Group.GroupMemberServerConnection.AVAILABLE;
                break;
            default:
                break;
        }
        if (result == null)
        {
            throw new FTLInvalidValueException("Invalid server connection status value " + serverConnStatus);
        }

        return result;
    }

    public static boolean 
    serverConnectionValid(Message statusAdvMsg) throws FTLException
    {
        Group.GroupMemberServerConnection result = null;
        
        result = serverConnectionValueOf(statusAdvMsg.getLong(Group.FIELD_GROUP_SERVER_AVAILABLE));

        if (result == Group.GroupMemberServerConnection.AVAILABLE)
        {
            return true;
        }

        return false;
    }

/*
 * This call returns the descriptor message for the member that generated the event
 * The descriptor is null if the member did not provided one at the time of joining 
 */
    public static Message getMembersDescriptor(Message statusMsg) throws FTLException
    {
        Message result = null;
        if (statusMsg.isFieldSet(Group.FIELD_GROUP_MEMBER_DESCRIPTOR))
        {
            result = statusMsg.getMessage(Group.FIELD_GROUP_MEMBER_DESCRIPTOR);
        }
        return result;
    }

    private class AdvisoryListener implements SubscriberListener
    {
        public void messagesReceived(List<Message> messages, EventQueue eventQueue)
        {
            int i;
            int msgNum = messages.size();

            for (i = 0;  i < msgNum;  i++)
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
                    System.out.println("    group: " +
                                       msg.getString(Group.ADVISORY_FIELD_GROUP));
                    System.out.println("    ordinal: " +
                                       msg.getLong(Group.ADVISORY_FIELD_ORDINAL));
                }
                catch(FTLException exp)
                {
                    exp.printStackTrace();
                }
            }
        }
    }
}
