/*
 * Copyright (c) 2001-$Date: 2020-08-20 11:01:59 -0700 (Thu, 20 Aug 2020) $ TIBCO Software Inc.
 * Licensed under a BSD-style license. Refer to [LICENSE]
 * For more information, please contact:
 * TIBCO Software Inc., Palo Alto, California, USA
 *
 * $Id: websocket.c 128030 2020-08-20 18:01:59Z bpeterse $
 *
 */

#if defined(_WIN32)
#include <winsock2.h>
#include <ws2tcpip.h>
#else
#include <netdb.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/tcp.h>
// certain platforms (AIX) require pthread.h to be included before errno.h
// for multi-threaded applications to define thread-specific errno variables.
#include <pthread.h>
#endif

#include <string.h>
#include <sys/types.h>
#include <openssl/ssl.h>
#include <openssl/err.h>
#include <errno.h>
#include <fcntl.h>

#include "base64.h"
#include "buffer.h"
#include "thread.h"
#include "url.h"
#include "websocket.h"

#if defined(_WIN32)
#define strtok_r strtok_s
#define strcasecmp _stricmp
#define strncasecmp _strnicmp
#define SHUT_WR SD_SEND
#define close closesocket
#endif

#if !defined(MSG_NOSIGNAL)
#define MSG_NOSIGNAL 0
#endif

#define MAX_FRAME_SIZE          (16*1024)
#define FIN                     (0x80)

#ifndef MIN
#define MIN(a,b) (((a) < (b)) ? (a) : (b))
#endif

typedef enum
{
    FLAG_IS_SECURE = 1 << 0,
    FLAG_SENT_CLOSE = 1 << 1

} websocket_flag;

typedef enum
{
    OP_CODE_CONT   = 0x0,
    OP_CODE_TEXT   = 0x1,
    OP_CODE_BINARY = 0x2,
    OP_CODE_CLOSE  = 0x8,
    OP_CODE_PING   = 0x9,
    OP_CODE_PONG   = 0xA

} websocket_op_code;

typedef enum
{
    FRAME_STATE_START = 0,
    FRAME_STATE_PAYLOAD_LENGTH = 1,
    FRAME_STATE_EXTENDED_PAYLOAD_LENGTH = 2,
    FRAME_STATE_MASK = 3,
    FRAME_STATE_PAYLOAD = 4

} websocket_frame_state;

typedef struct websocket_frame_s
{
    websocket_frame_state       state;
    unsigned char               fin;
    unsigned char               rsv;
    unsigned char               opcode;
    unsigned char               masked;
    unsigned int                payloadLength;
    unsigned char               mask[4];
    unsigned char*              payload;
    unsigned int                payloadSize;
    unsigned int                cursor;

} websocket_frame_t, *websocket_frame;

#define websocket_op_code_is_valid(opcode) \
    (((opcode) >= OP_CODE_CONT && (opcode) <= OP_CODE_BINARY) || \
     ((opcode) >= OP_CODE_CLOSE && (opcode) <= OP_CODE_PONG))

#define websocket_frame_is_control(frame) \
    ((frame) && ((frame)->opcode & 0x8) == 0x8)

#define websocket_frame_is_final(frame) \
    ((frame) && (frame)->fin)

struct websocket_s
{
    websocket_options_t         opts;
    websocket_frame             frame;
    websocket_frame             fragment;
    thread_t                    thread;
    unsigned int                flags;
    int                         sock;
    url_t*                      url;
    SSL_CTX*                    ctx;
    SSL*                        ssl;
    mutex_t                     mtx;
};

static thread_once_t            websocket_ssl_once;

static int
websocket_error(void)
{
#if defined(_WIN32)
    return WSAGetLastError();
#else
    return errno;
#endif
}

static char*
websocket_error_string(
    int             err,
    char*           buf,
    size_t          len)
{
    buf[0] = '\0';
#if defined(_WIN32)
    FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM |
            FORMAT_MESSAGE_IGNORE_INSERTS |
            FORMAT_MESSAGE_MAX_WIDTH_MASK,
            NULL, err,
            MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
            buf, (DWORD) len, NULL);
