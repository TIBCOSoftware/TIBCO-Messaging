/*
 * Copyright (c) 2009-$Date$ Cloud Software Group, Inc.
 * Licensed under a BSD-style license. Refer to [LICENSE]
 *
 * $Id$
 */

using System;
using System.Collections;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading;
using System.Threading.Tasks;
using System.Net;
using System.Net.Sockets;
using System.Net.Security;
using System.Security;
using System.Security.Cryptography;
using System.Security.Cryptography.X509Certificates;
using System.IO;
using System.Security.Authentication;
using System.Security.Permissions;
using System.Security.Principal;
using System.Configuration;
using System.Net.WebSockets;

namespace TIBCO.EFTL
{
    internal class WebSocket
    {
        private Uri                uri; 
        private WebSocketListener  listener;
        long                       state;
        private ArrayList          protocols;
        private TimeSpan           timeout;
        System.Net.WebSockets.ClientWebSocket        clientWebSocket = null;
        System.Net.WebSockets.ClientWebSocketOptions options;

        long INIT       = 0;
        long CONNECTING = 1;
        long OPEN       = 2;
        long CLOSING    = 3;
        long CLOSED     = 4;
        ServicePoint sp;

        // internal API /////////////////////////////////////////////////////////////
        internal WebSocket(Uri uri, WebSocketListener listener) 
        {
            bool ignoreCase = true;

            this.uri       = uri;
            this.listener  = listener;

            this.protocols = new ArrayList();

            if ((String.Compare("ws", uri.Scheme, ignoreCase) != 0) && 
                (String.Compare("wss", uri.Scheme, ignoreCase) != 0))
            {
                throw new Exception("URI scheme must be 'ws' or 'wss'");
            }

            clientWebSocket   = new ClientWebSocket();
            options           = clientWebSocket.Options;

            // turn off pongs from client to server.
            options.KeepAliveInterval = new System.TimeSpan(0);
        }

        internal void setUserInfo(String username, String password)
        {
            if (!String.IsNullOrEmpty(username) || !String.IsNullOrEmpty(password))
            {
                String auth = String.Format("{0}:{1}", username != null ? username : "",
                                            password != null ? password : "");
                String auth64 = Convert.ToBase64String(Encoding.UTF8.GetBytes(auth));

                options.SetRequestHeader("Authorization", String.Format("Basic {0}", auth64));
            }
        }

        private static bool ignoreCert(Object sender, X509Certificate cert, X509Chain chain, SslPolicyErrors sslPolicyErrors)
        {
            return true;
        }

        private static bool verifyCert(Object sender, X509Certificate cert, X509Chain chain, SslPolicyErrors sslPolicyErrors)
        {
            return (sslPolicyErrors == SslPolicyErrors.None);
        }

        internal void setTrustAll(bool trustAll)
        {
            // options.RemoteCertificateValidationCallback requires a higher runtime version
        }

        internal void setProtocol(String protocol) 
        {
            this.protocols.Add(protocol);
            options.AddSubProtocol(protocol);
        }

        internal void setTimeout(double seconds) 
        {
            this.timeout = TimeSpan.FromSeconds(seconds);
        }

        internal void Open() 
        {
            if (Interlocked.CompareExchange(ref state, CONNECTING, INIT) == INIT)
                connect();
        }
    
        internal void close() {
            close(1000);
        }

        internal void close(int code) {
            if (Interlocked.CompareExchange(ref state, CLOSED, CONNECTING) == CONNECTING)
                disconnect();
            else if (Interlocked.CompareExchange(ref state, CLOSING, OPEN) == OPEN)
                disconnect(code);
        }

        internal void setState()
        {
            if (Interlocked.Read(ref state) == CONNECTING)
                Interlocked.CompareExchange(ref state, CLOSED, CONNECTING);
            else if (Interlocked.Read(ref state) == OPEN)
                Interlocked.CompareExchange(ref state, CLOSING, OPEN);
        }

        internal void forceClose() {
            disconnect();
        }

        internal bool IsClientWebSocketClosed()
        {
            return (clientWebSocket.State == WebSocketState.Closed);
        }

        private void disconnect() 
        {
            try {
                clientWebSocket.CloseOutputAsync(WebSocketCloseStatus.Empty, null, CancellationToken.None);
            } 
            catch (Exception) {
                // ignore
            }
        }

