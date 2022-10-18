/* 
 * Copyright (c) 2002-$Date: 2017-06-19 10:55:41 -0500 (Mon, 19 Jun 2017) $ TIBCO Software Inc. 
 * Licensed under a BSD-style license. Refer to [LICENSE]
 * For more information, please contact:
 * TIBCO Software Inc., Palo Alto, California, USA
 * 
 * $Id: tibjmsServer.java 94094 2017-06-19 15:55:41Z vinasing $
 * 
 */

/*
 * Example of how to use TIBCO Enterprise Message Service Administration API
 * to administer a TIBCO EMS server.
 *
 * This sample is interactive and allows the user to set global server
 * properties in the server specified by the "-server" option.
 *
 * Usage:  java tibjmsServer  [options]
 *
 *    where options are:
 *
 *      -server     Server URL.
 *                  If not specified this sample assumes a
 *                  serverUrl of null
 *      -user       Admin user name. Default is "admin".
 *      -password   Admin password. Default is null.
 *
 */

import java.io.*;
import java.util.*;
import javax.jms.*;

import com.tibco.tibjms.admin.*;

public class tibjmsServer
{
    String      serverUrl       = null;
    String      username        = "admin";
    String      password        = null;

    TibjmsAdmin admin           = null;
    ServerInfo server           = null;
    BufferedReader reader       = new BufferedReader(new InputStreamReader(System.in));

    public tibjmsServer(String[] args) {

        parseArgs(args);

        try {
            //Create the admin connection to the TIBCO EMS server
            admin = new TibjmsAdmin(serverUrl,username,password);

            admin.setAutoSave(true);

            while(true) {
                printMenu();

                try {
                    String s = reader.readLine();
                    int c = Integer.parseInt(s);

                    if (c <1 || c > 6) {
                        printWarning();
                    }
                    else {
                        handleChoice(c);
                    }
                }
                catch (IOException e) {
                    e.printStackTrace();
                }
                catch (NumberFormatException e) {
                    printWarning();
                }
            }

        }
        catch(TibjmsAdminException e)
        {
            e.printStackTrace();
        }
    }

    private void doAuthorization() throws TibjmsAdminException, IOException {
        server = admin.getInfo();
        boolean b = server.isAuthorizationEnabled();

        System.out.println("\nAuthorization is currently: " + (b?"ENABLED":"DISABLED"));

        if(b) {
            b = getYN("Would you like to DISABLE Authorization");
            server.setAuthorizationEnabled(!b);
        }
        else {
            b = getYN("Would you like to ENABLE Authorization");
            server.setAuthorizationEnabled(b);
        }

        admin.updateServer(server);
    }

    private void doMaxMsgMem() throws TibjmsAdminException {
        server = admin.getInfo();

        while (true)
        {
            boolean noException = true;
            long l = server.getMaxMsgMemory();
            System.out.println("\nMaximum Message Memory is currently: " + l + " bytes");
            System.out.print("\nEnter the Maximum Message Memory (bytes): ");

            try
            {
                String s = reader.readLine();
                l = Long.parseLong(s);

                if (l < 0)
                {
                    throw new NumberFormatException();
                }
                server.setMaxMsgMemory(l);
                admin.updateServer(server);
            }
            catch (IllegalArgumentException ex)
            {
                noException = false;
                printMaxMsgMemWarning();
            }
            catch (Exception ex)
            {
                noException = false;
                System.out.println("Exception in tibjmsServer: " + ex);
                ex.printStackTrace();
            }

            if (noException)
            {
                break;
            }
        }
    }

    private void doCorrID() throws TibjmsAdminException , IOException {
        server = admin.getInfo();
        boolean b = server.isTrackCorrelationIds();

        System.out.println("\nCorrelation ID Tracking is currently: " + (b?"ENABLED":"DISABLED"));

        if(b) {
            b = getYN("Would you like to DISABLE Correlation ID Tracking");
            server.setTrackCorrelationIds(!b);
        }
        else {
            b = getYN("Would you like to ENABLE Correlation ID Tracking");
            server.setTrackCorrelationIds(b);
        }

        admin.updateServer(server);
    }