#else
    strerror_r(err, buf, len);
#endif
    buf[len-1] = '\0';
    return buf;
}

static void
websocket_ssl_init(void)
{
    SSL_load_error_strings();
    SSL_library_init();
}

static int
websocket_write(
    websocket_t*                ws,
    const void*                 buf,
    int                         len)
{
    int                         num = 0;

    mutex_lock(&ws->mtx);

    if (ws->flags & FLAG_IS_SECURE)
    {
        if (ws->ssl)
            num = SSL_write(ws->ssl, buf, len);

        if (num < 0)
            ERR_print_errors_fp(stderr);
    }
    else
    {
        num = send(ws->sock, buf, len, MSG_NOSIGNAL);
    }

    mutex_unlock(&ws->mtx);

    return num;
}

static int
websocket_read(
    websocket_t*                ws,
    void*                       buf,
    int                         len)
{
    int                         num = 0;

    if (ws->flags & FLAG_IS_SECURE)
    {
        if (ws->ssl)
            num = SSL_read(ws->ssl, buf, len);

        if (num < 0)
            ERR_print_errors_fp(stderr);
    }
    else
    {
        num = recv(ws->sock, buf, len, 0);
    }

    return num;
}

static void
websocket_set_blocking(
    int                         sock,
    int                         blocking)
{
#if defined(_WIN32)
    u_long                      flags;
    
    if (blocking)
        flags = 0;
    else
        flags = 1;

    ioctlsocket(sock, FIONBIO, &flags);
#else
    unsigned int                flags;

    flags = fcntl(sock, F_GETFL);

    if (blocking)
        flags &= ~O_NONBLOCK;
    else
        flags |= O_NONBLOCK;

    fcntl(sock, F_SETFL, flags);
#endif
}

