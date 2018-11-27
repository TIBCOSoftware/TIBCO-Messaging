/* 
 * Copyright (c) 2001-$Date: 2017-03-14 21:55:39 -0500 (Tue, 14 Mar 2017) $ TIBCO Software Inc. 
 * Licensed under a BSD-style license. Refer to [LICENSE]
 * For more information, please contact:
 * TIBCO Software Inc., Palo Alto, California, USA
 * 
 * $Id: tibjmsUtilities.java 92322 2017-03-15 02:55:39Z kshelham $
 * 
 */

/*
 * This sample uses JNDI to retrieve administered objects.
 *
 * Optionally all parameters hardcoded in this sample can be
 * read from the jndi.properties file.
 *
 * This file also contains an SSL parameter helper class to enable
 * an SSL connection to a TIBCO Enterprise Message Service server.
 *
 */

import javax.jms.*;
import javax.naming.*;
import java.util.Hashtable;
import java.util.Vector;
import java.security.*;


public class tibjmsUtilities
{
    static Context jndiContext = null;

    static final String  providerContextFactory =
                            "com.tibco.tibjms.naming.TibjmsInitialContextFactory";

    static final String  defaultProtocol = "tibjmsnaming";

    static final String  defaultProviderURL =
                            defaultProtocol + "://localhost:7222";

    public static void initJNDI(String providerURL) throws NamingException
    {
        initJNDI(providerURL,null,null);
    }

    public static void initJNDI(String providerURL, String userName,
        String password) throws NamingException
    {
        if (jndiContext != null)
            return;

        if (providerURL == null || (providerURL.length() == 0))
            providerURL = defaultProviderURL;

        try
        {
            Hashtable<String,String> env = new Hashtable<String,String>();

            env.put(Context.INITIAL_CONTEXT_FACTORY,providerContextFactory);
            env.put(Context.PROVIDER_URL,providerURL);

            if (userName != null)
            {
                env.put(Context.SECURITY_PRINCIPAL,userName);

                if (password != null)
                    env.put(Context.SECURITY_CREDENTIALS,password);
            }

            jndiContext = new InitialContext(env);
        }
        catch (NamingException e)
        {
            System.out.println("Failed to create JNDI InitialContext with provider URL set to "+
                                providerURL+", error = "+e.toString());
            throw e;
        }
    }

    public static Object lookup(String objectName) throws NamingException
    {
        if (objectName == null)
            throw new IllegalArgumentException("null object name not legal");

        if (objectName.length() == 0)
            throw new IllegalArgumentException("empty object name not legal");

        /*
         * check if not initialized, then initialize
         * with default parameters
         */
        initJNDI(null);

        /*
         * do the lookup
         */
        return jndiContext.lookup(objectName);
    }


    public static void initSSLParams(String serverUrl,String[] args)
        throws JMSSecurityException
    {
        if (serverUrl != null && serverUrl.indexOf("ssl://") >= 0)
        {
           SSLParams ssl = new SSLParams(args);
           ssl.init();
        }
    }

    public static void sslUsage()
    {
        System.err.println("\nSSL options:");
        System.err.println("");
        System.err.println(" -ssl_vendor               <name>      - SSL vendor: 'j2se' or 'entrust6'");
        System.err.println(" -ssl_trace                            - trace SSL initialization");
        System.err.println(" -ssl_vendor_trace                     - trace SSL handshake and related");
        System.err.println(" -ssl_trusted[n]           <file-name> - file with trusted certificates,");
        System.err.println("                                         this parameter may repeat if more");
        System.err.println("                                         than one file required");
        System.err.println(" -ssl_verify_hostname                  - do not verify certificate name.");
        System.err.println("                                         (this disabled by default)");
        System.err.println(" -ssl_expected_hostname    <string>    - expected name in the certificate");
        System.err.println(" -ssl_custom                           - use custom verifier (it shows names");
        System.err.println("                                         always approves them).");
        System.err.println(" -ssl_identity             <file-name> - client identity file");
        System.err.println(" -ssl_issuer[n]            <file-name> - client issuer file");
        System.err.println(" -ssl_private_key          <file-name> - client key file (optional)");
        System.err.println(" -ssl_password             <string>    - password to decrypt client identity");
        System.err.println("                                         or key file");
        System.err.println(" -ssl_ciphers              <suite-name(s)> - cipher suite names, colon separated");
        System.exit(0);
    }