        private void disconnect(int code) {
            try {
                clientWebSocket.CloseOutputAsync((WebSocketCloseStatus)code, "closing", CancellationToken.None);
            } 
            catch (IOException) {
                // ignore
            }
        }

        internal void send(String text) {
            if (!isConnected())
                throw new Exception("WebSocket is not open");

            try
            {
                byte[] buffer = Encoding.UTF8.GetBytes(text);
                ArraySegment<byte> segment = new ArraySegment<byte>(buffer);
                System.Threading.Tasks.Task t = clientWebSocket.SendAsync(segment, System.Net.WebSockets.WebSocketMessageType.Text, true, CancellationToken.None);
                t.Wait();
            }
            catch (System.Net.WebSockets.WebSocketException)
            {
                //TODO:
            }
        }

        internal void send(byte[] data) {
            if (!isConnected())
                throw new Exception("WebSocket is not open");

            try
            {
                ArraySegment<byte> segment = new ArraySegment<byte>(data);
                clientWebSocket.SendAsync(segment, System.Net.WebSockets.WebSocketMessageType.Binary, true, CancellationToken.None).Wait();
            }
            catch (System.Net.WebSockets.WebSocketException e)
            {
                Console.WriteLine(e.StackTrace);
            }
        }

        internal bool isConnected() 
        {
            return (Interlocked.Read(ref state) == OPEN);
        }


        // Implementation /////////////////////////////////////////////////////////

        private void notifyOpen() 
        {
            try 
            {
                long currentState = Interlocked.Read(ref state);

                if (currentState == CONNECTING)
                {
                    if (Interlocked.CompareExchange(ref state, OPEN, currentState) == currentState)
                        listener.OnOpen();
                }
            }
            catch (Exception e) {
                // discard exceptions thrown by the listener
                notifyError(e);
            }
        }
    
        private void notifyClose(int code, String reason) 
        {
            try {
                long currentState = Interlocked.Read(ref state);

                if (currentState == CLOSING) 
                {
                    if (Interlocked.CompareExchange(ref state, CLOSED, currentState) == currentState) 
                    {
                        disconnect();
                        listener.OnClose(1000, "");
                    }
                } 
                else if (currentState == OPEN) 
                {
                    if (Interlocked.CompareExchange(ref state, CLOSED, currentState) == currentState)
                    {
                        disconnect(code);
                        listener.OnClose(code, reason);
                    }
                }
            } catch (Exception) {
                // discard exceptions thrown by the listener
            }
            return;
        }

        private void notifyError(Exception cause) 
        {
            try {
                long currentState = Interlocked.Read(ref state);

                if (currentState == CLOSING) {
                    if (Interlocked.CompareExchange(ref state, CLOSED, currentState) == currentState)
                    {
                        listener.OnClose(1000, "");
                    }
                } 
                else if (currentState != CLOSED) 
                {
                    if (Interlocked.CompareExchange(ref state, CLOSED, currentState) == currentState) 
                    {
                        listener.OnError(cause);
                        
                    }
                }
            } catch (Exception) {
                // discard exceptions thrown by the listener
            }
            return;
        }

        private void notifyMessage(byte[] payload, int payloadLength) 
        {
            try {
                String payloadString =  System.Text.Encoding.UTF8.GetString(payload, 0, payloadLength);
                listener.OnMessage(payloadString);
            } catch (Exception) {
                // discard exceptions thrown by the listener
            }
        }
        
        private String getHost() 
        {
            return uri.Host;
        }
        
        private int getPort() 
        {
            int port = uri.Port;
            if (port == -1) 
            {
                if (System.String.Compare("wss", uri.Scheme, true) == 0)
                { 
                    port = 443;
                } 
                else 
                {
                    port = 80;
                }
            }
            return port;
        }
        
        internal static System.String getDNField(String subject, String field)
        {
            if (subject == null)
                return null;
            
            String lower = subject.ToLower();
            String lfld = field.ToLower();
            
            int cn = lower.IndexOf(lfld + "=");
            if (cn < 0)
                return null;
            
            cn += (field.Length + 1);
            if (cn >= subject.Length)
                return "";
            
            int comma = subject.IndexOf(",", cn);
            
            if (comma < 0)
                return subject.Substring(cn, (subject.Length) - (cn));
            
            return subject.Substring(cn, (comma) - (cn));
        }

