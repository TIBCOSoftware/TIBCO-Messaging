/* 
 * Copyright (c) 2002-$Date: 2017-03-27 16:01:48 -0500 (Mon, 27 Mar 2017) $ TIBCO Software Inc. 
 * Licensed under a BSD-style license. Refer to [LICENSE]
 * For more information, please contact:
 * TIBCO Software Inc., Palo Alto, California, USA
 * 
 * $Id: tibjmsUsersGroups.java 92619 2017-03-27 21:01:48Z olivierh $
 * 
 */

/*
 * Example of how to use TIBCO Enterprise Message Service Administration API
 * to create and administer users and groups.
 *
 * This sample does the following:
 * 1) Creates the users "mair" and "ahren". If they already exist, they
 *    are retrieved from the server.
 * 2) Changes the password for user "mair".
 * 3) Changes the description for user "ahren".
 * 4) Creates three groups: "Managers", "Leads", and "Employees". If they
 *    already exist, they are retrieved from the server.
 * 5) Changes the description for group "Managers".
 * 6) Adds user "mair" to group "Managers".
 * 7) Adds users "mair" and "ahren" to group "Leads".
 * 8) Adds user "ahren" to group "Employees".
 * 9) Removes user "mair" from group "Leads".
 * 10) Gets all of the groups from the server and prints them out.
 *
 *
 *
 * Usage:  java tibjmsUsersGroups  [options]
 *
 *    where options are:
 *
 *      -server     Server URL.
 *                  If not specified this sample assumes a
 *                  serverUrl of localhost.
 *
 *      -user       Admin user name. Default is "admin".
 *      -password   Admin user password. Default is null.
 *
 *
 */

import javax.jms.*;

import com.tibco.tibjms.admin.*;

public class tibjmsUsersGroups
{
    String      serverUrl       = null;
    String      username        = "admin";
    String      password        = null;

    public tibjmsUsersGroups(String[] args) {

        parseArgs(args);

        System.out.println("Admin: Create Users and Groups sample.");
        System.out.println("Using server:          "+serverUrl);

        try {
            //Create the admin connection to the TIBCO EMS server
            TibjmsAdmin admin = new TibjmsAdmin(serverUrl,username,password);

            //Create a UserInfo for each user
            UserInfo u1 = new UserInfo("mair","Architect");
            UserInfo u2 = new UserInfo("ahren","Engineer");

            //Create user "mair"
            try {
                u1 = admin.createUser(u1);
                System.out.println("Created User: " + u1);
            }
            catch(TibjmsAdminNameExistsException e) {
                u1 = admin.getUser("mair");
                System.out.println("User already exists: " + u1);
            }

            //Create user "ahren"
            try {
                u2 = admin.createUser(u2);
                System.out.println("Created User: " + u2);
            }
            catch(TibjmsAdminNameExistsException e) {
                u2 = admin.getUser("ahren");
                System.out.println("User already exists: " + u2);
            }

            //Change the password for user "mair"
            u1.setPassword("mair_temp_pass");
            admin.updateUser(u1);
            u1 = admin.getUser("mair");
            System.out.println("Updated User Password: " + u1);

            //Change the description for user "ahren"
            u2.setDescription("Lead Engineer");
            admin.updateUser(u2);
            u2 = admin.getUser("ahren");
            System.out.println("Updated User Description: " + u2);

            //Create groups
            GroupInfo g1 = new GroupInfo("Managers","Company managers");
            try {
                g1 = admin.createGroup(g1);
                System.out.println("Created Group: " + g1);
            }
            catch(TibjmsAdminNameExistsException e) {
                g1 = admin.getGroup("Managers");
                System.out.println("Group already exists: " + g1);
            }

            GroupInfo g2 = new GroupInfo("Leads","Company leads");
            try {
                g2 = admin.createGroup(g2);
                System.out.println("Created Group: " + g2);
            }
            catch(TibjmsAdminNameExistsException e) {
                g2 = admin.getGroup("Leads");
                System.out.println("Group already exists: " + g2);
            }

            GroupInfo g3 = new GroupInfo("Employees","Company employees");
            try {
                g3 = admin.createGroup(g3);
                System.out.println("Created Group: " + g3);
            }
            catch(TibjmsAdminNameExistsException e) {
                g3 = admin.getGroup("Employees");
                System.out.println("Group already exists: " + g3);
            }

            //Change group description
            g1.setDescription("Company managers, North America");
            admin.updateGroup(g1);
            g1 = admin.getGroup("Managers");
            System.out.println("Updated Group Description: " + g1);

            //Add user "mair" to the "Managers" group
            admin.addUserToGroup(g1.getName(),u1.getName());
            System.out.println("Added User: \"" + u1.getName() +
                               "\" to Group: \"" + g1.getName() + "\"");

            //Add users "mair" and "ahren" to the "Leads" group
            String[] users = {u1.getName(),u2.getName()};
            admin.addUsersToGroup(g2.getName(),users);
            System.out.println("Added Users: \"" + u1.getName() +
                "\", \"" + u2.getName() + "\" to Group: \"" + g2.getName() + "\"");

            //Add users "mair" and "ahren" to the "Employees" group
            admin.addUsersToGroup(g3.getName(),users);
            System.out.println("Added Users: \"" + u1.getName() +
                "\", \"" + u2.getName() + "\" to Group: \"" + g3.getName() + "\"");

            //Get groups from server
            g1 = admin.getGroup("Managers");
            g2 = admin.getGroup("Leads");
            g3 = admin.getGroup("Employees");

            //Remove user "mair" from "Lead" group
            admin.removeUserFromGroup(g2.getName(),u1.getName());
            System.out.println("Removed User: \"" + u1.getName() +
                               "\" from Group: \"" + g2.getName() + "\"");

            //Get groups from server
            GroupInfo[] groups = admin.getGroups();

            System.out.println("Groups defined in the server:");
            for(int i=0; i<groups.length; i++) {
                System.out.println("\t" + groups[i]);
            }
        }
        catch(TibjmsAdminException e)
        {
            e.printStackTrace();
        }
    }

    public static void main(String args[])
    {
        tibjmsUsersGroups t = new tibjmsUsersGroups(args);
    }

    void usage()
    {
        System.err.println("\nUsage: java tibjmsUsersGroups [options]");
        System.err.println("");
        System.err.println("   where options are:");
        System.err.println("");
        System.err.println(" -server    <server URL> - JMS server URL, default is local server");
        System.err.println(" -user      <user name>  - admin user name, default is null");
        System.err.println(" -password  <password>   - admin password, default is null");
        System.exit(0);
    }

    void parseArgs(String[] args)
    {
        int i=0;

        while(i < args.length)
        {
            if (args[i].compareTo("-server")==0)
            {
                if ((i+1) >= args.length) usage();
                serverUrl = args[i+1];
                i += 2;
            }
            else
            if (args[i].compareTo("-user")==0)
            {
                if ((i+1) >= args.length) usage();
                username = args[i+1];
                i += 2;
            }
            else
            if (args[i].compareTo("-password")==0)
            {
                if ((i+1) >= args.length) usage();
                password = args[i+1];
                i += 2;
            }
            else
            if (args[i].compareTo("-help")==0)
            {
                usage();
            }
            else
            {
                System.out.println("Unrecognized parameter: "+args[i]);
                usage();
            }
        }
    }

}


