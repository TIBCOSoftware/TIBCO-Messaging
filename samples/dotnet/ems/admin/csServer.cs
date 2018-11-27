/*
 * Copyright (c) 2017-$Date: 2017-06-16 16:40:08 -0500 (Fri, 16 Jun 2017) $ TIBCO Software Inc. 
 * Licensed under a BSD-style license. Refer to [LICENSE]
 * For more information, please contact:
 * TIBCO Software Inc., Palo Alto, California, USA
 * 
 * $Id: csServer.cs 94085 2017-06-16 21:40:08Z vinasing $
 * 
 */

/// <summary>
/// Example of how to use TIBCO Enterprise Message Service Administration API
/// to administer a TIBCO EMS server.
///
/// This sample is interactive and allows the user to set global server
/// properties in the server specified by the "-server" option.
///
/// Usage:  csServer  [options]
///
///    where options are:
///
///      -server     Server URL.
///                  If not specified this sample assumes a
///                  serverUrl of null
///      -user       Admin user name. Default is "admin".
///      -password   Admin password. Default is null.
///
/// </summary>

using System;
using System.IO;
using TIBCO.EMS.ADMIN;

public class csServer
{
    internal string serverUrl    = null;
    internal string username     = "admin";
    internal string password     = null;
    internal Admin admin         = null;
    internal ServerInfo server   = null;
    internal StreamReader reader = new StreamReader(Console.OpenStandardInput());

    public csServer(string[] args)
    {
        parseArgs(args);

        try
        {
            // Create the admin connection to the TIBCO EMS server
            admin = new Admin(serverUrl, username, password);
            admin.AutoSave = true;

            while (true)
            {
                printMenu();

                try
                {
                    string s = reader.ReadLine();
                    int c = int.Parse(s);

                    if (c < 1 || c > 6)
                    {
                        printWarning();
                    }
                    else
                    {
                        handleChoice(c);
                    }
                }
                catch (IOException e)
                {
                    Console.Error.WriteLine("Exception in csServer: " + e.Message);
                    Console.Error.WriteLine(e.StackTrace);
                }
                catch (System.FormatException)
                {
                    printWarning();
                }
            }
        }
        catch (AdminException e)
        {
            Console.Error.WriteLine("Exception in csServer: " + e.Message);
            Console.Error.WriteLine(e.StackTrace);
            Environment.Exit(-1);
        }
    }

    private void doAuthorization()
    {
        server = admin.Info;
        bool b = server.AuthorizationEnabled;

        Console.WriteLine("\nAuthorization is currently: " + (b ? "ENABLED" : "DISABLED"));

        if (b)
        {
            b = getYN("Would you like to DISABLE Authorization");
            server.AuthorizationEnabled = !b;
        }
        else
        {
            b = getYN("Would you like to ENABLE Authorization");
            server.AuthorizationEnabled = b;
        }

        admin.UpdateServer(server);
    }

    private void doMaxMsgMem()
    {
        server = admin.Info;
        
        while (true)
        {
            bool noException = true;
            long l = server.MaxMsgMemory;
            Console.WriteLine("\nMaximum Message Memory is currently: " + l + " bytes");
            Console.Write("\nEnter the Maximum Message Memory (bytes): ");

            try
            {
                string s = reader.ReadLine();
                l = long.Parse(s);
                if (l < 0)
                {
                    throw new System.FormatException();
                }
                server.MaxMsgMemory = l;
                admin.UpdateServer(server);
            }
            catch (Exception e) when (e is ArgumentException
                                   || e is FormatException
                                   || e is OverflowException)
            {
                noException = false;
                printMaxMsgMemWarning();
            }
            catch (Exception ex)
            {
                noException = false;
                Console.Error.WriteLine("Exception in csServer: " + ex.Message);
                Console.Error.WriteLine(ex.StackTrace);
            }

            if (noException)
            {
                break;
            }
        }
    }

    private void doCorrID()
    {
        server = admin.Info;
        bool b = server.TrackCorrelationIds;

        Console.WriteLine("\nCorrelation ID Tracking is currently: " + (b ? "ENABLED" : "DISABLED"));

        if (b)
        {
            b = getYN("Would you like to DISABLE Correlation ID Tracking");
            server.TrackCorrelationIds = !b;
        }
        else
        {
            b = getYN("Would you like to ENABLE Correlation ID Tracking");
            server.TrackCorrelationIds = b;
        }

        admin.UpdateServer(server);
    }

