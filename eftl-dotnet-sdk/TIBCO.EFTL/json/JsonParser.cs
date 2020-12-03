/*
 * Copyright (c) 2001-$Date: 2016-03-11 16:29:10 -0800 (Fri, 11 Mar 2016) $ TIBCO Software Inc.
 * Licensed under a BSD-style license. Refer to [LICENSE]
 * For more information, please contact:
 * TIBCO Software Inc., Palo Alto, California, USA
 *
 * $Id: JsonParser.java 84680 2016-03-12 00:29:10Z bpeterse $
 */

using System;
using System.Text;

public class JsonParser
{

    private readonly int len;
    private readonly string src;

    private int pos;

    public JsonParser(string text)
    {
        pos = 0;
        len = text.Length;
        src = text;
    }

    public object Parse()
    {
        object value = ReadJson();
        ConsumeWhitespace();
        if (pos < len)
        {
            throw new JsonException("Expected end of stream at " + pos);
        }
        return value;
    }

    private object ReadJson()
    {
        ConsumeWhitespace();
        while (pos < len)
        {
            char c = src[pos++];
            switch (c)
            {
                case '{':
                    return ReadJsonObject();
                case '[':
                    return ReadJsonArray();
                case '"':
                    return ReadJsonString();
                case 't':
                    return ReadJsonTrue();
                case 'f':
                    return ReadJsonFalse();
                case 'n':
                    return ReadJsonNull();
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
                    return ReadJsonNumber(c);
                default:
                    throw new JsonException("Unexpected token: " + c);
            }
        }
        return null;
    }

    private JsonObject ReadJsonObject()
    {
        ConsumeWhitespace();

        JsonObject jsonObject = new JsonObject();

        // check for an empty object
        if (pos < len && src[pos] == '}')
        {
            pos++;
            return jsonObject;
        }

        bool needsComma = false;

        while (pos < len)
        {
            char c = src[pos++];
            switch (c)
            {
                case '}':
                    if (!needsComma)
                    {
                        throw new JsonException("Unexpected comma in object literal");
                    }
                    return jsonObject;
                case ',':
                    if (!needsComma)
                    {
                        throw new JsonException("Unexpected comma in object literal");
                    }
                    needsComma = false;
                    break;
                case '"':
                    if (needsComma)
                    {
                        throw new JsonException("Missing comma in object literal");
                    }
                    needsComma = true;
                    string name = ReadJsonString();
                    ConsumeChar(':');
                    jsonObject.Add(name, ReadJson());
                    break;
                default:
                    throw new JsonException("Unexpected token in object literal");
            }

            ConsumeWhitespace();
        }

        throw new JsonException("Unterminated object literal");
    }

    private JsonArray ReadJsonArray()
    {
        ConsumeWhitespace();

        JsonArray jsonArray = new JsonArray();

        // check for an empty array
        if (pos < len && src[pos] == ']')
        {
            pos++;
            return jsonArray;
        }

        bool needsComma = false;

        while (pos < len)
        {
            char c = src[pos];
            switch (c)
            {
                case ']':
                    if (!needsComma)
                    {
                        throw new JsonException("Unexpected comma in array literal");
                    }
                    pos++;
                    return jsonArray;
                case ',':
                    if (!needsComma)
                    {
                        throw new JsonException("Unexpected comma in array literal");
                    }
                    needsComma = false;
                    pos++;
                    break;
                default:
                    if (needsComma)
                    {
                        throw new JsonException("Missing comma in array literal");
                    }
                    needsComma = true;
                    jsonArray.Add(ReadJson());
                    break;
            }

            ConsumeWhitespace();
        }

        throw new JsonException("Unterminated array literal");
    }

