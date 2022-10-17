/* 
 * Copyright (c) 2001-$Date: 2016-12-13 17:00:37 -0600 (Tue, 13 Dec 2016) $ TIBCO Software Inc. 
 * Licensed under a BSD-style license. Refer to [LICENSE]
 * For more information, please contact:
 * TIBCO Software Inc., Palo Alto, California, USA
 * 
 * $Id: csLookupLDAP.cs 90180 2016-12-13 23:00:37Z olivierh $
 * 
 */

/// <summary>
/// Example of how to lookup an administered object in LDAP server from a
/// TIBCO EMS client.
/// 
/// NOTE: these objects should already exist in the LDAP server. The objects
/// should be stored as xml representation. Please see the TIBCO EMS java client
/// tibemsSchema.ldif for more details on the XML schema for the administered
/// objects.
///
///  Usage:  csLookupLdap -ldapURL <ldapURL> -user <ldapUserName> -password <password>
///          -ldapProtocol <ldap|ldaps> -topicName <topicName>
///          -queueName <queueName> -factoryName <factoryName>
///          -ldapBaseDN <baseDN>
///  e.g.   csLookupLdap.exe -ldapURL qawin2k3.na.tibco.com:6199 -user cn=Manager 
///         -password password -ldapProtocol ldap -topicName cn=testTopic 
///         -queueName cn=testQueue -factoryName cn=connFactory -ldapBaseDN dc=test2
///
/// </summary>

using System;
using System.Collections;
using System.DirectoryServices;
using System.IO;
using System.Xml;
using System.Xml.Schema;
using System.Collections.Specialized;
using TIBCO.EMS;
using System.Threading;


public class csLookupLDAP
{
    string ldapURL        = null;
    string ldapPrincipal  = null;
    string ldapPassword   = null;
    string ldapBaseDN     = null;
    string ldapProtocol   = null;
    string certName       = null;
    string searchScope    = null;
    string storeName      = null;
    string storeLocation  = null;
    string topicName      = null;
    string queueName      = null;
    string factoryName    = null;
    
    private void parseArgs(String[] args) 
    {
        int i = 0;
        
        if (args.Length < 1) 
        {
            usage();
            System.Environment.Exit(1);
        }
        
        while (i < args.Length)
        {
            if (args[i].CompareTo("-ldapURL") == 0) {
                if ((i + 1) >= args.Length)
                    usage();
                ldapURL = args[i + 1];
                i += 2;
            } else if (args[i].CompareTo("-user") == 0) {
                if ((i + 1) >= args.Length)
                    usage();
                ldapPrincipal = args[i + 1];
                i += 2;
            } else if (args[i].CompareTo("-password") == 0) {
                if ((i + 1) >= args.Length)
                    usage();
                ldapPassword = args[i + 1];
                i += 2;
            } else if (args[i].CompareTo("-ldapBaseDN") == 0) {
                if ((i + 1) >= args.Length)
                    usage();
                ldapBaseDN = args[i + 1];
                i += 2;
            } else if (args[i].CompareTo("-ldapProtocol") == 0) {
                if ((i + 1) >= args.Length)
                    usage();
                ldapProtocol = args[i + 1];
                i += 2;
            } else if (args[i].CompareTo("-searchScope") == 0) {
                if ((i + 1) >= args.Length)
                    usage();
                searchScope = args[i + 1];
                i += 2;
            } else if (args[i].CompareTo("-certName") == 0) {
                if ((i + 1) >= args.Length)
                    usage();
                certName = args[i + 1];
                i += 2;
            } else if (args[i].CompareTo("-storeName") == 0) {
                if ((i + 1) >= args.Length)
                    usage();
                storeName = args[i + 1];
                i += 2;
            } else if (args[i].CompareTo("-storeLocation") == 0) {
                if ((i + 1) >= args.Length)
                    usage();
                storeLocation = args[i + 1];
                i += 2;
            } else if (args[i].CompareTo("-topicName") == 0) {
                if ((i + 1) >= args.Length)
                    usage();
                topicName = args[i + 1];
                i += 2;
            } else if (args[i].CompareTo("-queueName") == 0) {
                if ((i + 1) >= args.Length)
                    usage();
                queueName = args[i + 1];
                i += 2;
            } else if (args[i].CompareTo("-factoryName") == 0) {
                if ((i + 1) >= args.Length)
                    usage();
                factoryName = args[i + 1];
                i += 2;
            } else if (args[i].CompareTo("-help") == 0) {
                usage();
            } else {
                Console.Error.WriteLine("Unrecognized parameter: " + args[i]);
                usage();
            }
        }
    }