    private void doMsgID()
    {
        server = admin.Info;
        bool b = server.TrackMsgIds;

        Console.WriteLine("\nMessage ID Tracking is currently: " + (b ? "ENABLED" : "DISABLED"));

        if (b)
        {
            b = getYN("Would you like to DISABLE Message ID Tracking");
            server.TrackMsgIds = !b;
        }
        else
        {
            b = getYN("Would you like to ENABLE Message ID Tracking");
            server.TrackMsgIds = b;
        }

        admin.UpdateServer(server);
    }

    private void doShow()
    {
        server = admin.Info;
        string s = server.ToString();
        string[] st = s.Split(';');

        s = st[0];
        s = s.Substring(12);
        Console.WriteLine("\n---------------------------------------------------------");
        Console.WriteLine("-                      ServerInfo                       -");
        Console.WriteLine("---------------------------------------------------------");
        Console.WriteLine(s.Trim());

        foreach (string t in st)
        {
            if (t.Equals("  }"))
            {
                continue;
            }
            Console.WriteLine(t.Trim());
        }
    }

    private void doExit()
    {
        Console.WriteLine("\n*********************************************************");
        Console.WriteLine("*                       Goodbye                         *");
        Console.WriteLine("*********************************************************");
        Environment.Exit(0);
    }

    private bool getYN(string question)
    {
        while (true)
        {
            Console.Write("\n" + question + " (y/n): ");
            string s = reader.ReadLine();
            if (s.Equals("y", StringComparison.CurrentCultureIgnoreCase))
            {
                return true;
            }
            else if (s.Equals("n", StringComparison.CurrentCultureIgnoreCase))
            {
                return false;
            }
            else
            {
                printYNWarning();
            }
        }
    }

    private void handleChoice(int c)
    {
        switch (c)
        {
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

    private void printMaxMsgMemWarning()
    {
        Console.WriteLine("\n*********************************************************");
        Console.WriteLine("*       Please enter a reasonable number >= 65536       *");
        Console.WriteLine("*          or 0 for unlimited message memory.           *");
        Console.WriteLine("*********************************************************");
    }

    private void printYNWarning()
    {
        Console.WriteLine("\n*********************************************************");
        Console.WriteLine("                 Please enter 'y' or 'n'.               *");
        Console.WriteLine("*********************************************************");
    }

    private void printWarning()
    {
        Console.WriteLine("\n*********************************************************");
        Console.WriteLine("*         Please enter a number between 1 and 6.        *");
        Console.WriteLine("*********************************************************");
    }

    private void printMenu()
    {
        Console.WriteLine("---------------------------------------------------------");
        Console.WriteLine("-               TIBCO Enterprise Message Service        -");
        Console.WriteLine("-                 Server Administration Sample          -");
        Console.WriteLine("---------------------------------------------------------");
        Console.WriteLine(" Select an item from the following menu:");
        Console.WriteLine("---------------------------------------------------------");
        Console.WriteLine(" 1  Enable/Disable Authorization");
        Console.WriteLine(" 2  Set Maximum Message Memory");
        Console.WriteLine(" 3  Enable/Disable Correlation ID Tracking");
        Console.WriteLine(" 4  Enable/Disable Message ID Tracking");
        Console.WriteLine(" 5  Show ServerInfo");
        Console.WriteLine(" 6  Exit");
        Console.WriteLine("---------------------------------------------------------");
        Console.Write(" Choice: ");
    }

    public static void Main(string[] args)
    {
        csServer t = new csServer(args);
    }

    internal void usage()
    {
        Console.Error.WriteLine("\nUsage:  csServer [options]");
        Console.Error.WriteLine("");
        Console.Error.WriteLine("   where options are:");
        Console.Error.WriteLine("");
        Console.Error.WriteLine(" -server    <server URL>   - JMS server URL, default is local server");
        Console.Error.WriteLine(" -user      <user name>    - admin user name, default is 'admin'");
        Console.Error.WriteLine(" -password  <password>     - admin password, default is null");
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
