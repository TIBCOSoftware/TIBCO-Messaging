/* 
 * Copyright (c) 2001-$Date: 2016-12-13 17:00:37 -0600 (Tue, 13 Dec 2016) $ TIBCO Software Inc. 
 * Licensed under a BSD-style license. Refer to [LICENSE]
 * For more information, please contact:
 * TIBCO Software Inc., Palo Alto, California, USA
 * 
 * $Id: emsSSLSampleClient.cs 90180 2016-12-13 23:00:37Z olivierh $
 * 
 */

/*
 * This is a simple sample of SSL connectivity for
 * EMS .NET clients.  It demonstrates ssl-based client/server communications
 * using global SSL settings.
 *
 * This sample establishes an SSL connection, publishes and
 * receives back three messages.
 *
 *      -server     Server URL passed to the ConnectionFactory
 *                  constructor.
 *                  If not specified this sample assumes a
 *                  serverUrl of "ssl://localhost:7243"
 *
 *
 * NOTE: The makecert tool from .NET 2.0 framework is required.
 *
 * Example usage:
 * creating a self signed certificate
 *    This creates a SampleAuthority and added it to the currentuser store location
 *       makecert -pe -n "CN=SampleAuthority" -ss My -sr CurrentUser -a sha1 -sky Signature -r "SampleAuthority.cer" 
 *    This creates a certificate signed by the Sample Authority
 *       makecert -pe -ss my -sr CurrentUser -a sha1 -sky exchange -in "SampleAuthority" -is My -ir CurrentUser -sp "Microsoft RSA Schannel Cryptographic Provider" -sy 12 -n "CN=client-sample-cert" -b 11/15/2005 -e 01/01/2099 -eku 1.3.6.1.5.5.7.3.2 Client.cer
 * 
 * You will need to convert the DER encoded file to base64 encoded file to add
 * it to the client_root.cert.pem file of the EMS server's certificate store.
 * One way to achieve this is to.
 * Open the Microsoft management console and import the SampleAuthority certificate into
 * a base64 encoded file and open this file and copy everything betweeen
 * -----BEGIN CERTIFICATE----- and -----END CERTIFICATE----- into the EMS server's
 * client_root.cert.pem file. 
 *
 *
 *  emsSSLSampleClient -server ssl://localhost:7243 -ssl_target_hostname <hostname>
 *                  -ssl_cert_store_location currentuser -ssl_cert_store_name My -ssl_cert_name "CN=client-sample-cert"
 */
 
using System;
using TIBCO.EMS;
using System.Collections;
using System.Net.Security;
using System.Security.Cryptography;
using System.Security.Cryptography.X509Certificates;


public class emsSSLSampleClient
{
    String      serverUrl       = "ssl://localhost:7243";
    String      userName        = null;
    String      password        = null;
    String      topicName       = "topic.sample";

    // SSL options
    bool        ssl_trace                 = false;
    String      ssl_target_hostname       = null;
    String      ssl_cert_store_location   = null;
    String      ssl_cert_store_name       = null;
    String      ssl_cert_name             = null;

    public emsSSLSampleClient(String[] args) 
    {
        parseArgs(args);

        if (topicName == null) {
            System.Console.WriteLine("Error: must specify topic name");
            usage();
        }

        System.Console.WriteLine("Global SSL parameters sample with Microsoft Cerrtificate Store.");

        try {
            EMSSSLSystemStoreInfo storeInfo = new EMSSSLSystemStoreInfo();

            // set trace for client-side operations, loading of certificates
            // and other
            if (ssl_trace) {
                EMSSSL.SetClientTracer(new System.IO.StreamWriter(System.Console.OpenStandardError()));
            }

            // set target host name in the sertificate if specified
            if (ssl_target_hostname != null) {
                EMSSSL.SetTargetHostName(ssl_target_hostname);
            }

            if (ssl_cert_store_location != null) {
                if (ssl_cert_store_location.Equals("currentuser")) {
                    storeInfo.SetCertificateStoreLocation(StoreLocation.CurrentUser);
                }
                else if (ssl_cert_store_location.Equals("localmachine")) {
                    storeInfo.SetCertificateStoreLocation(StoreLocation.LocalMachine);
                }
            }

            if (ssl_cert_store_name != null) {
                storeInfo.SetCertificateStoreName(ssl_cert_store_name);
            }
            if (ssl_cert_name != null) {
                storeInfo.SetCertificateNameAsFullSubjectDN(ssl_cert_name);
            }

            EMSSSL.SetCertificateStoreType(EMSSSLStoreType.EMSSSL_STORE_TYPE_SYSTEM, storeInfo);
        }
        catch(Exception e) 
        {
            System.Console.WriteLine(e.StackTrace);

            if (e is EMSException) {
                EMSException je = (EMSException)e;

                if (je.LinkedException != null) {
                    System.Console.WriteLine("##### Linked Exception:");
                    System.Console.WriteLine(je.LinkedException.StackTrace);
                }
            }
            System.Environment.Exit(-1);
        }

        try
        {
            ConnectionFactory factory = new ConnectionFactory(serverUrl);

            Connection connection = factory.CreateConnection(userName,password);

            Session session = connection.CreateSession(false,TIBCO.EMS.SessionMode.AutoAcknowledge);

            Topic topic = session.CreateTopic(topicName);

            MessageProducer  publisher  = session.CreateProducer(topic);
            MessageConsumer  subscriber = session.CreateConsumer(topic);

            connection.Start();

            MapMessage message = session.CreateMapMessage();
            message.SetStringProperty("field","SSL message");

            for (int i=0; i<3; i++) 
            {
                publisher.Send(message);
                System.Console.WriteLine("\nPublished message: "+message);

                /* read same message back */
                message = (MapMessage)subscriber.Receive();
                if (message == null)
                    System.Console.WriteLine("\nCould not receive message");
                else
                    System.Console.WriteLine("\nReceived message: "+message);

                try { 
                    System.Threading.Thread.Sleep(1000); 
                }
                catch(Exception){}
            }

            connection.Close();
        }
        catch(EMSException e)
        {
            System.Console.WriteLine("##### Exception:" + e.Message);
            System.Console.WriteLine(e.StackTrace);

            if (e.LinkedException != null) {
                System.Console.WriteLine("##### Linked Exception error msg:" + e.LinkedException.Message);
                System.Console.WriteLine("##### Linked Exception:");
                System.Console.WriteLine(e.LinkedException.StackTrace);
            }
            System.Environment.Exit(-1);
        }
    }