    private string ReadJsonString()
    {

        int start = pos;

        // optimal case when string contains no escape characters

        while (pos < len)
        {
            char c = src[pos++];
            if (c <= '\u001F')
            {
                throw new JsonException("String contains control character");
            }
            else if (c == '\\')
            {
                break;
            }
            else if (c == '"')
            {
                return src.Substring(start, ((pos - 1) - start));
            }
        }

        // non-optimal case where string contains escape characters

        StringBuilder sb = new StringBuilder();

        while (pos < len)
        {
            sb.Append(src, start, pos - start - 1);

            if (pos >= len)
            {
                throw new JsonException("Unterminated string");
            }

            char c = src[pos++];
            switch (c)
            {
                case '"':
                    sb.Append('"');
                    break;
                case '\\':
                    sb.Append('\\');
                    break;
                case '/':
                    sb.Append('/');
                    break;
                case 'b':
                    sb.Append('\b');
                    break;
                case 'f':
                    sb.Append('\f');
                    break;
                case 'n':
                    sb.Append('\n');
                    break;
                case 'r':
                    sb.Append('\r');
                    break;
                case 't':
                    sb.Append('\t');
                    break;
                case 'u':
                    if (len - pos < 5)
                    {
                        throw new JsonException("Invalid character code: \\u" + src.Substring(pos));
                    }
                    int code = FromHex(src[pos]) << 12 |
                               FromHex(src[pos + 1]) << 8 |
                               FromHex(src[pos + 2]) << 4 |
                               FromHex(src[pos + 3]);
                    if (code < 0)
                    {
                        throw new JsonException("Invalid character code: " + src.Substring(pos, 4));
                    }
                    pos += 4;
                    sb.Append((char)code);
                    break;
                default:
                    throw new JsonException("Unexpected character in string: '\\" + c + "'");
            }

            start = pos;

            while (pos < len)
            {
                c = src[pos++];
                if (c <= '\u001F')
                {
                    throw new JsonException("String contains control character");
                }
                else if (c == '\\')
                {
                    break;
                }
                else if (c == '"')
                {
                    sb.Append(src, start, pos - start - 1);
                    return sb.ToString();
                }
            }
        }

        throw new JsonException("Unterminated string literal");
    }

    private int FromHex(char c)
    {
        return c >= '0' && c <= '9' ? c - '0'
                : c >= 'A' && c <= 'F' ? c - 'A' + 10
                        : c >= 'a' && c <= 'f' ? c - 'a' + 10
                                : -1;
    }

    private object ReadJsonNumber(char c)
    {
        int start = pos - 1;

        if (c == '-')
        {
            c = NextChar();
            if (!(c >= '0' && c <= '9'))
            {
                throw new JsonException("Invalid number format: " + src.Substring(start, (pos - start)));
            }
        }

        if (c != '0')
        {
            ReadDigits();
        }

        bool isDouble = false;

        // fraction part
        if (pos < len)
        {
            c = src[pos];
            if (c == '.')
            {
                pos++;
                c = NextChar();
                if (!(c >= '0' && c <= '9'))
                {
                    throw new JsonException("Invalid number format: " + src.Substring(start, (pos - start)));
                }
                ReadDigits();
                isDouble = true;
            }
        }

        // exponent part
        if (pos < len)
        {
            c = src[pos];
            if (c == 'e' || c == 'E')
            {
                pos++;
                c = NextChar();
                if (c == '-' || c == '+')
                {
                    c = NextChar();
                }
                if (!(c >= '0' && c <= '9'))
                {
                    throw new JsonException("Invalid number format: " + src.Substring(start, (pos - start)));
                }
                ReadDigits();
                isDouble = true;
            }
        }

        if (isDouble)
        {
            return Double.Parse(src.Substring(start, (pos - start)));
        }
        else
        {
            try
            {
                return Int64.Parse(src.Substring(start, (pos - start)));
            }
            catch (OverflowException)
            {
                // overflow, must be a Double
                return Double.Parse(src.Substring(start, (pos - start)));
            }
        }
    }

    private void ReadDigits()
    {
        for (; pos < len; pos++)
        {
            char c = src[pos];
            if (!(c >= '0' && c <= '9'))
            {
                break;
            }
        }
    }

    private bool ReadJsonTrue()
    {
        if (len - pos < 3
                || src[pos] != 'r'
                || src[pos + 1] != 'u'
                || src[pos + 2] != 'e')
        {
            throw new JsonException("Unexpected token: t");
        }
        pos += 3;
        return true;
    }

    private bool ReadJsonFalse()
    {
        if (len - pos < 4
                || src[pos] != 'a'
                || src[pos + 1] != 'l'
                || src[pos + 2] != 's'
                || src[pos + 3] != 'e')
        {
            throw new JsonException("Unexpected token: f");
        }
        pos += 3;
        return false;
    }

    private object ReadJsonNull()
    {
        if (len - pos < 3
                || src[pos] != 'u'
                || src[pos + 1] != 'l'
                || src[pos + 2] != 'l')
        {
            throw new JsonException("Unexpected token: n");
        }
        pos += 3;
        return null;
    }

    private char NextChar()
    {
        if (pos >= len)
        {
            throw new JsonException("Unexpected end of stream");
        }
        return src[pos++];
    }

    private void ConsumeChar(char token)
    {
        ConsumeWhitespace();
        if (pos >= len)
        {
            throw new JsonException("Expected " + token + " but reached end of stream");
        }
        char c = src[pos++];
        if (c == token)
        {
            return;
        }
        else
        {
            throw new JsonException("Expected " + token + " found " + c);
        }
    }

    private void ConsumeWhitespace()
    {
        while (pos < len)
        {
            char c = src[pos];
            switch (c)
            {
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
