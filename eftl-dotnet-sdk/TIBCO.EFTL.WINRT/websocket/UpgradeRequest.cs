/*
 * Copyright (c) 2013-$Date: 2016-01-25 19:33:46 -0600 (Mon, 25 Jan 2016) $ TIBCO Software Inc.
 * Licensed under a BSD-style license. Refer to [LICENSE]
 * For more information, please contact:
 * TIBCO Software Inc., Palo Alto, California, USA
 *
 * $Id: UpgradeRequest.cs 83756 2016-01-26 01:33:46Z bmahurka $
 *
 */

using System;
using System.Text;
using System.Collections.Generic;

namespace TIBCO.EFTL
{
    public class UpgradeRequest 
    {
        private Uri uri;
        private String[] protocols;
        private String key;
        
        private static Random random = new Random();
    
        private static String generateKey() 
        {
            byte[] bytes = new byte[16];
            random.NextBytes(bytes);
            return Convert.ToBase64String(bytes);
        }
    
        public UpgradeRequest(Uri uri, ArrayList protocols) 
        {
            this.uri = uri;
            try {
                this.protocols = new String[protocols.Count];
                for(int i = 0; i < protocols.Count; i++) {
                    this.protocols[i] = (String)protocols[i];
                }
            }
            catch(Exception) { }

            try {
                this.key = generateKey();
            } catch (Exception) {}

        }
        
        public String getKey() 
        {
            return key;
        }
        
        public void write(System.IO.Stream stream)
        {
            byte[] b = Encoding.UTF8.GetBytes(this.toString());
            stream.Write(b, 0, b.Length);
            stream.Flush();
        }
        
        public String toString() 
        {
            StringBuilder request = new StringBuilder(256);
            
            request.Append("GET ");

            if (String.IsNullOrEmpty(uri.AbsolutePath))
                request.Append("/");
            else 
                request.Append(uri.AbsolutePath);

            request.Append(" HTTP/1.1\r\n");
            
            request.Append("Host: ").Append(uri.Host);
            if (uri.Port > 0) {
                request.Append(":").Append(uri.Port);
            }
            request.Append("\r\n");
            
            request.Append("Upgrade: websocket\r\n");
            request.Append("Connection: Upgrade\r\n");
            request.Append("Sec-WebSocket-Key: ").Append(getKey()).Append("\r\n");
            request.Append("Sec-WebSocket-Version: 13\r\n");
            
            foreach (String protocol in protocols) {
                request.Append("Sec-WebSocket-Protocol: ").Append(protocol).Append("\r\n");
            }
            
            request.Append("\r\n");
            
            return request.ToString();
        }
    }
}