    public static void Main (string []args) 
    {
        emsSSLSampleClient t = new emsSSLSampleClient(args);
    }

    void usage()
    {
        System.Console.WriteLine("\nUsage:EMSSSLGlobal [options]");
        System.Console.WriteLine("");
        System.Console.WriteLine("   where options are:");
        System.Console.WriteLine("");
        System.Console.WriteLine(" -server        <server URL>   - EMS server URL, default is ssl://localhost:7243");
        System.Console.WriteLine(" -user          <user name>    - user name, default is null");
        System.Console.WriteLine(" -password      <password>     - password, default is null");
        System.Console.WriteLine(" -topic         <topic-name>   - topic name, default is \"topic.sample\"");
        System.Console.WriteLine("");
        System.Console.WriteLine("  SSL options:");
        System.Console.WriteLine("");
        System.Console.WriteLine(" -ssl_trace                         - trace SSL initialization");
        System.Console.WriteLine(" -ssl_target_hostname  <host-name>  - name in the server certificate");
        System.Console.WriteLine(" -ssl_cert_store_location    <location>   - system store where location, currentuser or localmachine"); 
        System.Console.WriteLine(" -ssl_cert_store_name  <store_name> - name of the store at the store location");
        System.Console.WriteLine(" -ssl_cert_name        <cert-name>  - name of the certificate");
        System.Environment.Exit(-1);
    }

    void parseArgs(String[] args)
    {
        int i=0;

        while(i < args.Length)
        {
            if (args[i].CompareTo("-server")==0)
            {
                if ((i+1) >= args.Length) usage();
                serverUrl = args[i+1];
                i += 2;
            }
            else
            if (args[i].CompareTo("-topic")==0)
            {
                if ((i+1) >= args.Length) usage();
                topicName = args[i+1];
                i += 2;
            }
            else
            if (args[i].CompareTo("-user")==0)
            {
                if ((i+1) >= args.Length) usage();
                userName = args[i+1];
                i += 2;
            }
            else
            if (args[i].CompareTo("-password")==0)
            {
                if ((i+1) >= args.Length) usage();
                password = args[i+1];
                i += 2;
            }
            else
            if (args[i].CompareTo("-ssl_trace")==0)
            {
                ssl_trace = true;
                i += 1;
            }
            else
            if (args[i].CompareTo("-ssl_target_hostname")==0)
            {
                if ((i+1) >= args.Length) usage();
                ssl_target_hostname = args[i+1];
                i += 2;
            }
            else
            if (args[i].CompareTo("-ssl_cert_store_location")==0)
            {
                if ((i+1) >= args.Length) usage();
                ssl_cert_store_location = args[i+1];
                i += 2;
            }
            else
            if (args[i].CompareTo("-ssl_cert_store_name")==0)
            {
                if ((i+1) >= args.Length) usage();
                ssl_cert_store_name = args[i+1];
                i += 2;
            }
            else
            if (args[i].CompareTo("-ssl_cert_name")==0)
            {
                if ((i+1) >= args.Length) usage();
                ssl_cert_name = args[i+1];
                i += 2;
            }
            else
            if (args[i].CompareTo("-help")==0)
            {
                usage();
            }
            else
            {
                System.Console.WriteLine("Unrecognized parameter: "+args[i]);
                usage();
            }
        }
    }
}