static int
websocket_connect(
    websocket_t*                ws)
{
    struct addrinfo             hints, *results, *result;
    char                        buf[256];
    int                         err;
    int                         flag = 1;
    int                         sock = -1;

    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;

    if ((err = getaddrinfo(url_host(ws->url), url_port(ws->url), &hints, &results)))
    {
        if (ws->opts.error_cb)
            ws->opts.error_cb(ws, WEBSOCKET_ERR_GETADDRINFO, gai_strerror(err), ws->opts.context);
        return -1;
    }

    for (result = results; result != NULL; result = result->ai_next)
    {
        // prefer IPv4
        if (result->ai_family != AF_INET && result->ai_next != NULL)
            continue;

        if ((sock = socket(result->ai_family, result->ai_socktype, result->ai_protocol)) == -1)
            continue;

        // disable Nagle's
        setsockopt(sock, IPPROTO_TCP, TCP_NODELAY, (char*)&flag, sizeof(flag));

        // set non-blocking
        websocket_set_blocking(sock, 0);

        if (connect(sock, result->ai_addr, result->ai_addrlen) == 0)
            break;

        err = websocket_error();

#if defined(_WIN32)
        if (err == WSAEWOULDBLOCK)
#else
        if (err == EINPROGRESS)
#endif
        {
            struct timeval tv;
            fd_set fdset;
            socklen_t len = sizeof(int);
            int num;

            tv.tv_sec = ws->opts.timeout / 1000;
            tv.tv_usec = (ws->opts.timeout % 1000) * 1000;

            FD_ZERO(&fdset);
            FD_SET(sock, &fdset);

            num = select(sock+1, NULL, &fdset, NULL, &tv);
            if (num == -1)
            {
                err = websocket_error();
            }
            else if (num == 0)
            {
#if defined(_WIN32)
                err = WSAETIMEDOUT;
#else
                err = ETIMEDOUT;
#endif
            }
            else
            {
                getsockopt(sock, SOL_SOCKET, SO_ERROR, (void*)(&err), &len);
            }
        }

        if (err == 0)
            break;

        if (ws->opts.error_cb)
            ws->opts.error_cb(ws, WEBSOCKET_ERR_CONNECT, websocket_error_string(err, buf, sizeof(buf)), ws->opts.context);

        close(sock);
        sock = -1;

        break;
    }

    freeaddrinfo(results);

    if (sock == -1)
        return -1;

    // set blocking
    websocket_set_blocking(sock, 1);

    ws->sock = sock;

    if (ws->flags & FLAG_IS_SECURE)
    {
        thread_once(&websocket_ssl_once, websocket_ssl_init);

        if (ws->ctx == NULL)
        {
            X509_VERIFY_PARAM* param;

            param = X509_VERIFY_PARAM_new();
            X509_VERIFY_PARAM_set1_host(param, url_host(ws->url), strlen(url_host(ws->url)));

            ws->ctx = SSL_CTX_new(SSLv23_client_method());

            SSL_CTX_set1_param(ws->ctx, param);
            SSL_CTX_set_mode(ws->ctx, SSL_MODE_AUTO_RETRY);

            if (ws->opts.trustAll == false)
            {
                int rc;

                SSL_CTX_set_verify(ws->ctx, SSL_VERIFY_PEER, NULL);

                if (ws->opts.trustStore)
                    rc = SSL_CTX_load_verify_locations(ws->ctx, ws->opts.trustStore, NULL);
                else
                    rc = SSL_CTX_set_default_verify_paths(ws->ctx);

                if (rc != 1)
                {
                    err = ERR_get_error();

                    SSL_CTX_free(ws->ctx);

                    ws->ctx = NULL;

                    if (ws->opts.error_cb)
                        ws->opts.error_cb(ws, WEBSOCKET_ERR_TRUST_STORE, ERR_reason_error_string(err), ws->opts.context);
                }
            }

            X509_VERIFY_PARAM_free(param);
        }

        if (ws->ctx)
        {
            ws->ssl = SSL_new(ws->ctx);

            SSL_set_fd(ws->ssl, sock);

            if (SSL_connect(ws->ssl) != 1)
            {
                err = SSL_get_verify_result(ws->ssl);

                SSL_free(ws->ssl);

                ws->ssl = NULL;

                if (ws->opts.error_cb && err != X509_V_OK)
                    ws->opts.error_cb(ws, WEBSOCKET_ERR_CERTIFICATE, X509_verify_cert_error_string(err), ws->opts.context);
            }
        }
    }

    return 0;
}