        internal static String getCertCN(X509Certificate cert)
        {
            if (cert == null)
                return null;
            
            return getDNField(cert.Subject,"CN");
        }

        
        internal void Run() 
        {
            try
            {
                // set the server verification callback if connecting via wss
                if (String.Compare("wss", uri.Scheme, true) == 0)
                {
                    sp = ServicePointManager.FindServicePoint(this.uri);
                    sp.Expect100Continue = true;
                }

                if (!clientWebSocket.ConnectAsync(this.uri, CancellationToken.None).Wait(this.timeout))
                {
                    throw new Exception("timeout");
                }
                
                // notify listener
                notifyOpen();
                
                // dispatch frames
                dispatch();
            } 
            catch (Exception e) 
            {
                String message = "failed to connect";

                for (Exception inner = e.InnerException; inner != null; inner = inner.InnerException)
                    message += (" : " + inner.Message);

                Exception exp = new Exception(message);

                notifyError(exp);
            } 
            finally 
            {
               // ignore
            }
        }

        private void dispatch()
        {
            const Int32 bufferSize = 128 * 1024;

            byte[] buffer = new byte[bufferSize];
            bool breakOut = false;

            try 
            {
                while (Interlocked.Read(ref state) != CLOSED) 
                {
                    ArraySegment<byte> segment = new ArraySegment<byte>(buffer);

                    System.Net.WebSockets.WebSocketReceiveResult result;
                    System.Threading.Tasks.Task<System.Net.WebSockets.WebSocketReceiveResult> r;

                    try 
                    {
                       r  = clientWebSocket.ReceiveAsync(segment, System.Threading.CancellationToken.None);
                       r.Wait(CancellationToken.None);
                       result = r.Result;
        
                       if (result.MessageType == WebSocketMessageType.Close)
                       {
                           Task t1 = clientWebSocket.CloseAsync((System.Net.WebSockets.WebSocketCloseStatus)result.CloseStatus, result.CloseStatusDescription, CancellationToken.None);
                           t1.Wait(CancellationToken.None);
          
                           notifyClose((int)result.CloseStatus, result.CloseStatusDescription);
                           breakOut = true;
                           break;
                       }
                     } 
                     catch (Exception e)
                     {
                         notifyError(e);
                         break;
                     }
        
                     int count = result.Count;
                     while (!result.EndOfMessage)
                     {
                         if (count > buffer.Length)
                         {
                            Task t1 = clientWebSocket.CloseAsync(WebSocketCloseStatus.InvalidPayloadData, "payload too long", CancellationToken.None);
                            t1.Wait(CancellationToken.None);
                            notifyClose((int)WebSocketCloseStatus.InvalidPayloadData, "payload too long");
                            return;
                         }
                    
                        segment = new ArraySegment<byte>(buffer, count, buffer.Length - count);

                        r = clientWebSocket.ReceiveAsync(segment, CancellationToken.None);
                        r.Wait(CancellationToken.None);
                        result = r.Result;

                        if (result.MessageType == WebSocketMessageType.Close)
                        {
                            Task t1 = clientWebSocket.CloseAsync((System.Net.WebSockets.WebSocketCloseStatus)result.CloseStatus, result.CloseStatusDescription, CancellationToken.None);
                            t1.Wait(CancellationToken.None);
   
                            notifyClose((int)result.CloseStatus, result.CloseStatusDescription);
                            breakOut = true;
                            break;
                        }
                        else if (result.Count == 0)
                        {
                            // filled the buffer, resize it
                            var newBuffer = new byte[buffer.Length + bufferSize];
                            Array.Copy(buffer, newBuffer, buffer.Length);
                            buffer = newBuffer;
                        }
                        if (breakOut)
                            break;

                        count += result.Count;
                    }
                
                    // now invoke the callback.
                    if (count > 0)
                        notifyMessage(buffer, count);
                }
            } 
            catch (Exception) { return;}

            return;
        }
        
        Thread t;

        private void connect() 
        {
            t = new Thread(new ThreadStart(Run));
            t.Name = "WebSocket";

            t.Start();
        }
    }
}
