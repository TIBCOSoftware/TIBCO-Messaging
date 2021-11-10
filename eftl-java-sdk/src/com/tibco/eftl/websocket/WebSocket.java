/*
 * Copyright (c) 2013-$Date: 2020-08-19 11:55:34 -0700 (Wed, 19 Aug 2020) $ TIBCO Software Inc.
 * All rights reserved.
 * For more information, please contact:
 * TIBCO Software Inc., Palo Alto, California, USA
 *
 * $Id: WebSocket.java 127983 2020-08-19 18:55:34Z bpeterse $
 *
 */
package com.tibco.eftl.websocket;

import java.io.EOFException;
import java.io.IOException;
import java.lang.reflect.InvocationTargetException;
import java.lang.reflect.Method;
import java.net.InetSocketAddress;
import java.net.Socket;
import java.net.SocketException;
import java.net.URI;
import java.nio.ByteBuffer;
import java.security.KeyManagementException;
import java.security.NoSuchAlgorithmException;
import java.security.cert.CertificateException;
import java.security.cert.X509Certificate;
import java.util.ArrayList;
import java.util.concurrent.TimeUnit;
import java.util.concurrent.atomic.AtomicReference;

import javax.net.SocketFactory;
import javax.net.ssl.HostnameVerifier;
import javax.net.ssl.SSLContext;
import javax.net.ssl.SSLParameters;
import javax.net.ssl.SSLPeerUnverifiedException;
import javax.net.ssl.SSLSocket;
import javax.net.ssl.TrustManager;
import javax.net.ssl.X509TrustManager;

public class WebSocket implements Runnable {

    private enum ReadyState {
        INIT, CONNECTING, OPEN, CLOSING, CLOSED;
    }
    
    private URI uri;
    private WebSocketListener listener;
    private ArrayList<String> protocols;
    private String protocol;
    private Socket socket;
    private TrustManager[] trustManagers;
    private boolean trustAll;
    private AtomicReference<ReadyState> state = 
            new AtomicReference<ReadyState>(ReadyState.INIT);
    
    // configuration
    private int connectTimeout = 15000;
    private int socketTimeout = 0;

    // Public API /////////////////////////////////////////////////////////////
    
    public WebSocket(URI uri, WebSocketListener listener) {
        this.uri = uri;
        this.listener = listener;
        this.protocols = new ArrayList<String>();
        
        if (!"ws".equalsIgnoreCase(uri.getScheme()) &&
            !"wss".equalsIgnoreCase(uri.getScheme())) {
            throw new IllegalArgumentException("URI scheme must be 'ws' or 'wss'");
        }
    }
    
    public void setProtocol(String protocol) {
        this.protocols.add(protocol);
    }
    
    public String getProtocol() {
        return protocol;
    }
    
    public void setConnectTimeout(int connectTimeout, TimeUnit unit) {
        this.connectTimeout = (int) unit.convert(connectTimeout, TimeUnit.MILLISECONDS);
    }
    
    public void setSocketTimeout(int socketTimeout, TimeUnit unit) {
        this.socketTimeout = (int) unit.convert(socketTimeout, TimeUnit.MILLISECONDS);
        try {
            if (socket != null) {
                socket.setSoTimeout(this.socketTimeout);
            }
        } catch (SocketException e) {
            // ignore
        }
    }
    
    public void setTrustManagers(TrustManager[] trustManagers) {
        this.trustManagers = trustManagers;
    }
    
    public void setTrustAll(boolean trustAll) {
        this.trustAll = trustAll;
    }

    public void open() {
        if (state.compareAndSet(ReadyState.INIT, ReadyState.CONNECTING)) {
            connect();
        }
    }
    
    public void close() {
        close(1000);
    }
    
    public void close(int code) {
        if (state.compareAndSet(ReadyState.CONNECTING, ReadyState.CLOSED)) {
            disconnect();
        }
        else if (state.compareAndSet(ReadyState.OPEN, ReadyState.CLOSING)) {
            disconnect(code);
        }
    }
    
    public void forceClose() {
        disconnect();
    }
    
    public void send(String text) throws IOException {
        if (!isConnected())
            throw new IllegalStateException("WebSocket is not open");
        write(WebSocketFrame.textFrame(text));
    }
    
    public void send(byte[] data) throws IOException {
        if (!isConnected())
            throw new IllegalStateException("WebSocket is not open");
        write(WebSocketFrame.binaryFrame(data));
    }
    
    public void ping(byte[] data) throws IOException {
        if (!isConnected())
            throw new IllegalStateException("WebSocket is not open");
        write(WebSocketFrame.pingFrame(data));
    }
    
    public boolean isConnected() {
        return (state.get() == ReadyState.OPEN);
    }
    
