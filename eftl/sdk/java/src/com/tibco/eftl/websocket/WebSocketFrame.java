/*
 * Copyright (c) 2013-$Date$ Cloud Software Group, Inc.
 * Licensed under a BSD-style license. Refer to [LICENSE]
 *
 * $Id$
 *
 */
package com.tibco.eftl.websocket;

import java.nio.ByteBuffer;
import java.nio.charset.Charset;
import java.security.SecureRandom;
import java.util.Arrays;

/**
 *  0                   1                   2                   3
 *  0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
 * +-+-+-+-+-------+-+-------------+-------------------------------+
 * |F|R|R|R| opcode|M| Payload len |    Extended payload length    |
 * |I|S|S|S| (4)   |A|     (7)     |             (16/64)           |
 * |N|V|V|V|       |S|             |   (if payload len==126/127)   |
 * | |1|2|3|       |K|             |                               |
 * +-+-+-+-+-------+-+-------------+ - - - - - - - - - - - - - - - +
 * |   Extended payload length continued, if payload len == 127    |
 * + - - - - - - - - - - - - - - - +-------------------------------+
 * |                               |Masking-key, if MASK set to 1  |
 * +-------------------------------+-------------------------------+
 * | Masking-key (continued)       |          Payload Data         |
 * +-------------------------------- - - - - - - - - - - - - - - - +
 * :                     Payload Data continued ...                :
 * + - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - +
 * |                     Payload Data continued ...                |
 * +---------------------------------------------------------------+
 */

public class WebSocketFrame {

    public static final byte CONTINUATION = 0x00;
    public static final byte TEXT         = 0x01;
    public static final byte BINARY       = 0x02;
    public static final byte CLOSE        = 0x08;
    public static final byte PING         = 0x09;
    public static final byte PONG         = 0x0A;

    private static final SecureRandom random;
    private static final Charset UTF8;
    
    static {
        random = new SecureRandom();
        UTF8 = Charset.forName("UTF-8");
    }
    
    /** Create a binary frame. */
    public static byte[] binaryFrame(byte[] data) {
        return frame(BINARY, data);
    }
    
    /** Create a text frame. */
    public static byte[] textFrame(String text) {
        return frame(TEXT, text.getBytes(UTF8));
    }
    
    /** Create a close frame. */
    public static byte[] closeFrame(int code) {
        byte[] data = new byte[2];
        data[0] = (byte) ((code >> 8) & 0xFF);
        data[1] = (byte) ( code       & 0xFF);
        return frame(CLOSE, data);
    }
    
    /** Create a ping frame. */
    public static byte[] pingFrame(byte[] data) {
        return frame(PING, data);
    }
    
    /** Create a pong frame. */
    public static byte[] pongFrame(byte[] data, int offset, int length) {
        return frame(PONG, data, offset, length);
    }
    
    private static byte[] frame(byte opcode, byte[] data) {
       return frame(opcode, true, data, 0, (data != null ? data.length : 0));
    }
    
    private static byte[] frame(byte opcode, byte[] data, int offset, int length) {
        return frame(opcode, true, data, offset, length);
    }
    
    private static byte[] frame(byte opcode, boolean fin, byte[] data, int offset, int length) {
        if (offset < 0 || length < 0) 
            throw new IndexOutOfBoundsException();
        
        if (data != null && offset + length > data.length) 
            throw new IndexOutOfBoundsException();
        
        int maskOffset = (length <= 125 ? 2 : (length <= 65535 ? 4 : 10));
        int dataOffset = maskOffset + 4; // plus 4 for mask
    
        byte[] frame = new byte[length + dataOffset];
        
        frame[0] = (byte) ((byte) (fin ? 0x80 : 0) | opcode);
        
        if (length <= 125) {
            frame[1] = (byte) (0x80 | length);
        } else if (length <= 65535) {
            frame[1] = (byte) (0x80 | 126);
            frame[2] = (byte) ((((long) length) >> 8)  & 0xFF);
            frame[3] = (byte) ( ((long) length)        & 0xFF);
        } else {
            frame[1] = (byte) (0x80 | 127);
            frame[2] = (byte) ((((long) length) >> 56) & 0xFF);
            frame[3] = (byte) ((((long) length) >> 48) & 0xFF);
            frame[4] = (byte) ((((long) length) >> 40) & 0xFF);
            frame[5] = (byte) ((((long) length) >> 32) & 0xFF);
            frame[6] = (byte) ((((long) length) >> 24) & 0xFF);
            frame[7] = (byte) ((((long) length) >> 16) & 0xFF);
            frame[8] = (byte) ((((long) length) >> 8)  & 0xFF);
            frame[9] = (byte) ( ((long) length)        & 0xFF);
        }

        generateMask(frame, maskOffset);
        
        if (data != null) {
            for (int i = 0; i < length; i++) {
                frame[dataOffset + i] = (byte) (data[offset + i] ^ frame[maskOffset + (i % 4)]);
            }
        }
        
        return frame;
    }

    /** Generates a 4-byte mask. */
    private static void generateMask(byte[] buffer, int offset) {
        int rand = 0;
        for (int i = 0; i < 4; i++) {
            rand = (i == 0 ? random.nextInt() : rand >> 8);
            buffer[i + offset] = (byte) rand;
        }
    }