static int
websocket_handshake(
    websocket_t*                ws)
{
    unsigned char               nonce[16];
    char                        key[BASE64_ENC_MAX_LEN(sizeof(nonce))];
    char                        buf[1024];
    char                        request[1024];
    char                        response[1024];
    char                        sha[20];
    char                        expected_key[BASE64_ENC_MAX_LEN(sizeof(sha))];
    char*                       tok;
    char*                       ptr;
    char*                       saveptr = NULL;
    unsigned int                flags = 0;
    int                         i;
    int                         n;

#define HAS_UPGRADE    (1 << 0)
#define HAS_CONNECTION (1 << 1)
#define VALID_ACCEPT   (1 << 2)

    static const char*          GUID = "258EAFA5-E914-47DA-95CA-C5AB0DC85B11";

    for (i = 0; i < 16; i++)
        nonce[i] = (unsigned int)rand() & 0xff;

    base64_encode(nonce, 16, key, sizeof(key));

    snprintf(request, 1024, "GET %s HTTP/1.1\r\nUpgrade: websocket\r\nConnection: Upgrade\r\nHost: %s:%s\r\nSec-WebSocket-Key: %s\r\nSec-WebSocket-Version: 13\r\n\r\n",
            url_path(ws->url), url_host(ws->url), url_port(ws->url), key);

    websocket_write(ws, request, strlen(request));

    i = 0;

    do
    {
        n = websocket_read(ws, response + i, 1023 - i);
        i += n;

        // null terminate the buffer
        response[i] = '\0';

    } while((i < 4 || strstr(response, "\r\n\r\n") == NULL) && n > 0);

    if (n == 0)
    {
        if (ws->opts.error_cb)
            ws->opts.error_cb(ws, WEBSOCKET_ERR_SOCKET, "Connection closed", ws->opts.context);
        return -1;
    }
    if (n < 0)
    {
        char errbuf[256];
        int err = websocket_error();

        if (ws->opts.error_cb)
            ws->opts.error_cb(ws, WEBSOCKET_ERR_SOCKET, websocket_error_string(err, errbuf, sizeof(errbuf)), ws->opts.context);
        return -1;
    }

    snprintf(buf, sizeof(buf), "%s%s", key, GUID);
    SHA1((unsigned char*)buf, strlen(buf), (unsigned char*)sha);
    base64_encode((unsigned char*)sha, 20, expected_key, sizeof(expected_key));

    for (tok = strtok_r(response, "\r\n", &saveptr); tok != NULL; tok = strtok_r(NULL, "\r\n", &saveptr))
    {
        if (*tok == 'H' && *(tok+1) == 'T' && *(tok+2) == 'T' && *(tok+3) == 'P')
        {
            char* status;
            char* reason;

            ptr = strchr(tok, ' ');
            status = ptr + 1;
            ptr = strchr(ptr+1, ' ');
            reason = ptr + 1;
            *ptr = '\0';

            if (strncmp(status, "101", 3) != 0)
            {
                if (ws->opts.error_cb)
                    ws->opts.error_cb(ws, WEBSOCKET_ERR_HANDSHAKE_STATUS, reason, ws->opts.context);
                return -1;
            }
        }
        else if ((ptr = strchr(tok, ' ')))
        {
            *ptr = '\0';
            if (strcasecmp(tok, "Upgrade:") == 0)
            {
                if(strcasecmp(ptr+1, "websocket") == 0)
                {
                    flags |= HAS_UPGRADE;
                }
            }
            if (strcasecmp(tok, "Connection:") == 0)
            {
                if(strcasecmp(ptr+1, "upgrade") == 0)
                {
                    flags |= HAS_CONNECTION;
                }
            }
            if (strcasecmp(tok, "Sec-WebSocket-Accept:") == 0)
            {
                if (strcmp(ptr+1, expected_key) == 0)
                {
                    flags |= VALID_ACCEPT;
                }
            }
        }
    }
    if (!(flags & HAS_UPGRADE))
    {
        if (ws->opts.error_cb)
            ws->opts.error_cb(ws, WEBSOCKET_ERR_HANDSHAKE_HEADER, "missing required Upgrade header", ws->opts.context);
        return -1;
    }
    if (!(flags & HAS_CONNECTION))
    {
        if (ws->opts.error_cb)
            ws->opts.error_cb(ws, WEBSOCKET_ERR_HANDSHAKE_HEADER, "missing required Connection header", ws->opts.context);
        return -1;
    }
    if (!(flags & VALID_ACCEPT))
    {
        if (ws->opts.error_cb)
            ws->opts.error_cb(ws, WEBSOCKET_ERR_HANDSHAKE_HEADER, "missing required Sec-WebSocket-Accept header", ws->opts.context);
        return -1;
    }

    return 0;
}

static int
websocket_write_frame(
    websocket_t*            ws,
    unsigned char           opcode,
    unsigned char*          data,
    unsigned int            size)
{
    int                     maskOffset;
    int                     dataOffset;
    unsigned int            mask;
    unsigned char*          buf;
    int                     num;

    maskOffset = (size < 126 ? 2 : (size < 65536 ? 4 : 10));
    dataOffset = maskOffset + 4; // plus 4 for mask

    buf = malloc(dataOffset + size); // plus 4 for mask
    if (!buf)
        return -1;

    buf[0] = opcode;

    if (size < 126)
    {
        buf[1] = (0x80 | size);
    }
    else if (size < 65536)
    {
        buf[1] = (0x80 | 126);
        buf[2] = ((size >> 8) & 0xFF);
        buf[3] = ((size >> 0) & 0xFF);
    }
    else
    {
        buf[1] = (0x80 | 127);
        buf[2] = (((uint64_t)size >> 56) & 0xFF);
        buf[3] = (((uint64_t)size >> 48) & 0xFF);
        buf[4] = (((uint64_t)size >> 40) & 0xFF);
        buf[5] = (((uint64_t)size >> 32) & 0xFF);
        buf[6] = (((uint64_t)size >> 24) & 0xFF);
        buf[7] = (((uint64_t)size >> 16) & 0xFF);
        buf[8] = (((uint64_t)size >>  8) & 0xFF);
        buf[9] = (((uint64_t)size >>  0) & 0xFF);
    }

    mask = rand();

    memcpy(buf + maskOffset, &mask, 4);

    if (data && size > 0)
    {
        int i;

        for (i = 0; i < size; i++)
            buf[dataOffset + i] = (data[i] ^ buf[maskOffset + (i % 4)]);
    }

    num = websocket_write(ws, buf, (dataOffset + size));

    free(buf);

    return (num > 0 ? 0 : -1);
}

