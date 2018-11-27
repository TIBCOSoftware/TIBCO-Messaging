/*
 * Copyright (c) 2017-$Date: 2017-03-27 16:01:48 -0500 (Mon, 27 Mar 2017) $ TIBCO Software Inc. 
 * Licensed under a BSD-style license. Refer to [LICENSE]
 * For more information, please contact:
 * TIBCO Software Inc., Palo Alto, California, USA
 * 
 * $Id: csUsersGroups.cs 92619 2017-03-27 21:01:48Z olivierh $
 * 
 */

/// <summary>
/// Example of how to use TIBCO Enterprise Message Service Administration API
/// to create and administer users and groups.
///
/// This sample does the following:
/// 1) Creates the users "mair" and "ahren". If they already exist, they
///    are retrieved from the server.
/// 2) Changes the password for user "mair".
/// 3) Changes the description for user "ahren".
/// 4) Creates three groups: "Managers", "Leads", and "Employees". If they
///    already exist, they are retrieved from the server.
/// 5) Changes the description for group "Managers".
/// 6) Adds user "mair" to group "Managers".
/// 7) Adds users "mair" and "ahren" to group "Leads".
/// 8) Adds user "ahren" to group "Employees".
/// 9) Removes user "mair" from group "Leads".
/// 10) Gets all of the groups from the server and prints them out.
///
///
///
/// Usage:  csUsersGroups  [options]
///
///    where options are:
///
///      -server     Server URL.
///                  If not specified this sample assumes a
///                  serverUrl of localhost.
///
///      -user       Admin user name. Default is "admin".
///      -password   Admin user password. Default is null.
///
/// </summary>

using System;
using TIBCO.EMS.ADMIN;

public class csUsersGroups
{
    internal string serverUrl = null;
    internal string username  = "admin";
    internal string password  = null;