    // Implementation /////////////////////////////////////////////////////////

    private void notifyOpen() {
        try {
            ReadyState currentState = state.get();
            if (currentState == ReadyState.CONNECTING) {
                if (state.compareAndSet(currentState, ReadyState.OPEN)) {
                    listener.onOpen();
                }
            }
        } catch (Exception e) {
            // discard exceptions thrown by the listener
        }
    }
    
    private void notifyClose(int code, String reason) {
        try {
            ReadyState currentState = state.get();
            if (currentState == ReadyState.CLOSING) {
                if (state.compareAndSet(currentState, ReadyState.CLOSED)) {
                    disconnect();
                    listener.onClose(1000, "");
                }
            } else if (currentState == ReadyState.OPEN) {
                if (state.compareAndSet(currentState, ReadyState.CLOSED)) {
                    disconnect(code);
                    listener.onClose(code, reason);
                }
            }
        } catch (Exception e) {
            // discard exceptions thrown by the listener
        }
    }
    
    private void notifyError(Throwable cause) {
        try {
            ReadyState currentState = state.get();
            if (currentState == ReadyState.CLOSING) {
                if (state.compareAndSet(currentState, ReadyState.CLOSED)) {
                    listener.onClose(1000, "");
                }
            } else if (currentState != ReadyState.CLOSED) {
                if (state.compareAndSet(currentState, ReadyState.CLOSED)) {
                    listener.onError(cause);
                }
            }
        } catch (Exception e) {
            // discard exceptions thrown by the listener
        }
    }

    private void notifyMessage(WebSocketFrame frame) {
        try {
            if (frame.getOpCode() == WebSocketFrame.TEXT) {
                listener.onMessage(frame.getPayloadAsString());
            } else {
                listener.onMessage(frame.getPayload(), 0, frame.getPayloadLength());
            }
        } catch (Exception e) {
            // discard exceptions thrown by the listener
        }
    }
    
    private void notifyPong(WebSocketFrame frame) {
        try {
            listener.onPong(frame.getPayload(), 0, frame.getPayloadLength());
        } catch (Exception e) {
            // discard exceptions thrown by the listener
        }
    }
    
    private String getHost() {
        return uri.getHost();
    }
    
    private int getPort() {
        int port = uri.getPort();
        if (port == -1) {
            if ("wss".equalsIgnoreCase(uri.getScheme())) { 
                port = 443;
            } else {
                port = 80;
            }
        }
        return port;
    }

    @Override
    public void run() {
        try {
            // socket connect
            socket = getSocketFactory().createSocket();
            socket.connect(new InetSocketAddress(getHost(), getPort()), connectTimeout);
            
            // verify hostname
            verifyHostname(getHost(), socket);
            
            // socket configuration
            socket.setSoTimeout(socketTimeout);
            socket.setSendBufferSize(128*1024);
            socket.setReceiveBufferSize(128*1024);
            socket.setTcpNoDelay(true);
            
            // send HTTP upgrade request
            UpgradeRequest request = new UpgradeRequest(uri, protocols);
            request.write(socket.getOutputStream());
            
            // read HTTP upgrade response
            UpgradeResponse response = UpgradeResponse.read(socket.getInputStream());
            response.validate(request, protocols);
            
            // get the agreed upon protocol
            protocol = response.getProtocol();
            
            // notify listener
            notifyOpen();
            
            // dispatch frames
            dispatch();
        } catch (Exception e) {
            notifyError(e);
        } finally {
            try {
                if (socket != null)
                    socket.close();
            } catch (IOException e) {
                // ignore
            }
        }
    }

    private void dispatch() throws IOException, WebSocketException {
        WebSocketFrame frame = new WebSocketFrame();
        WebSocketFrame partialFrame = null;
        ByteBuffer buffer = ByteBuffer.allocate(128*1024);

        while (state.get() != ReadyState.CLOSED) {
            read(buffer);
            buffer.flip();
            while (frame.parse(buffer)) {
                switch (frame.getOpCode()) {
                case WebSocketFrame.CONTINUATION:
                    if (partialFrame == null) {
                        throw new WebSocketException("unexpected continuation frame");
                    }
                    partialFrame.append(frame);
                    if (partialFrame.isFin()) {
                        notifyMessage(partialFrame);
                        partialFrame = null;
                    }
                    break;
                case WebSocketFrame.TEXT:
                    if (partialFrame != null) {
                        throw new WebSocketException("expected continuation frame");
                    }
                    if (frame.isFin()) {
                        notifyMessage(frame);
                    } else {
                        partialFrame = frame.copy();
                    }
                    break;
                case WebSocketFrame.BINARY:
                    if (partialFrame != null) {
                        throw new WebSocketException("expected continuation frame");
                    }
                    if (frame.isFin()) {
                        notifyMessage(frame);
                    } else {
                        partialFrame = frame.copy();
                    }
                    break;
                case WebSocketFrame.CLOSE:
                    notifyClose(frame.getCloseCode(), frame.getCloseReason());
                    break;
                case WebSocketFrame.PING:
                    write(WebSocketFrame.pongFrame(frame.getPayload(), 0, frame.getPayloadLength()));
                    break;
                case WebSocketFrame.PONG:
                    notifyPong(frame);
                    break;
                }
            }
            buffer.clear();
        }
    }
    