static int
websocket_send_frame(
    websocket_t*                ws,
    unsigned char               opcode,
    unsigned char*              data,
    int                         size)
{
    int                         remaining = size;
    unsigned char*              fragment = data;

    while (remaining > MAX_FRAME_SIZE)
    {
        websocket_write_frame(ws, opcode, fragment, MAX_FRAME_SIZE);

        fragment += MAX_FRAME_SIZE;
        remaining -= MAX_FRAME_SIZE;

        opcode = OP_CODE_CONT;
    }

    return websocket_write_frame(ws, (opcode | FIN), fragment, remaining);
}

static int
websocket_send_close(
    websocket_t*                ws,
    unsigned int                code,
    const char*                 reason)
{
    unsigned char               buf[125];
    int                         len = 2;

    if (ws->flags & FLAG_SENT_CLOSE)
        return 0;

    ws->flags |= FLAG_SENT_CLOSE;

    buf[0] = (unsigned char)((code >> 8) & 0xFF);
    buf[1] = (unsigned char)((code >> 0) & 0xFF);

    if (reason)
    {
        len += strlen(reason);

        if (len > 125)
            len = 125;

        memcpy(&buf[2], reason, MIN(len-2, (int)strlen(reason)));
    }

    websocket_send_frame(ws, OP_CODE_CLOSE, buf, len);

    shutdown(ws->sock, SHUT_WR);

    return 0;
}

static websocket_frame
websocket_frame_create(void)
{
    websocket_frame             frame;

    frame = calloc(1, sizeof(*frame));

    frame->payloadSize = 1024;
    frame->payload = malloc(frame->payloadSize);
    frame->state = FRAME_STATE_START;

    return frame;
}

static void
websocket_frame_destroy(
    websocket_frame             frame)
{
    if (!frame)
        return;

    free(frame->payload);
    free(frame);
}

