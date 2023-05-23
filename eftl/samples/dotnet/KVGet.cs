/*
 * Copyright (c) 2013-$Date: 2015-07-31 16:05:02 -0700 (Fri, 31 Jul 2015) $ Cloud Software Group, Inc.
 * All Rights Reserved.
 */

using System;
using System.Collections;
using System.Threading;
using TIBCO.EFTL;

public class KVGet {

    static String url = "ws://localhost:8585/map";
    static String username = "user";
    static String password = "password";
    static String name = "sample_map";
    static String key = "key1";

    private static void Usage () {
        Console.WriteLine ();
        Console.WriteLine ("usage: KVGet [options] url");
        Console.WriteLine ();
        Console.WriteLine ("options:");
        Console.WriteLine ("  -u, --username <username>");
        Console.WriteLine ("  -p, --password <password>");
        Console.WriteLine ("  -m, --map <map>");
        Console.WriteLine ("  -k, --key <key>");
        Console.WriteLine ();
        System.Environment.Exit (1);
    }

    public class KVMapListener : IKVMapListener {
        private IConnection connection;

        public KVMapListener (IConnection connection) {
            this.connection = connection;
        }

        public void OnSuccess (String key, IMessage value) {
            Console.WriteLine ("\nSuccess getting key-value pair {0} = {1}\n", key, value);

            // Asynchronously disconnect from the eFTL server.
            connection.Disconnect ();
        }

        public void OnError (String key, IMessage value, int code, String reason) {
            Console.WriteLine ("\nError getting key-value pair: {0}\n", reason);

            // Asynchronously disconnect from the eFTL server.
            connection.Disconnect ();
        }
    }

    public class ConnectionListener : IConnectionListener {
        public void OnConnect (IConnection connection) {
            Console.WriteLine ("Connected to eFTL server at " + url);

            // Create the map.
            IKVMap map = connection.CreateKVMap (name);

            // Asynchronously get the key-value pair.
            map.Get (key, new KVMapListener (connection));
        }

        public void OnDisconnect (IConnection connection, int code, String reason) {
            Console.WriteLine ("Disconnected from eFTL server: " + reason);
        }

        public void OnError (IConnection connection, int code, String reason) {
            Console.WriteLine ("Connection error: " + reason);
        }

        public void OnReconnect (IConnection connection) {
            Console.WriteLine ("Reconnected to eFTL server at " + url);
        }
    }

    public KVGet (String url, String username, String password) {
        Hashtable options = new Hashtable ();

        if (username != null)
            options.Add (EFTL.PROPERTY_USERNAME, username);
        if (password != null)
            options.Add (EFTL.PROPERTY_PASSWORD, password);

        // Asynchronously connect to the eFTL server.
        EFTL.Connect (url, options, new ConnectionListener ());
    }

    public static void Main (string[] args) {
        for (int i = 0; i < args.Length; i++) {
            if (args[i].Equals ("--username") || args[i].Equals ("-u")) {
                if (i + 1 < args.Length && !args[i + 1].StartsWith ("-")) {
                    username = args[++i];
                } else {
                    Usage ();
                }
            } else if (args[i].Equals ("--password") || args[i].Equals ("-p")) {
                if (i + 1 < args.Length && !args[i + 1].StartsWith ("-")) {
                    password = args[++i];
                } else {
                    Usage ();
                }
            } else if (args[i].Equals ("--map") || args[i].Equals ("-m")) {
                if (i + 1 < args.Length && !args[i + 1].StartsWith ("-")) {
                    name = args[++i];
                } else {
                    Usage ();
                }
            } else if (args[i].Equals ("--key") || args[i].Equals ("-k")) {
                if (i + 1 < args.Length && !args[i + 1].StartsWith ("-")) {
                    key = args[++i];
                } else {
                    Usage ();
                }
            } else if (args[i].StartsWith ("-")) {
                Usage ();
            } else {
                url = args[i];
            }
        }

        Console.Write ("#\n# KVGet\n#\n# {0}\n#\n", EFTL.GetVersion ());

        try {
            new KVGet (url, username, password);
        } catch (Exception e) {
            Console.WriteLine (e.ToString ());
        }
    }
}