    private static class SSLParams
        implements com.tibco.tibjms.TibjmsSSLHostNameVerifier
    {
        String          ssl_vendor                  = null;
        boolean         ssl_trace                   = false;
        boolean         ssl_debug_trace             = false;
        boolean         ssl_verify_hostname         = false;
        String          ssl_expected_hostname       = null;
        Vector<String>  ssl_trusted                 = null;
        Vector<String>  ssl_issuers                 = null;
        String          ssl_identity                = null;
        String          ssl_private_key             = null;
        String          ssl_password                = null;
        boolean         ssl_custom                  = false;
        String          ssl_ciphers                 = null;

        public SSLParams(String[] args)
        {
            int     trusted_pi      = 0;
            String  trusted_suffix  = "";
            int     issuer_pi      = 0;
            String  issuer_suffix  = "";

            int i=0;
            while (i < args.length)
            {
                if (args[i].compareTo("-ssl_vendor")==0)
                {
                    if ((i+1) >= args.length) sslUsage();
                    ssl_vendor = args[i+1];
                    i += 2;
                }
                else
                if (args[i].compareTo("-ssl_trace")==0)
                {
                    ssl_trace = true;
                    i += 1;
                }
                else
                if (args[i].compareTo("-ssl_debug_trace")==0)
                {
                    ssl_debug_trace = true;
                    i += 1;
                }
                else
                if (args[i].compareTo("-ssl_expected_hostname")==0)
                {
                    if ((i+1) >= args.length) sslUsage();
                    ssl_expected_hostname = args[i+1];
                    i += 2;
                }
                else
                if (args[i].compareTo("-ssl_verify_hostname")==0)
                {
                    ssl_verify_hostname = true;
                    i += 1;
                }
                else
                if (args[i].compareTo("-ssl_custom")==0)
                {
                    ssl_custom = true;
                    i += 1;
                }
                else
                if (args[i].compareTo("-ssl_ciphers")==0)
                {
                    if ((i+1) >= args.length) sslUsage();
                    ssl_ciphers = args[i+1];
                    i += 2;
                }
                else
                if (args[i].compareTo("-ssl_identity")==0)
                {
                    if ((i+1) >= args.length) sslUsage();
                    ssl_identity = args[i+1];
                    i += 2;
                }
                else
                if (args[i].compareTo("-ssl_private_key")==0)
                {
                    if ((i+1) >= args.length) sslUsage();
                    ssl_private_key = args[i+1];
                    i += 2;
                }
                else
                if (args[i].compareTo("-ssl_password")==0)
                {
                    if ((i+1) >= args.length) sslUsage();
                    ssl_password = args[i+1];
                    i += 2;
                }
                else
                if (args[i].compareTo("-ssl_trusted"+trusted_suffix)==0)
                {
                    if ((i+1) >= args.length) sslUsage();
                    String cert = args[i+1];
                    if (cert == null) continue;
                    if (ssl_trusted == null)
                        ssl_trusted = new Vector<String>();
                    ssl_trusted.addElement(cert);
                    trusted_pi++;
                    trusted_suffix = String.valueOf(trusted_pi);
                    i += 2;
                }
                else
                if (args[i].compareTo("-ssl_issuer"+issuer_suffix)==0)
                {
                    if ((i+1) >= args.length) sslUsage();
                    String cert = args[i+1];
                    if (cert == null) continue;
                    if (ssl_issuers == null)
                        ssl_issuers = new Vector<String>();
                    ssl_issuers.addElement(cert);
                    issuer_pi++;
                    issuer_suffix = String.valueOf(issuer_pi);
                    i += 2;
                }
                else
                {
                    i++;
                }
            }
        }

        public void init() throws JMSSecurityException
        {
            if (ssl_trace)
                com.tibco.tibjms.TibjmsSSL.setClientTracer(System.err);

            if (ssl_debug_trace)
                com.tibco.tibjms.TibjmsSSL.setDebugTraceEnabled(true);

            if (ssl_vendor != null)
                com.tibco.tibjms.TibjmsSSL.setVendor(ssl_vendor);

            if (ssl_expected_hostname != null)
                com.tibco.tibjms.TibjmsSSL.setExpectedHostName(ssl_expected_hostname);

            if (ssl_custom)
                com.tibco.tibjms.TibjmsSSL.setHostNameVerifier(this);

            if (!ssl_verify_hostname)
                com.tibco.tibjms.TibjmsSSL.setVerifyHostName(false);

            if (ssl_trusted != null)
            {
                for (int i=0; i<ssl_trusted.size(); i++)
                {
                    String certfile = ssl_trusted.elementAt(i);
                    com.tibco.tibjms.TibjmsSSL.addTrustedCerts(certfile);
                }
            }
            else
            {
                com.tibco.tibjms.TibjmsSSL.setVerifyHost(false);
            }

            if (ssl_issuers != null)
            {
                for (int i=0; i<ssl_issuers.size(); i++)
                {
                    String certfile = ssl_issuers.elementAt(i);
                    com.tibco.tibjms.TibjmsSSL.addIssuerCerts(certfile);
                }
            }

            if (ssl_identity != null)
            {
                com.tibco.tibjms.TibjmsSSL.setIdentity(
                    ssl_identity,ssl_private_key,
                    ssl_password.toCharArray());
            }
            else if (ssl_password != null)
            {
                com.tibco.tibjms.TibjmsSSL.setIdentity(
                    null,null,ssl_password.toCharArray());
            }

            if (ssl_ciphers != null)
                com.tibco.tibjms.TibjmsSSL.setCipherSuites(ssl_ciphers);
        }

        public void verifyHostName(String connectedHostName,
                                   String expectedHostName,
                                   String certificateCN,
                                   java.security.cert.X509Certificate server_certificate)
                    throws JMSSecurityException
        {
            System.err.println("HostNameVerifier: "+
                    "    connected = ["+connectedHostName+"]\n"+
                    "    expected  = ["+expectedHostName+"]\n"+
                    "    certCN    = ["+certificateCN+"]");

            return;
        }
    }
}



