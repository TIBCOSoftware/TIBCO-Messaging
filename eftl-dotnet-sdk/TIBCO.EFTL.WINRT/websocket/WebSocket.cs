/*
 * Copyright (c) 2009-$Date: 2016-04-29 21:04:32 -0500 (Fri, 29 Apr 2016) $ TIBCO Software Inc.
 * Licensed under a BSD-style license. Refer to [LICENSE]
 * For more information, please contact:
 * TIBCO Software Inc., Palo Alto, California, USA
 *
 * $Id: WebSocket.cs 85918 2016-04-30 02:04:32Z bpeterse $
 */

using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading;
using System.IO;
using System.Security.Permissions;
using System.Security.Principal;
using System.Configuration;
using Windows.Storage.Streams;
using System.Runtime.InteropServices.WindowsRuntime;
using Windows.Foundation;
using System.Threading.Tasks;

namespace TIBCO.EFTL
{
    public class WebSocket
    {
        private Uri                uri; 
        private WebSocketListener  listener;
        long                       state;
        private ArrayList          protocols;
        private String             protocol;
        
        private Windows.Networking.HostName remoteHostName;
        private Windows.Networking.Sockets.StreamSocket socket = null;

        long INIT       = 0;
        long CONNECTING = 1;
        long OPEN       = 2;
        long CLOSING    = 3;
        long CLOSED     = 4;

        // Public API /////////////////////////////////////////////////////////////
        public WebSocket(Uri uri, WebSocketListener listener) 
        {
            this.uri       = uri;
            this.listener  = listener;

            remoteHostName = new Windows.Networking.HostName(uri.Host);
            this.protocols = new ArrayList();

            if ((String.Compare("ws", uri.Scheme, StringComparison.CurrentCultureIgnoreCase) != 0) && 
                (String.Compare("wss", uri.Scheme, StringComparison.CurrentCultureIgnoreCase) != 0))
            {
                throw new Exception("URI scheme must be 'ws' or 'wss'");
            }
        }

        public void setProtocol(String protocol) {
            this.protocols.Add(protocol);
        }

        public String getProtocol() {
            return protocol;
        }
                
        public void Open() 
        {
            if (Interlocked.CompareExchange(ref state, CONNECTING, INIT) == INIT)
                connect();
        }
    
        public void close() {
            close(1000);
        }

        public void close(int code) {
            if (Interlocked.CompareExchange(ref state, CLOSED, CONNECTING) == CONNECTING)
                disconnect();
            else if (Interlocked.CompareExchange(ref state, CLOSING, OPEN) == OPEN)
                disconnect(code);
        }

        public void forceClose() {
            disconnect();
        }

        private void disconnect() 
        {
            try {
            } catch (IOException) {
                // ignore
            }
        }

        private void disconnect(int code) {
            try {
                write(WebSocketFrame.close(code));
            }
            catch (IOException) {
                // ignore
            }
        }

        public void send(String text) {
            if (!isConnected())
                throw new Exception("WebSocket is not open");

            write(WebSocketFrame.text(text));
        }

        public void send(byte[] data) {
            if (!isConnected())
                throw new Exception("WebSocket is not open");
            write(data);
        }

        public void ping(byte[] data) {
            if (!isConnected())
                throw new Exception("WebSocket is not open");
            write(data);
        }

        public bool isConnected() 
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

        private void notifyMessage(WebSocketFrame frame) 
        {
            try {
                if (frame.getOpCode() == WebSocketFrameConstants.TEXT) {
                    listener.OnMessage(frame.getPayloadAsString());
                } else {
                    listener.OnMessage(frame.getPayload(), 0, frame.getPayloadLength());
                }
            } catch (Exception) {
                // discard exceptions thrown by the listener
            }
        }
        