/**
 * The wire format for a WebSocket frame is defined as follows:
 *
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
static websocket_frame
websocket_parse_frame(
    websocket_t*            ws,
    buffer_t*               buffer)
{
    websocket_frame         frame;
    unsigned char           byte;
    int                     size;

    frame = ws->frame;

    if (!frame)
    {
        frame = ws->frame = websocket_frame_create();
    }

    while (buffer_remaining(buffer) > 0)
    {
        switch (frame->state)
        {
        case FRAME_STATE_START:

            byte = *(buffer_position(buffer)++);

            frame->fin = (byte & 0x80);
            frame->rsv = (byte & 0x70);
            frame->opcode = (byte & 0x0F);
            frame->state = FRAME_STATE_PAYLOAD_LENGTH;

            if (frame->rsv != 0)
            {
                websocket_send_close(ws, WEBSOCKET_CLOSE_PROTOCOL_ERROR, "frame contains nonzero rsv bits");
                return NULL;
            }

            if (!websocket_op_code_is_valid(frame->opcode))
            {
                websocket_send_close(ws, WEBSOCKET_CLOSE_PROTOCOL_ERROR, "frame contains invalid opcode");
                return NULL;
            }

            if (websocket_frame_is_control(frame) && !websocket_frame_is_final(frame))
            {
                websocket_send_close(ws, WEBSOCKET_CLOSE_PROTOCOL_ERROR, "control frame is fragmented");
                return NULL;
            }
            break;

        case FRAME_STATE_PAYLOAD_LENGTH:

            byte = *(buffer_position(buffer)++);

            frame->masked = (byte & 0x80);
            frame->payloadLength = (byte & 0x7F);
            if (frame->payloadLength == 126)
            {
                if (websocket_frame_is_control(frame))
                {
                    websocket_send_close(ws, WEBSOCKET_CLOSE_MESSAGE_TOO_BIG, "control frame is too big");
                    return NULL;
                }

                frame->payloadLength = 0;
                frame->cursor = 2;
                frame->state = FRAME_STATE_EXTENDED_PAYLOAD_LENGTH;
            }
            else if (frame->payloadLength == 127)
            {
                if (websocket_frame_is_control(frame))
                {
                    websocket_send_close(ws, WEBSOCKET_CLOSE_MESSAGE_TOO_BIG, "control frame is too big");
                    return NULL;
                }

                frame->payloadLength = 0;
                frame->cursor = 8;
                frame->state = FRAME_STATE_EXTENDED_PAYLOAD_LENGTH;
            }
            else
            {
                if (frame->masked)
                {
                    frame->cursor = 4;
                    frame->state = FRAME_STATE_MASK;
                }
                else if (frame->payloadLength)
                {
                    frame->cursor = 0;
                    frame->state = FRAME_STATE_PAYLOAD;
                }
                else
                {
                    frame->state = FRAME_STATE_START;
                    return frame;
                }
            }
            break;

        case FRAME_STATE_EXTENDED_PAYLOAD_LENGTH:

            byte = *(buffer_position(buffer)++);

            frame->payloadLength |= ((unsigned int)byte & 0xFF) << (8 * --frame->cursor);
            if (frame->cursor == 0)
            {
                if (frame->masked)
                {
                    frame->cursor = 4;
                    frame->state = FRAME_STATE_MASK;
                }
                else if (frame->payloadLength)
                {
                    frame->cursor = 0;
                    frame->state = FRAME_STATE_PAYLOAD;
                }
                else
                {
                    frame->state = FRAME_STATE_START;
                    return frame;
                }
            }
            break;

        case FRAME_STATE_MASK:

            byte = *(buffer_position(buffer)++);

            frame->mask[4 - frame->cursor--] = byte;
            if (frame->cursor == 0)
            {
                if (frame->payloadLength)
                {
                    frame->cursor = 0;
                    frame->state = FRAME_STATE_PAYLOAD;
                }
                else
                {
                    frame->state = FRAME_STATE_START;
                    return frame;
                }
            }
            break;

        case FRAME_STATE_PAYLOAD:

            if (frame->payloadSize < frame->payloadLength)
            {
                while (frame->payloadSize < frame->payloadLength)
                    frame->payloadSize <<= 1;

                frame->payload = realloc(frame->payload, frame->payloadSize + 1);
            }

            size = MIN(buffer_remaining(buffer), (frame->payloadLength - frame->cursor));
            memcpy(frame->payload + frame->cursor, buffer_position(buffer), size);
            frame->cursor += size;
            buffer_position(buffer) += size;

            if (frame->cursor == frame->payloadLength)
            {
                frame->payload[frame->payloadLength] = '\0';

                if (frame->masked)
                {
                    int i;

                    for (i = 0; i < frame->payloadLength; i++)
                        frame->payload[i] = (frame->payload[i] ^ frame->mask[i % 4]);
                }

                frame->state = FRAME_STATE_START;
                return frame;
            }
        }
    }

    return NULL;
}

static websocket_frame
websocket_append_fragment(
    websocket_t*            ws,
    websocket_frame         fragment)
{
    websocket_frame         frame;

    frame = ws->fragment;

    if (!frame)
    {
        frame = ws->fragment = websocket_frame_create();
    }

    if (fragment->opcode != OP_CODE_CONT)
    {
        frame->opcode = fragment->opcode;
        frame->payloadLength = 0;
    }

    if (frame->payloadSize < frame->payloadLength + fragment->payloadLength)
    {
        while (frame->payloadSize < frame->payloadLength + fragment->payloadLength)
            frame->payloadSize <<= 1;

        frame->payload = realloc(frame->payload, frame->payloadSize + 1);
    }

    memcpy(frame->payload + frame->payloadLength, fragment->payload, fragment->payloadLength);

    frame->payloadLength += fragment->payloadLength;
    frame->payload[frame->payloadLength] = '\0';

    frame->fin = fragment->fin;

    return frame;
}

static void
websocket_handle_text(
    websocket_t*            ws,
    websocket_frame         frame)
{
    if (ws->opts.text_cb)
        ws->opts.text_cb(ws, (const char*)frame->payload, ws->opts.context);
}

static void
websocket_handle_binary(
    websocket_t*            ws,
    websocket_frame         frame)
{
    if (ws->opts.binary_cb)
        ws->opts.binary_cb(ws, frame->payload, frame->payloadLength, ws->opts.context);
}

static void
websocket_handle_close(
    websocket_t*            ws,
    websocket_frame         frame)
{
    unsigned int            code = 0;
    char                    reason[128];
    unsigned int            len;

    if (frame->payloadLength >= 2)
    {
        code |= (unsigned int)(frame->payload[0] & 0xFF) << 8;
        code |= (unsigned int)(frame->payload[1] & 0xFF);
    }
    else
    {
        code = WEBSOCKET_CLOSE_NO_STATUS_RECEIVED;
    }

    if (frame->payloadLength > 2)
    {
        len = MIN((int)sizeof(reason)-1, frame->payloadLength-2);
        memcpy(reason, &frame->payload[2], len);
        reason[len] = '\0';
    }
    else
    {
        reason[0] = '\0';
    }

    websocket_send_close(ws, code, NULL);

    if (ws->opts.close_cb)
        ws->opts.close_cb(ws, code, reason, ws->opts.context);
}

static void
websocket_handle_ping(
    websocket_t*            ws,
    websocket_frame         frame)
{
    websocket_send_frame(ws, OP_CODE_PONG, frame->payload, frame->payloadLength);
}

static void
websocket_handle_frame(
    websocket_t*            ws,
    websocket_frame         frame)
{
    switch (frame->opcode)
    {
    case OP_CODE_CONT:
        frame = websocket_append_fragment(ws, frame);
        if (frame && websocket_frame_is_final(frame))
            websocket_handle_frame(ws, frame);
        break;
    case OP_CODE_TEXT:
        if (websocket_frame_is_final(frame))
            websocket_handle_text(ws, frame);
        else
            websocket_append_fragment(ws, frame);
        break;
    case OP_CODE_BINARY:
        if (websocket_frame_is_final(frame))
            websocket_handle_binary(ws, frame);
        else
            websocket_append_fragment(ws, frame);
        break;
    case OP_CODE_CLOSE:
        websocket_handle_close(ws, frame);
         break;
    case OP_CODE_PING:
        websocket_handle_ping(ws, frame);
        break;
    case OP_CODE_PONG:
        break;
    }
}

static void*
websocket_run(
    void*                       arg)
{
    websocket_t*                ws = (websocket_t*)arg;
    websocket_frame             frame;
    buffer_t*                   buffer;
    int                         num = 0;

#if defined(_WIN32)
    WSADATA WSAData;

    WSAStartup(MAKEWORD(2,0), &WSAData);
#endif

    ws->flags &= ~FLAG_SENT_CLOSE;

    if (websocket_connect(ws) != 0)
    {
        // connect failed
    }
    else if (websocket_handshake(ws) != 0)
    {
        // handshake failed
    }
    else
    {
        if (ws->opts.open_cb)
            ws->opts.open_cb(ws, ws->opts.context);

        buffer = buffer_create(1024);

        while (!(ws->flags & FLAG_SENT_CLOSE) && (num = websocket_read(ws, buffer_data(buffer), buffer_size(buffer))) > 0)
        {
            buffer_ready(buffer, num);

            while ((frame = websocket_parse_frame(ws, buffer)))
                websocket_handle_frame(ws, frame);
        }

        if (num <= 0)
        {
            char buf[256];
            int err = websocket_error();

            if (ws->opts.error_cb)
                ws->opts.error_cb(ws, WEBSOCKET_ERR_SOCKET, websocket_error_string(err, buf, sizeof(buf)), ws->opts.context);
        }

        buffer_destroy(buffer);
    }

    mutex_lock(&ws->mtx);

    close(ws->sock);
    ws->sock = -1;

    if (ws->ssl)
    {
        SSL_free(ws->ssl);
        ws->ssl = NULL;
    }

    mutex_unlock(&ws->mtx);

#if defined(_WIN32)
    WSACleanup();
#endif

    return NULL;
}

static int
websocket_start(
    websocket_t*                ws)
{
    thread_start(&ws->thread, websocket_run, ws);

    return 0;
}

static int
websocket_stop(
    websocket_t*                ws)
{
    websocket_send_close(ws, WEBSOCKET_CLOSE_NORMAL, NULL);

    thread_join(ws->thread);
    thread_destroy(ws->thread);

    return 0;
}

websocket_t*
websocket_create(
    websocket_options_t*        opts)
{
    websocket_t*                ws;

    ws = calloc(1, sizeof(*ws));

    mutex_init(&ws->mtx);

    if (opts && opts->ver == 0)
    {
        if (opts->origin)
            ws->opts.origin = strdup(opts->origin);
        if (opts->protocol)
            ws->opts.protocol = strdup(opts->protocol);
        if (opts->trustStore)
            ws->opts.trustStore = strdup(opts->trustStore);

        ws->opts.trustAll = opts->trustAll;

        ws->opts.timeout = opts->timeout;

        ws->opts.open_cb = opts->open_cb;
        ws->opts.text_cb = opts->text_cb;
        ws->opts.binary_cb = opts->binary_cb;
        ws->opts.error_cb = opts->error_cb;
        ws->opts.close_cb = opts->close_cb;
        ws->opts.context = opts->context;
    }

    return ws;
}

void
websocket_destroy(
    websocket_t*                ws)
{
    if (!ws)
        return;

    if (ws->ctx)
        SSL_CTX_free(ws->ctx);

    url_destroy(ws->url);

    mutex_destroy(&ws->mtx);

    websocket_frame_destroy(ws->frame);
    websocket_frame_destroy(ws->fragment);

    if (ws->opts.origin)
        free((char*)ws->opts.origin);
    if (ws->opts.protocol)
        free((char*)ws->opts.protocol);
    if (ws->opts.trustStore)
        free((char*)ws->opts.trustStore);

    free(ws);
}

int
websocket_set_timeout(
    websocket_t*                ws,
    int                         to)
{
#if defined(_WIN32)
    DWORD timeout = to * 1000;
#else
    struct timeval timeout;
    timeout.tv_sec = to;
    timeout.tv_usec = 0;
#endif
    return setsockopt(ws->sock, SOL_SOCKET, SO_RCVTIMEO, (const char*)&timeout, sizeof(timeout));
}

int
websocket_open(
    websocket_t*                ws,
    url_t*                      url)
{
    if (ws->url)
        url_destroy(ws->url);

    ws->url = url_copy(url);

    if (url_is_secure(ws->url))
        ws->flags |= FLAG_IS_SECURE;
    else
        ws->flags &= ~(FLAG_IS_SECURE);

    return websocket_start(ws);
}

int
websocket_close(
    websocket_t*                ws)
{
    return websocket_stop(ws);
}

int
websocket_send_text(
    websocket_t*                ws,
    const char*                 text)
{
    return websocket_send_frame(ws, OP_CODE_TEXT, (unsigned char*)text, strlen(text));
}

int
websocket_send_binary(
    websocket_t*                ws,
    const void*                 data,
    unsigned int                size)
{
    return websocket_send_frame(ws, OP_CODE_BINARY, (unsigned char*)data, size);
}

url_t*
websocket_url(
    websocket_t*                websocket)
{
    if (!websocket)
        return NULL;

    return websocket->url;
}