    private boolean fin;
    private boolean masked;
    private byte opcode;
    private byte[] mask;
    private byte[] payload;
    private int payloadOffset;
    private int payloadLength;
    private int state;
    private int cursor;
    
    public WebSocketFrame() {
    }

    public void setFin(boolean fin) {
        this.fin = fin;
    }
    
    public boolean isFin() {
        return fin;
    }

    public void setOpCode(byte opcode) {
        this.opcode = opcode;
    }
    
    public byte getOpCode() {
        return opcode;
    }

    public void setPayload(byte[] data, int offset, int length) {
        payload = Arrays.copyOfRange(data, offset, length);
        payloadLength = length;
    }
    
    public int getPayloadLength() {
        return payloadLength;
    }
    
    public byte[] getPayload() {
        return payload;
    }

    public String getPayloadAsString() {
        return new String(payload, 0, payloadLength, UTF8);
    }
    
    public int getCloseCode() {
        int code = 0;
        
        if (payload != null && payloadLength >= 2) {
            code |= (payload[0] & 0xFF) << 8;
            code |= (payload[1] & 0xFF);
        } else {
            code = 1005;
        }
        
        return code;
    }
    
    public String getCloseReason() {
        String reason = null;
        
        if (payload != null && payloadLength > 2) {
            reason = new String(Arrays.copyOfRange(payload, 2, payloadLength), UTF8);
        } else {
            reason = "";
        }
        
        return reason;
    }
    
    public WebSocketFrame copy() {
        WebSocketFrame frame = new WebSocketFrame();
        frame.setOpCode(opcode);
        frame.setFin(fin);
        frame.setPayload(payload, 0, payloadLength);
        return frame;
    }
    
    public void append(WebSocketFrame frame) {
        payloadOffset = payloadLength;
        payloadLength += frame.getPayloadLength();
        // append to the payload
        payload = Arrays.copyOf(payload, payloadLength);
        System.arraycopy(frame.getPayload(), 0, payload, payloadOffset, frame.getPayloadLength());
        // update the fin
        fin = frame.isFin();
    }
    
    public boolean parse(ByteBuffer buffer) throws WebSocketException {
        while (buffer.hasRemaining()) {
            switch (state) {
            
            // FIN and Opcode
            case 0: {
                byte b = buffer.get();
                fin = (b & 0x80) != 0;
                opcode = (byte) (b & 0x0F);
                int rsv = (b & 0x70) >> 4;
                
                if (rsv != 0)
                    throw new WebSocketException("rsv is non-zero");
                
                if (opcode == CONTINUATION || opcode == TEXT || opcode == BINARY) {
                    // valid data messages
                } else if (opcode == CLOSE || opcode == PING || opcode == PONG) {
                    if (!fin)
                        throw new WebSocketException("fragmented control message");
                } else {
                    throw new WebSocketException("unknown op code: " + opcode);
                }
                
                state = 1;
                
                break;
            }
            
            // Length
            case 1: {
                byte b = buffer.get();
                masked = (b & 0x80) != 0;

                payloadLength = (byte) (b & 0x7F);
                if (payloadLength >= 0 && payloadLength <= 125) {
                    if (payload == null || payload.length < payloadLength) {
                        payload = new byte[payloadLength];
                    }
                    if (masked) {
                        cursor = 4;
                        state = 3;
                    } else {
                        if (payloadLength == 0) {
                            state = 0;
                            return true;
                        }
                        state = 4;
                    }
                } else if (payloadLength == 126) {
                    payloadLength = 0;
                    cursor = 2;
                    state = 2;
                } else if (payloadLength == 127) {
                    payloadLength = 0;
                    cursor = 8;
                    state = 2;
                } else {
                    throw new WebSocketException("invalid payload length");
                }
                
                break;
            }
            
            // Extended Length
            case 2: {
                byte b = buffer.get();
                
                payloadLength |= (b & 0xFF) << (8 * --cursor);
                if (cursor == 0) {
                    if (payload == null || payload.length < payloadLength) {
                        payload = new byte[payloadLength];
                    }
                    if (masked) {
                        cursor = 4;
                        state = 3;
                    } else {
                        if (payloadLength == 0) {
                            state = 0;
                            return true;
                        }
                        state = 4;
                    }
                }
                
                break;
            }
            
            // Mask (typically not used by server)
            case 3: {
                byte b = buffer.get();
                
                if (mask == null) {
                    mask = new byte[4];
                }
                
                mask[4 - cursor--] = b;
                if (cursor == 0) {
                    if (payloadLength == 0) {
                        state = 0;
                        return true;
                    }
                    state = 4;
                }

                break;
            }
            
            // Payload
            case 4: {
                int numBytes = Math.min(buffer.remaining(), (payloadLength - payloadOffset));
                buffer.get(payload, payloadOffset, numBytes);
                payloadOffset += numBytes;
                
                if (payloadOffset >= payloadLength) {
                    payloadOffset = 0;
                    
                    if (masked) {
                        for (int i = 0; i < payloadLength; i++) {
                            payload[i] = (byte) (payload[i] ^ mask[i % 4]);
                        }
                    }
                    
                    state = 0;
                    return true;
                }

                break;
            }
            
            }
        }
        
        return false;
    }
}
