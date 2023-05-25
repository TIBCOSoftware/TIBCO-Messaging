/*
 * Copyright (c) 2001-$Date$ TIBCO Software Inc.
 * Licensed under a BSD-style license. Refer to [LICENSE]
 * For more information, please contact:
 * TIBCO Software Inc., Palo Alto, California, USA
 *
 * $Id$
 */
package com.tibco.eftl.json;

public class JsonParser {

    private final int len;
    private final String src;

    private int pos;
    
    public JsonParser(String text) {
        pos = 0;
        len = text.length();
        src = text;
    }

    public Object parse() {
        Object value = readJson();
        consumeWhitespace();
        if (pos < len) {
            throw new JsonException("Expected end of stream at " + pos);
        }
        return value;
    }

    private Object readJson() {
        consumeWhitespace();
        while (pos < len) {
            char c = src.charAt(pos++);
            switch (c) {
            case '{':
                return readJsonObject();
            case '[':
                return readJsonArray();
            case '"':
                return readJsonString();
            case 't':
                return readJsonTrue();
            case 'f':
                return readJsonFalse();
            case 'n':
                return readJsonNull();
            case '-':
            case '0':
            case '1':
            case '2':
            case '3':
            case '4':
            case '5':
            case '6':
            case '7':
            case '8':
            case '9':
                return readJsonNumber(c);
            default:
                throw new JsonException("Unexpected token: " + c);
            }
        }
        return null;
    }

    private JsonObject readJsonObject() {
        consumeWhitespace();
        
        JsonObject object = new JsonObject();

        // check for an empty object
        if (pos < len && src.charAt(pos) == '}') {
            pos++;
            return object;
        }

        boolean needsComma = false;

        while (pos < len) {
            char c = src.charAt(pos++);
            switch(c) {
            case '}':
                if (!needsComma) {
                    throw new JsonException("Unexpected comma in object literal");
                }
                return object;
            case ',':
                if (!needsComma) {
                    throw new JsonException("Unexpected comma in object literal");
                }
                needsComma = false;
                break;
            case '"':
                if (needsComma) {
                    throw new JsonException("Missing comma in object literal");
                }
                needsComma = true;
                String name = readJsonString();
                consumeChar(':');
                object.put(name, readJson());
                break;
            default:
                throw new JsonException("Unexpected token in object literal");
            }
            
            consumeWhitespace();
        }

        throw new JsonException("Unterminated object literal");
    }

    private JsonArray readJsonArray() {
        consumeWhitespace();
        
        JsonArray array = new JsonArray();

        // check for an empty array
        if (pos < len && src.charAt(pos) == ']') {
            pos++;
            return array;
        }

        boolean needsComma = false;

        while (pos < len) {
            char c = src.charAt(pos);
            switch(c) {
            case ']':
                if (!needsComma) {
                    throw new JsonException("Unexpected comma in array literal");
                }
                pos++;
                return array;
            case ',':
                if (!needsComma) {
                    throw new JsonException("Unexpected comma in array literal");
                }
                needsComma = false;
                pos++;
                break;
            default:
                if (needsComma) {
                    throw new JsonException("Missing comma in array literal");
                }
                needsComma = true;
                array.add(readJson());
            }
            
            consumeWhitespace();
        }

        throw new JsonException("Unterminated array literal");
    }

    private String readJsonString() {

        int start = pos;

        // optimal case when string contains no escape characters
        
        while (pos < len) {
            char c = src.charAt(pos++);
            if (c <= '\u001F') {
                throw new JsonException("String contains control character");
            } else if (c == '\\') {
                break;
            } else if (c == '"') {
                return src.substring(start, pos-1);
            }
        }

        // non-optimal case where string contains escape characters

        StringBuilder b = new StringBuilder();

        while (pos < len) {
            b.append(src, start, pos-1);

            char c = src.charAt(pos++);
            switch (c) {
            case '"':
                b.append('"');
                break;
            case '\\':
                b.append('\\');
                break;
            case '/':
                b.append('/');
                break;
            case 'b':
                b.append('\b');
                break;
            case 'f':
                b.append('\f');
                break;
            case 'n':
                b.append('\n');
                break;
            case 'r':
                b.append('\r');
                break;
            case 't':
                b.append('\t');
                break;
            case 'u':
                if (len - pos < 5) {
                    throw new JsonException("Invalid character code: \\u" + src.substring(pos));
                }
                int code = fromHex(src.charAt(pos)) << 12 |
                           fromHex(src.charAt(pos+1)) <<  8 |
                           fromHex(src.charAt(pos+2)) <<  4 |
                           fromHex(src.charAt(pos+3));
                if (code < 0) {
                    throw new JsonException("Invalid character code: " + src.substring(pos, pos + 4));
                }
                pos += 4;
                b.append((char) code);
                break;
            default:
                throw new JsonException("Unexpected character in string: '\\" + c + "'");
            }

            start = pos;

            while (pos < len) {
                c = src.charAt(pos++);
                if (c <= '\u001F') {
                    throw new JsonException("String contains control character");
                } else if (c == '\\') {
                    break;
                } else if (c == '"') {
                    b.append(src, start, pos-1);
                    return b.toString();
                }
            }
        }

        throw new JsonException("Unterminated string literal");
    }