    private void doMsgID() throws TibjmsAdminException, IOException {
        server = admin.getInfo();
        boolean b = server.isTrackMsgIds();

        System.out.println("\nMessage ID Tracking is currently: " + (b?"ENABLED":"DISABLED"));

        if(b) {
            b = getYN("Would you like to DISABLE Message ID Tracking");
            server.setTrackMsgIds(!b);
        }
        else {
            b = getYN("Would you like to ENABLE Message ID Tracking");
            server.setTrackMsgIds(b);
        }

        admin.updateServer(server);
    }

    private void doShow() throws TibjmsAdminException {
        server = admin.getInfo();
        String s = server.toString();
        StringTokenizer st = new StringTokenizer(s,";");

        s = st.nextToken();
        s = s.substring(12);
        System.out.println("\n---------------------------------------------------------");
        System.out.println("-                      ServerInfo                       -");
        System.out.println("---------------------------------------------------------");
        System.out.println(s.trim());

        while(st.hasMoreTokens()) {
            String t = st.nextToken();
            if(t.equals("  }")) continue;
            System.out.println(t.trim());
        }
    }

    private void doExit() {
        System.out.println("\n*********************************************************");
        System.out.println("*                       Goodbye                         *");
        System.out.println("*********************************************************");
        System.exit(0);
    }

    private boolean getYN(String question) throws IOException {
        while (true)
        {
            System.out.print("\n" + question + " (y/n): ");
            String s = reader.readLine();
            if (s.equalsIgnoreCase("y")) {
                return true;
            }
            else if (s.equalsIgnoreCase("n")) {
                return false;
            }
            else {
                printYNWarning();
            }
        }
    }

    private void handleChoice(int c) throws TibjmsAdminException, IOException {
        switch(c) {
            case 1:
                doAuthorization();
                break;
            case 2:
                doMaxMsgMem();
                break;
            case 3:
                doCorrID();
                break;
            case 4:
                doMsgID();
                break;
            case 5:
                doShow();
                break;
            case 6:
                doExit();
                break;
            default:
                break;
        }
    }

    private void printMaxMsgMemWarning() {
        System.out.println("\n*********************************************************");
        System.out.println("*       Please enter a reasonable number >= 65536       *");
        System.out.println("*          or 0 for unlimited message memory.           *");
        System.out.println("*********************************************************");
    }

    private void printYNWarning() {
        System.out.println("\n*********************************************************");
        System.out.println("                 Please enter 'y' or 'n'.               *");
        System.out.println("*********************************************************");
    }

    private void printWarning() {
        System.out.println("\n*********************************************************");
        System.out.println("*         Please enter a number between 1 and 6.        *");
        System.out.println("*********************************************************");
    }

    private void printMenu() {
        System.out.println("---------------------------------------------------------");
        System.out.println("-               TIBCO Enterprise Message Service        -");
        System.out.println("-             Server Administration Sample              -");
        System.out.println("---------------------------------------------------------");
        System.out.println(" Select an item from the following menu:");
        System.out.println("---------------------------------------------------------");
        System.out.println(" 1  Enable/Disable Authorization");
        System.out.println(" 2  Set Maximum Message Memory");
        System.out.println(" 3  Enable/Disable Correlation ID Tracking");
        System.out.println(" 4  Enable/Disable Message ID Tracking");
        System.out.println(" 5  Show ServerInfo");
        System.out.println(" 6  Exit");
        System.out.println("---------------------------------------------------------");
        System.out.print(" Choice: ");
    }

    public static void main(String args[])
    {
        tibjmsServer t = new tibjmsServer(args);
    }

    void usage()
    {
        System.err.println("\nUsage: java tibjmsServer [options]");
        System.err.println("");
        System.err.println("   where options are:");
        System.err.println("");
        System.err.println(" -server    <server URL>   - JMS server URL, default is local server");
        System.err.println(" -user      <user name>    - admin user name, default is null");
        System.err.println(" -password  <password>     - admin password, default is null");
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


