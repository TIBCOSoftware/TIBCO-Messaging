/* 
 * Copyright (c) 2002-$Date: 2017-03-27 16:01:48 -0500 (Mon, 27 Mar 2017) $ TIBCO Software Inc. 
 * Licensed under a BSD-style license. Refer to [LICENSE]
 * For more information, please contact:
 * TIBCO Software Inc., Palo Alto, California, USA
 * 
 * $Id: tibjmsRoute.java 92619 2017-03-27 21:01:48Z olivierh $
 * 
 */

/*
 * Example of how to use TIBCO Enterprise Message Service Administration API
 * to create and administer routes.
 *
 * This sample does the following:
 * 1) Creates a RouteInfo with parameters:
 *
 *    name = value specified by "-routeName" argument.
 *           Default is "E4JMS-SERVER-RT".
 *    url  = value specified by "-routeURL" argument.
 *           Default is "tcp://localhost:7223".
 *
 * 2) Changes the route URL to "tcp://localhost:7224" and updates the route.
 *
 * 3) Gets all of the routes from the server and prints them out.
 *
 * 4) Destroys the route created in this sample.
 *
 *
 *
 * Usage:  java tibjmsRoute  [options]
 *
 *    where options are:
 *
 *      -server     Server URL.
 *                  If not specified this sample assumes a
 *                  serverUrl of null
 *      -user       Admin user name. Default is "admin".
 *      -password   Admin password. Default is null.
 *      -routeName  The server name of the TIBCO EMS
 *                  server that will participate in routing.
 *                  Default is "E4JMS-SERVER-RT".
 *      -routeURL   The server URL of the TIBCO EMS
 *                  server that will participate in routing.
 *                  Default is "tcp://localhost:7223".
 *
 */

import javax.jms.*;

import com.tibco.tibjms.admin.*;

public class tibjmsRoute
{
    String      serverUrl       = null;
    String      username        = "admin";
    String      password        = null;
    String      routeUrl        = "tcp://localhost:7223";
    String      routeName       = "E4JMS-SERVER-RT";

    public tibjmsRoute(String[] args) {

        parseArgs(args);

        System.out.println("Admin: Route sample.");
        System.out.println("Using server:     "+serverUrl);

        try {
            //Create the admin connection to the TIBCO EMS server
            TibjmsAdmin admin = new TibjmsAdmin(serverUrl,username,password);

            //Create the RouteInfo
            RouteInfo route = new RouteInfo(routeName,routeUrl,null);

            try {
                route = admin.createRoute(route);
                System.out.println("Created Route: " + route);
            }
            catch(TibjmsAdminNameExistsException e) {
                route = admin.getRoute(routeName);
                System.out.println("Route already exists: " + route);
            }

            //Change the route URL
            routeUrl = "tcp://localhost:7224";
            route.setURL(routeUrl);
            admin.updateRoute(route);
            route = admin.getRoute(routeName);
            System.out.println("Updated Route URL: " + route);

            //Get all of the routes from the server
            RouteInfo[] routes = admin.getRoutes();

            System.out.println("Routes defined in the server:");
            for(int i=0; i<routes.length; i++) {
                System.out.println("\t" + routes[i]);
            }

            //Destroy the route
            admin.destroyRoute(routeName);
            System.out.println("Destroyed Route: " + routeName);
        }
        catch(TibjmsAdminException e)
        {
            e.printStackTrace();
        }
    }

    public static void main(String args[])
    {
        tibjmsRoute t = new tibjmsRoute(args);
    }

    void usage()
    {
        System.err.println("\nUsage: java tibjmsRoute [options]");
        System.err.println("");
        System.err.println("   where options are:");
        System.err.println("");
        System.err.println(" -server    <server URL>   - JMS server URL, default is local server");
        System.err.println(" -user      <user name>    - admin user name, default is null");
        System.err.println(" -password  <password>     - admin password, default is null");
        System.err.println(" -routeName <server name>  - routed JMS server name, default is \"E4JMS-SERVER-RT\"");
        System.err.println(" -routeURL  <server URL>   - routed JMS server URL, default is \"tcp://localhost:7223\"");
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
            if (args[i].compareTo("-routeURL")==0)
            {
                if ((i+1) >= args.length) usage();
                routeUrl = args[i+1];
                i += 2;
            }
            else
            if (args[i].compareTo("-routeName")==0)
            {
                if ((i+1) >= args.length) usage();
                routeName = args[i+1];
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