    private int fromHex(char c) {
        return c >= '0' && c <= '9' ? c - '0'
                : c >= 'A' && c <= 'F' ? c - 'A' + 10
                        : c >= 'a' && c <= 'f' ? c - 'a' + 10
                                : -1;
    }

    private Number readJsonNumber(char c) {
        final int start = pos - 1;

        if (c == '-') {
            c = nextChar();
            if (!(c >= '0' && c <= '9')) {
                throw new JsonException("Invalid number format: " + src.substring(start, pos));
            }
        }
        
        if (c != '0') {
            readDigits();
        }

        boolean isDouble = false;
        
        // fraction part
        if (pos < len) {
            c = src.charAt(pos);
            if (c == '.') {
                pos++;
                c = nextChar();
                if (!(c >= '0' && c <= '9')) {
                    throw new JsonException("Invalid number format: " + src.substring(start, pos));
                }
                readDigits();
                isDouble = true;
            }
        }

        // exponent part
        if (pos < len) {
            c = src.charAt(pos);
            if (c == 'e' || c == 'E') {
                pos++;
                c = nextChar();
                if (c == '-' || c == '+') {
                    c = nextChar();
                }
                if (!(c >= '0' && c <= '9')) {
                    throw new JsonException("Invalid number format: " + src.substring(start, pos));
                }
                readDigits();
                isDouble = true;
            }
        }

        if (isDouble) {
            return new Double(src.substring(start, pos));
        } else {
            try {
                return new Long(src.substring(start, pos));
            } catch (NumberFormatException e) {
                // overflow, must be a double
                return new Double(src.substring(start, pos));
            }
        }
    }

    private void readDigits() {
        for (; pos < len; pos++) {
            char c = src.charAt(pos);
            if (!(c >= '0' && c <= '9')) {
                break;
            }
        }
    }

    private Boolean readJsonTrue() {
        if (len - pos < 3
                || src.charAt(pos)   != 'r'
                || src.charAt(pos+1) != 'u'
                || src.charAt(pos+2) != 'e') {
            throw new JsonException("Unexpected token: t");
        }
        pos += 3;
        return Boolean.TRUE;
    }

    private Boolean readJsonFalse() {
        if (len - pos < 4
                || src.charAt(pos)   != 'a'
                || src.charAt(pos+1) != 'l'
                || src.charAt(pos+2) != 's'
                || src.charAt(pos+3) != 'e') {
            throw new JsonException("Unexpected token: f");
        }
        pos += 3;
        return Boolean.FALSE;
    }

    private Object readJsonNull() {
        if (len - pos < 3
                || src.charAt(pos)   != 'u'
                || src.charAt(pos+1) != 'l'
                || src.charAt(pos+2) != 'l') {
            throw new JsonException("Unexpected token: n");
        }
        pos += 3;
        return null;
    }

    private char nextChar() {
        if (pos >= len) {
            throw new JsonException("Unexpected end of stream");
        }
        return src.charAt(pos++);
    }

    private void consumeChar(char token) {
        consumeWhitespace();
        if (pos >= len) {
            throw new JsonException("Expected " + token + " but reached end of stream");
        }
        char c = src.charAt(pos++);
        if (c == token) {
            return;
        } else {
            throw new JsonException("Expected " + token + " found " + c);
        }
    }

    private void consumeWhitespace() {
        while (pos < len) {
            char c = src.charAt(pos);
            switch (c) {
            case ' ':
            case '\t':
            case '\r':
            case '\n':
                pos++;
                break;
            default:
                return;
            }
        }
    }
}