        private void notifyPong(WebSocketFrame frame) 
        {
            try {
                listener.OnPong(frame.getPayload(), 0, frame.getPayloadLength());
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
                if (System.String.Compare("wss", uri.Scheme, StringComparison.CurrentCultureIgnoreCase) == 0)
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
        
        public void Run() 
        {
            try 
            {
                socket = new Windows.Networking.Sockets.StreamSocket();
                UpgradeRequest request = null;
                try
                {
                    // connect to the eftlServer
                    if (String.Compare("ws", uri.Scheme, StringComparison.CurrentCultureIgnoreCase) == 0)
                        socket.ConnectAsync(remoteHostName, getPort().ToString(), Windows.Networking.Sockets.SocketProtectionLevel.PlainSocket).AsTask().Wait(5000);
                    else if (String.Compare("wss", uri.Scheme, StringComparison.CurrentCultureIgnoreCase) == 0)
                        socket.ConnectAsync(remoteHostName, getPort().ToString(), Windows.Networking.Sockets.SocketProtectionLevel.Ssl).AsTask().Wait(5000);
                    
                    Windows.Networking.Sockets.SocketProtectionLevel l = socket.Information.ProtectionLevel;
                    Console.WriteLine("ProtectionLevel = " + l);

                    // send HTTP upgrade request
                    request = new UpgradeRequest(uri, protocols);
                    DataWriter writer = new DataWriter(socket.OutputStream);

                    String s = request.toString();
                    writer.WriteString(s);

                    // Call StoreAsync method to store the data to a backing stream
                    try
                    {
                        writer.StoreAsync().AsTask().Wait();
                        writer.FlushAsync().AsTask().Wait();
                    }
                    catch (Exception e) { 
                        Console.WriteLine(e.StackTrace);  
                    }
                    writer.DetachStream();
                }
                catch (Exception e)
                {
                    Exception exp = new Exception("failed to connect" + ((e.InnerException != null) ? e.InnerException.Message : ""));
                    notifyError(exp);
                }

                byte[] buffer = new byte[32768];
                IInputStream inputStream = socket.InputStream;

                try {
                    inputStream.ReadAsync(buffer.AsBuffer(), (uint)buffer.Length, InputStreamOptions.Partial).AsTask().Wait();
                    System.IO.Stream stream = new System.IO.MemoryStream(buffer);

                    // read HTTP upgrade response
                    UpgradeResponse response = UpgradeResponse.read(stream);
                    response.validate(request);

                    // get the agreed upon protocol
                    protocol = response.getProtocol();
                }
                catch (Exception e) 
                {
                    notifyError(e);
                }

                // notify listener
                notifyOpen();
                    
                // dispatch frames
                dispatch();
            } 
            catch (Exception e) 
            {
                notifyError(e);
            } 
            finally 
            {
                socket.Dispose();
            }
        }

        private void dispatch()
        {
            WebSocketFrame frame = new WebSocketFrame();
            WebSocketFrame partialFrame = null;

            byte[] buffer = new byte[128*1024];

            while (Interlocked.Read(ref state) != CLOSED) 
            {
                int readBytes = read(buffer);

                MemoryStream s = new MemoryStream(buffer, 0, readBytes);
                while (frame.parse(s, readBytes)) 
                {
                    switch (frame.getOpCode()) 
                    {
                        case WebSocketFrameConstants.CONTINUATION:
                            if (partialFrame == null) 
                            {
                                throw new WebSocketException("unexpected continuation frame");
                            }
                            break;
                        case WebSocketFrameConstants.TEXT:
                            if (partialFrame != null) {
                                throw new WebSocketException("expected continuation frame");
                            }
                            if (frame.isFin()) {
                                notifyMessage(frame);
                            } else {
                                partialFrame = frame.copy();
                            }
                            break;
                        case WebSocketFrameConstants.BINARY:
                            if (partialFrame != null) {
                                throw new WebSocketException("expected continuation frame");
                            }
                            if (frame.isFin()) {
                                notifyMessage(frame);
                            } else {
                                partialFrame = frame.copy();
                            }
                            break;
                        case WebSocketFrameConstants.CLOSE:
                            notifyClose(frame.getCloseCode(), frame.getCloseReason());
                            break;
                        case WebSocketFrameConstants.PING:
                            write(WebSocketFrame.pong(frame.getPayload(), 0, frame.getPayloadLength()));
                            break;
                        case WebSocketFrameConstants.PONG:
                            notifyPong(frame);
                            break;
                    }
                }
            }
        }
        
        Thread t;

        private void connect() 
        {
            t = new Thread(new ThreadStart(Run));
            t.Name = "WebSocket";

            t.Start();
        }
    
        private void write(byte[] frame) 
        {
            DataWriter writer = new DataWriter(socket.OutputStream);
            writer.WriteBytes(frame);

            // Call StoreAsync method to store the data to a backing stream
            try
            {
                writer.StoreAsync().AsTask().Wait();
                writer.FlushAsync().AsTask().Wait();
            }
            catch (Exception) 
            { 
            }
            finally
            {
                writer.DetachStream();
            }
        }
        
        private int read(byte[] buffer)
        {
            IBuffer result;

            result = socket.InputStream.ReadAsync(buffer.AsBuffer(), (uint)buffer.Length, InputStreamOptions.Partial).AsTask().Result;

            return (int)result.Length;
        }
    }
}