    void usage() 
    {
        Console.WriteLine("\nUsage: csLookupLDAP [options]");
        Console.WriteLine("");
        Console.WriteLine("  where options are:");
        Console.WriteLine("");
        Console.WriteLine("  -ldapURL      <ldapURL>        - url to ldap server");
        Console.WriteLine("  -user         <user name>      - user name, default is null");
        Console.WriteLine("  -password     <password>       - password, default is null");
        Console.WriteLine("  -ldapBaseDN   <ldapBaseDN>     - base dn ");
        Console.WriteLine("  -ldapProtocol <protocol>       - either 'ldap' or 'ldaps'");
        Console.WriteLine("  -searchScope  <searchscope>    - either 'subtree' or 'onelevel'");
        Console.WriteLine("  -certName     <ssl_cert_name>  - Name of the SSL certificate if protocol is 'ldaps'");
        Console.WriteLine("  -storeName    <store_name>     - Name of the certficate store, where the certificate with certName can be found");
        Console.WriteLine("  -storeLocation <storeLocation> - Location where the storeName can be found");
        Console.WriteLine("  -topicName     <topicName>     - topic to look for in the LDAP server");
        Console.WriteLine("  -queueName     <queueName>     - queue to look for in the LDAP server");
        Console.WriteLine("  -factoryName   <factoryName>   - factory to look for in the LDAP server");

        Environment.Exit(1);
    }

    public csLookupLDAP(string[] args)
    {
        parseArgs(args);
    }

    public static void Main (string []args) {
        try  {
            csLookupLDAP t = new csLookupLDAP(args);
            t.lookup();
        }
        catch (Exception e) 
        {
            Console.WriteLine(e.Message);
            Console.WriteLine(e.StackTrace);
        }
    }

    private void lookup()
    {
        Hashtable table = new Hashtable();
        
        if (ldapURL != null)
            table.Add(LdapLookupConsts.LDAP_SERVER_URL, ldapURL);

        if (ldapBaseDN != null)
            table.Add(LdapLookupConsts.LDAP_BASE_DN, ldapBaseDN);

        if (ldapPrincipal != null)
            table.Add(LdapLookupConsts.LDAP_PRINCIPAL, ldapPrincipal);
        else 
            table.Add(LdapLookupConsts.LDAP_PRINCIPAL, "");

        if (ldapPassword != null)
            table.Add(LdapLookupConsts.LDAP_CREDENTIAL, ldapPassword);
        else
            table.Add(LdapLookupConsts.LDAP_CREDENTIAL, "");

        if (searchScope != null)
            table.Add(LdapLookupConsts.LDAP_SEARCH_SCOPE, searchScope);

        if (ldapProtocol != null) 
        {
            if (ldapProtocol.Equals("ldaps")) 
            {
                table.Add(LdapLookupConsts.LDAP_CONN_TYPE, "ldaps");
                
                if (certName != null)
                    table.Add(LdapLookupConsts.LDAP_CERT_NAME, certName);
                if (storeName != null)
                    table.Add(LdapLookupConsts.LDAP_CERT_STORE_NAME, storeName);
                if (storeLocation != null)
                    table.Add(LdapLookupConsts.LDAP_CERT_STORE_LOCATION, storeLocation);

                LdapLookupSSLParams parms = new LdapLookupSSLParams();

                table.Add(LdapLookupConsts.LDAP_SSL_PARAMS, parms);
            }
            else if (ldapProtocol.Equals("ldap")) 
            {
                // nothing
            }
            else 
            {
                Console.WriteLine("invalid protocol name: only ldap or ldaps supported");
                System.Environment.Exit(-1);
            }
        }

        LookupContextFactory fc = new LookupContextFactory();
        ILookupContext searcher = fc.CreateContext(LookupContextFactory.LDAP_CONTEXT, table);

        // now lookup
        if (factoryName != null)
            LookupFactory(searcher, factoryName);
        if (topicName != null)
            LookupTopic(searcher, topicName);
        if (queueName != null)
            LookupQueue(searcher, queueName);
    }

    private void LookupFactory(ILookupContext searcher, String name)
    {
        ConnectionFactory cf = (ConnectionFactory)searcher.Lookup(name);
        if (cf != null) 
        {
            Console.WriteLine("");
            Console.WriteLine("######### Connection Factory ###########");
            Console.WriteLine(cf.ToString());
            Console.WriteLine("#########################################");
        }
    }

    private void LookupTopic(ILookupContext searcher, String name)
    {
        Topic topic = (Topic)searcher.Lookup(name);
        if (topic != null) 
        {
            Console.WriteLine("");
            Console.WriteLine("########## Topic ######### ");
            Console.WriteLine(topic.ToString());
            Console.WriteLine("########################## ");
        }
    }

    private void LookupQueue(ILookupContext searcher, String name)
    {
        TIBCO.EMS.Queue queue = (TIBCO.EMS.Queue)searcher.Lookup(name);
        if (queue != null) 
        {
            Console.WriteLine("");
            Console.WriteLine("########## Topic ######### ");
            Console.WriteLine(queue.ToString());
            Console.WriteLine("########################## ");
        }
    }
}

