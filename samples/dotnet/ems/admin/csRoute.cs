/*
 * Copyright (c) 2017-$Date: 2017-03-27 16:01:48 -0500 (Mon, 27 Mar 2017) $ TIBCO Software Inc. 
 * Licensed under a BSD-style license. Refer to [LICENSE]
 * For more information, please contact:
 * TIBCO Software Inc., Palo Alto, California, USA
 * 
 * $Id: csRoute.cs 92619 2017-03-27 21:01:48Z olivierh $
 * 
 */

/// <summary>
/// Example of how to use TIBCO Enterprise Message Service Administration API
/// to create and administer routes.
///
/// This sample does the following:
/// 1) Creates a RouteInfo with parameters:
///
///    name = value specified by "-routeName" argument.
///           Default is "E4JMS-SERVER-RT".
///    url  = value specified by "-routeURL" argument.
///           Default is "tcp://localhost:7223".
///
/// 2) Changes the route URL to "tcp://localhost:7224" and updates the route.
///
/// 3) Gets all of the routes from the server and prints them out.
///
/// 4) Destroys the route created in this sample.
///
/// Usage:  csRoute  [options]
///
///    where options are:
///
///      -server     Server URL.
///                  If not specified this sample assumes a
///                  serverUrl of null
///      -user       Admin user name. Default is "admin".
///      -password   Admin password. Default is null.
///      -routeName  The server name of the TIBCO EMS
///                  server that will participate in routing.
///                  Default is "E4JMS-SERVER-RT".
///      -routeURL   The server URL of the TIBCO EMS
///                  server that will participate in routing.
///                  Default is "tcp://localhost:7223".
///
/// </summary>

using System;
using TIBCO.EMS.ADMIN;

public class csRoute
{
    internal string serverUrl = null;
    internal string username  = "admin";
    internal string password  = null;
    internal string routeUrl  = "tcp://localhost:7223";
    internal string routeName = "E4JMS-SERVER-RT";

    public csRoute(string[] args)
    {
        parseArgs(args);

        Console.WriteLine("Admin: Route sample.");
        Console.WriteLine("Using server:     " + serverUrl);

        try
        {
            // Create the admin connection to the TIBCO EMS server
            Admin admin = new Admin(serverUrl, username, password);
            // Create the RouteInfo
            RouteInfo route = new RouteInfo(routeName, routeUrl, null);

            try
            {
                route = admin.CreateRoute(route);
                Console.WriteLine("Created Route: " + route);
            }
            catch (AdminNameExistsException)
            {
                route = admin.GetRoute(routeName);
                Console.WriteLine("Route already exists: " + route);
            }

            // Change the route URL
            routeUrl = "tcp://localhost:7224";
            route.URL = routeUrl;
            admin.UpdateRoute(route);
            route = admin.GetRoute(routeName);
            Console.WriteLine("Updated Route URL: " + route);
            // Get all of the routes from the server
            RouteInfo[] routes = admin.Routes;
            Console.WriteLine("Routes defined in the server:");

            for (int i = 0; i < routes.Length; i++)
            {
                Console.WriteLine("\t" + routes[i]);
            }

            // Destroy the route
            admin.DestroyRoute(routeName);
            Console.WriteLine("Destroyed Route: " + routeName);
        }
        catch (AdminException e)
        {
            Console.Error.WriteLine("Exception in csRoute: " + e.Message);
            Console.Error.WriteLine(e.StackTrace);
            Environment.Exit(-1);
        }
    }

    public static void Main(string[] args)
    {
        csRoute t = new csRoute(args);
    }

    internal void usage()
    {
        Console.Error.WriteLine("\nUsage:  csRoute [options]");
        Console.Error.WriteLine("");
        Console.Error.WriteLine("   where options are:");
        Console.Error.WriteLine("");
        Console.Error.WriteLine(" -server    <server URL>   - JMS server URL, default is local server");
        Console.Error.WriteLine(" -user      <user name>    - admin user name, default is 'admin'");
        Console.Error.WriteLine(" -password  <password>     - admin password, default is null");
        Console.Error.WriteLine(" -routeName <server name>  - routed JMS server name, default is \"E4JMS-SERVER-RT\"");
        Console.Error.WriteLine(" -routeURL  <server URL>   - routed JMS server URL, default is \"tcp://localhost:7223\"");
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
            if (args[i].CompareTo("-routeURL") == 0)
            {
                if ((i + 1) >= args.Length)
                {
                    usage();
                }
                routeUrl = args[i + 1];
                i += 2;
            }
            else
            if (args[i].CompareTo("-routeName") == 0)
            {
                if ((i + 1) >= args.Length)
                {
                    usage();
                }
                routeName = args[i + 1];
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