    public csUsersGroups(string[] args)
    {
        parseArgs(args);

        Console.WriteLine("Admin: Create Users and Groups sample.");
        Console.WriteLine("Using server:          " + serverUrl);

        try
        {
            // Create the admin connection to the TIBCO EMS server
            Admin admin = new Admin(serverUrl, username, password);

            // Create a UserInfo for each user
            UserInfo u1 = new UserInfo("mair", "Architect");
            UserInfo u2 = new UserInfo("ahren", "Engineer");

            // Create user "mair"
            try
            {
                u1 = admin.CreateUser(u1);
                Console.WriteLine("Created User: " + u1);
            }
            catch (AdminNameExistsException)
            {
                u1 = admin.GetUser("mair");
                Console.WriteLine("User already exists: " + u1);
            }

            // Create user "ahren"
            try
            {
                u2 = admin.CreateUser(u2);
                Console.WriteLine("Created User: " + u2);
            }
            catch (AdminNameExistsException)
            {
                u2 = admin.GetUser("ahren");
                Console.WriteLine("User already exists: " + u2);
            }

            // Change the password for user "mair"
            u1.Password = "mair_temp_pass";
            admin.UpdateUser(u1);
            u1 = admin.GetUser("mair");
            Console.WriteLine("Updated User Password: " + u1);

            // Change the description for user "ahren"
            u2.Description = "Lead Engineer";
            admin.UpdateUser(u2);
            u2 = admin.GetUser("ahren");
            Console.WriteLine("Updated User Description: " + u2);

            // Create groups
            GroupInfo g1 = new GroupInfo("Managers", "Company managers");
            try
            {
                g1 = admin.CreateGroup(g1);
                Console.WriteLine("Created Group: " + g1);
            }
            catch (AdminNameExistsException)
            {
                g1 = admin.GetGroup("Managers");
                Console.WriteLine("Group already exists: " + g1);
            }

            GroupInfo g2 = new GroupInfo("Leads", "Company leads");
            try
            {
                g2 = admin.CreateGroup(g2);
                Console.WriteLine("Created Group: " + g2);
            }
            catch (AdminNameExistsException)
            {
                g2 = admin.GetGroup("Leads");
                Console.WriteLine("Group already exists: " + g2);
            }

            GroupInfo g3 = new GroupInfo("Employees", "Company employees");
            try
            {
                g3 = admin.CreateGroup(g3);
                Console.WriteLine("Created Group: " + g3);
            }
            catch (AdminNameExistsException)
            {
                g3 = admin.GetGroup("Employees");
                Console.WriteLine("Group already exists: " + g3);
            }

            // Change group description
            g1.Description = "Company managers, North America";
            admin.UpdateGroup(g1);
            g1 = admin.GetGroup("Managers");
            Console.WriteLine("Updated Group Description: " + g1);

            // Add user "mair" to the "Managers" group
            admin.AddUserToGroup(g1.Name, u1.Name);
            Console.WriteLine("Added User: \"" + u1.Name + "\" to Group: \"" + g1.Name + "\"");

            // Add users "mair" and "ahren" to the "Leads" group
            string[] users = new string[] { u1.Name, u2.Name };
            admin.AddUsersToGroup(g2.Name, users);
            Console.WriteLine("Added Users: \"" + u1.Name + "\", \"" + u2.Name + "\" to Group: \"" + g2.Name + "\"");

            // Add users "mair" and "ahren" to the "Employees" group
            admin.AddUsersToGroup(g3.Name, users);
            Console.WriteLine("Added Users: \"" + u1.Name + "\", \"" + u2.Name + "\" to Group: \"" + g3.Name + "\"");

            // Get groups from server
            g1 = admin.GetGroup("Managers");
            g2 = admin.GetGroup("Leads");
            g3 = admin.GetGroup("Employees");

            // Remove user "mair" from "Lead" group
            admin.RemoveUserFromGroup(g2.Name, u1.Name);
            Console.WriteLine("Removed User: \"" + u1.Name + "\" from Group: \"" + g2.Name + "\"");

            // Get groups from server
            GroupInfo[] groups = admin.Groups;

            Console.WriteLine("Groups defined in the server:");
            for (int i = 0; i < groups.Length; i++)
            {
                Console.WriteLine("\t" + groups[i]);
            }
        }
        catch (AdminException e)
        {
            Console.Error.WriteLine("Exception in csUsersGroups: " + e.Message);
            Console.Error.WriteLine(e.StackTrace);
            Environment.Exit(-1);
        }
    }

    public static void Main(string[] args)
    {
        csUsersGroups t = new csUsersGroups(args);
    }

    internal void usage()
    {
        Console.Error.WriteLine("\nUsage: csUsersGroups [options]");
        Console.Error.WriteLine("");
        Console.Error.WriteLine("   where options are:");
        Console.Error.WriteLine("");
        Console.Error.WriteLine(" -server    <server URL> - JMS server URL, default is local server");
        Console.Error.WriteLine(" -user      <user name>  - admin user name, default is 'admin'");
        Console.Error.WriteLine(" -password  <password>   - admin password, default is null");
        Environment.Exit(0);
    }

    internal void parseArgs(string[] args)
    {
        int i = 0;

        while (i < args.Length)
        {
            if (args[i].CompareTo("-server") == 0)
            {
                if ((i + 1) >= args.Length)
                {
                    usage();
                }
                serverUrl = args[i + 1];
                i += 2;
            }
            else
            if (args[i].CompareTo("-user") == 0)
            {
                if ((i + 1) >= args.Length)
                {
                    usage();
                }
                username = args[i + 1];
                i += 2;
            }
            else
            if (args[i].CompareTo("-password") == 0)
            {
                if ((i + 1) >= args.Length)
                {
                    usage();
                }
                password = args[i + 1];
                i += 2;
            }
            else
            if (args[i].CompareTo("-help") == 0)
            {
                usage();
            }
            else
            {
                Console.WriteLine("Unrecognized parameter: " + args[i]);
                usage();
            }
        }
    }
}
