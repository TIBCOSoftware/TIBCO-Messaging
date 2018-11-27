/*
 * Copyright (c) 2013-$Date: 2016-01-25 19:33:46 -0600 (Mon, 25 Jan 2016) $ TIBCO Software Inc.
 * Licensed under a BSD-style license. Refer to [LICENSE]
 * For more information, please contact:
 * TIBCO Software Inc., Palo Alto, California, USA
 *
 * $Id: UpgradeResponse.cs 83756 2016-01-26 01:33:46Z bmahurka $
 *
 */

using System;
using System.IO;
using System.Collections;
using System.Text;
using System.Security.Cryptography;

namespace TIBCO.EFTL
{
    public class UpgradeResponse 
    {
        private String version;
        private int statusCode;
        private  String statusReason;
        private  Hashtable headers;

        public UpgradeResponse(String version, int statusCode, String statusReason)
        {
            this.version = version;
            this.statusCode = statusCode;
            this.statusReason = statusReason;
            this.headers = new Hashtable();
        }
        
        public static UpgradeResponse read(System.IO.Stream stream)
        {
            UpgradeResponse response = null;
            String delimiter = "  ";
            char[] delimiters = delimiter.ToCharArray();

            String delimiter2 = ":  ";
            char[] delimiters2 = delimiter2.ToCharArray();
            
            StreamReader   reader = new StreamReader(stream);

            for (String line; (line = reader.ReadLine()) != null;) 
            {
                if (String.IsNullOrEmpty(line))
                    break;
                if (response == null) {
                    String[] status = line.Split(delimiters, 3);
                    response = new UpgradeResponse(status[0], Int32.Parse(status[1]), status[2]);
                } else {
                    String[] header = line.Split(delimiters2, 2);
                    response.setHeader(header[0].Trim(), header[1].Trim());
                }
            }
            
            if (response == null) {
                throw new IOException("no HTTP response");
            }
            
            return response;
        }
        
        public String getVersion() {
            return version;
        }
        
        public int getStatusCode() {
            return statusCode;
        }
        
        public String getStatusReason() {
            return statusReason;
        }
        
        public String getHeader(String name) {
            foreach(String key in headers.Keys) 
            {
                if (String.Compare(key, name, StringComparison.CurrentCultureIgnoreCase) == 0)
                {
                    return (String)headers[key];
                }
            }
            return null;
        }
        
        public void setHeader(String name, String value) {
            headers.Add(name, value);
        }
        
        public String getProtocol() {
            return getHeader("Sec-WebSocket-Protocol");
        }
        
        public void validate(UpgradeRequest request) 
        {
            // check for 101 - Switching Protocols
            int statusCode = getStatusCode();
            if (statusCode != 101) {
                throw new UpgradeException(statusCode, getStatusReason());
            }
            
            // Connection header must be "upgrade"
            String connection = getHeader("Connection");
            if (String.Compare("upgrade", connection, StringComparison.CurrentCultureIgnoreCase) != 0)
            {
                throw new UpgradeException(statusCode,
                                           "expected Connection header value of [upgrade], received [" + connection + "]");
            }
            
            // Upgrade header must be "websocket"
            String upgrade = getHeader("Upgrade");
            if (String.Compare("websocket", upgrade, StringComparison.CurrentCultureIgnoreCase) != 0) {
                throw new UpgradeException(statusCode,
                                           "expected Upgrade header value of [websocket], received [" + upgrade + "]");
            }
            
            // check for properly hashed request key
            String key = request.getKey();
            String keyHash = getHeader("Sec-WebSocket-Accept");
           
            /*String expectedKeyHash = generateKeyHash(key);
            if (String.Compare(expectedKeyHash, keyHash) != 0) {
                throw new UpgradeException(statusCode,
                                           "invalid Sec-WebSocket-Accept value");
            }*/
        }
        
        /*private static String generateKeyHash(String key) {
            try 
            {
                SHA1 sha1 = SHA1Managed.Create();
                byte[] inputBytes = Encoding.ASCII.GetBytes(key + "258EAFA5-E914-47DA-95CA-C5AB0DC85B11");
                byte[] outputBytes = sha1.ComputeHash(inputBytes);
                return Convert.ToBase64String(outputBytes).Trim();
            }
            catch (Exception) 
            {
                throw new Exception("failed to generate key hash");
            }
        }*/
        
        public String toString() 
        {
            StringBuilder sbuilder = new StringBuilder(256);
            
            sbuilder.Append(version).Append(" ");
            sbuilder.Append(statusCode).Append(" ");
            sbuilder.Append(statusReason).Append("\r\n");
            
            foreach (String key in headers.Keys) 
            {
                sbuilder.Append(key).Append(": ");
                sbuilder.Append(getHeader(key)).Append("\r\n");
            }
            
            sbuilder.Append("\r\n");
            
            return sbuilder.ToString();
        }
    }
}