    private void connect() {
        new Thread(this, "WebSocket").start();
    }
    
    private void disconnect() {
        try {
            socket.close();
        } catch (IOException e) {
            // ignore
        }
    }
    
    private void disconnect(int code) {
        try {
            write(WebSocketFrame.closeFrame(code));
            socket.shutdownOutput();
        } catch (UnsupportedOperationException e) {
            // socket.shutdownOutput() not supported by SSL
        } catch (IOException e) {
            // ignore
        }
    }
    
    private void write(byte[] frame) throws IOException {
        synchronized(socket) {
            socket.getOutputStream().write(frame);
        }
    }

    private void read(ByteBuffer buffer) throws IOException {
        int numRead = socket.getInputStream().read(buffer.array(), buffer.position(), buffer.remaining());
        if (numRead < 0)
            throw new EOFException();
        buffer.position(buffer.position() + numRead);
    }
    
    private SocketFactory getSocketFactory() throws NoSuchAlgorithmException, KeyManagementException {
        if ("wss".equalsIgnoreCase(uri.getScheme())) {
            SSLContext context = SSLContext.getInstance("TLS");
            context.init(null, (trustAll ? TRUST_ALL : trustManagers), null);
            return context.getSocketFactory();
        } else {
            return SocketFactory.getDefault();
        }
    }
    
    private void verifyHostname(String hostname, Socket socket) throws CertificateException, IOException {
        
        // Perform hostname verification only if one or more trust managers
        // have been provided. Otherwise, any certificate will be accepted.
        
        if (socket instanceof SSLSocket && trustManagers != null) {
            SSLSocket sslSocket = (SSLSocket)socket;

            boolean verified = false;
            
            Exception javaException = null;
            Exception androidException = null;
            
            // Java 7 hostname verification: must be done before the handshake.
            // This mechanism is not supported by Android.
            
            try {
                Method setEndpointIdentificationAlgorithm = 
                        SSLParameters.class.getMethod("setEndpointIdentificationAlgorithm", String.class);
                SSLParameters params = new SSLParameters();
                setEndpointIdentificationAlgorithm.invoke(params, "HTTPS");
                sslSocket.setSSLParameters(params);
                verified = true;
            } catch (NoSuchMethodException e) {
                javaException = e;
            } catch (SecurityException e) {
                javaException = e;
            } catch (IllegalAccessException e) {
                javaException = e;
            } catch (IllegalArgumentException e) {
                javaException = e;
            } catch (InvocationTargetException e) {
                javaException = e;
            }
            
            // start the SSL handshake
            sslSocket.startHandshake();
            
            // Android hostname verification: must be done after the handshake.
            // Java does not provide a built-in HostnameVerifier implementation

            if (!verified) {
                try {
                    // Java does not provide built-in HostnameVerifier implementations.
                    HostnameVerifier verifier = 
                            (HostnameVerifier) Class.forName("org.apache.http.conn.ssl.BrowserCompatHostnameVerifier").newInstance();
                    if (!verifier.verify(hostname, sslSocket.getSession())) {
                        throw new CertificateException("No hostname matching " + hostname + " found");
                    } 
                    verified = true;
                } catch (InstantiationException e) {
                    androidException = e;
                } catch (IllegalAccessException e) {
                    androidException = e;
                } catch (ClassNotFoundException e) {
                    androidException = e;
                }
            }
            
            if (!verified) {
                throw new SSLPeerUnverifiedException("Unable to verify hostname: " + 
                        (javaException != null ? "<"+javaException.getClass().getName()+" "+javaException.getMessage()+">" : "") +
                        (androidException != null ? "<"+androidException.getClass().getName()+" "+androidException.getMessage()+">" : ""));
            }
        }
    }
    
    private final static TrustManager[] TRUST_ALL = new TrustManager[] { new X509TrustManager() {
            
        @Override
        public void checkClientTrusted(X509Certificate[] chain, String authType) throws CertificateException {
        }

        @Override
        public void checkServerTrusted(X509Certificate[] chain, String authType) throws CertificateException {
        }

        @Override
        public X509Certificate[] getAcceptedIssuers() {
            return null;
        }
    }};
}
