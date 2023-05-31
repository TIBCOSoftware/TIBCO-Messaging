/*
 * Copyright (c) 2013-$Date$ Cloud Software Group, Inc.
 * Licensed under a BSD-style license. Refer to [LICENSE]
 *
 * $Id$
 *
 */
package com.tibco.eftl.websocket;

import java.io.UnsupportedEncodingException;

public class Base64
{
    private static final byte[] base64map = new byte[] {
        'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J', 'K', 'L', 'M', 
        'N', 'O', 'P', 'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X', 'Y', 'Z', 
        'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h', 'i', 'j', 'k', 'l', 'm', 
        'n', 'o', 'p', 'q', 'r', 's', 't', 'u', 'v', 'w', 'x', 'y', 'z', 
        '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '+', '/'};
    
    public static String encode(byte[] input) {
        
        int length = (input.length + 2) * 4 / 3;
        byte[] output = new byte[length];
        int end = input.length - input.length % 3;
        int index = 0;
        
        // encode every 3 bytes of the input into 4 bytes of output
        
        for (int i = 0; i < end; i += 3) {
            // first 6 bits from the first byte
            output[index++] = base64map[(input[i] & 0xff) >> 2];
            // last 2 bits from the first byte and first 4 bits from the second byte
            output[index++] = base64map[((input[i] & 0x03) << 4) | ((input[i+1] & 0xff) >> 4)];
            // last 4 bits from the second byte and first 2 bits from the third byte 
            output[index++] = base64map[((input[i+1] & 0x0f) << 2) | ((input[i+2] & 0xff) >> 6)];
            // last 6 bits from the third byte
            output[index++] = base64map[(input[i+2] & 0x3f)];
        }
        
        // handle the last byte(s) if the input length is not divisible by 3
        
        switch (input.length % 3) {
        case 1:
            // first 6 bits from the last byte
            output[index++] = base64map[(input[end] & 0xff) >> 2];
            // last 2 bits from the last byte
            output[index++] = base64map[(input[end] & 0x03) << 4];
            // padding
            output[index++] = '=';
            output[index++] = '=';
            break;
        case 2:
            // first 6 bits from the second to last byte
            output[index++] = base64map[(input[end] & 0xff) >> 2];
            // last 2 bits from the second to last byte and first 4 bits from the last byte
            output[index++] = base64map[((input[end] & 0x03) << 4) | ((input[end+1] & 0xff) >> 4)];
            // last 4 bits from the last byte 
            output[index++] = base64map[((input[end+1] & 0x0f) << 2)];
            // padding
            output[index++] = '=';
            break;
        }

        // convert to a string
        
        try {
            return new String(output, 0, index, "US-ASCII");
        } catch (UnsupportedEncodingException e) {
            // should never happen, US-ASCII is always required to be present
            return new String(output, 0, index);
        }
    }
}
