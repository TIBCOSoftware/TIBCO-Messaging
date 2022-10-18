/*
 * Copyright (c) 2001-$Date: 2022-01-14 14:03:58 -0800 (Fri, 14 Jan 2022) $ TIBCO Software Inc.
 * Licensed under a BSD-style license. Refer to [LICENSE]
 * For more information, please contact:
 * TIBCO Software Inc., Palo Alto, California, USA
 *
 * $Id: websocket.h 138851 2022-01-14 22:03:58Z $
 *
 */

#ifndef INCLUDED_TIBEFTL_WS_H
#define INCLUDED_TIBEFTL_WS_H

#include <inttypes.h>
#include <stdbool.h>

#include "url.h"

typedef struct websocket_s websocket_t;

#define WEBSOCKET_ERR_GETADDRINFO          1
#define WEBSOCKET_ERR_CONNECT              2
#define WEBSOCKET_ERR_TRUST_STORE          3
#define WEBSOCKET_ERR_CERTIFICATE          4
#define WEBSOCKET_ERR_HANDSHAKE_STATUS     5
#define WEBSOCKET_ERR_HANDSHAKE_HEADER     6
#define WEBSOCKET_ERR_SOCKET               7

#define WEBSOCKET_CLOSE_NORMAL             1000
#define WEBSOCKET_CLOSE_GOING_AWAY         1001
#define WEBSOCKET_CLOSE_PROTOCOL_ERROR     1002
#define WEBSOCKET_CLOSE_UNSUPPORTED_DATA   1003
#define WEBSOCKET_CLOSE_RESERVED           1004
#define WEBSOCKET_CLOSE_NO_STATUS_RECEIVED 1005
#define WEBSOCKET_CLOSE_ABNORMAL_CLOSE     1006
#define WEBSOCKET_CLOSE_INVALID_DATA       1007
#define WEBSOCKET_CLOSE_POLICY_VIOLATION   1008
#define WEBSOCKET_CLOSE_MESSAGE_TOO_BIG    1009
#define WEBSOCKET_CLOSE_EXTENSION_REQUIRED 1010
#define WEBSOCKET_CLOSE_INTERNAL_ERROR     1011
#define WEBSOCKET_CLOSE_TLS_FAILURE        1015

typedef void (*websocket_open_cb)(
    websocket_t*                websocket,
    void*                       context);

typedef void (*websocket_text_cb)(
    websocket_t*                websocket,
    const char*                 text,
    void*                       context);

typedef void (*websocket_binary_cb)(
    websocket_t*                websocket,
    const unsigned char*        data,
    int                         size,
    void*                       context);

typedef void (*websocket_error_cb)(
    websocket_t*                websocket,
    int                         error,
    const char*                 reason,
    void*                       context);

typedef void (*websocket_close_cb)(
    websocket_t*                websocket,
    int                         code,
    const char*                 reason,
    void*                       context);

typedef struct websocket_options_s
{
    const char*                 username;
    const char*                 password;
    const char*                 clientId;

    const char*                 origin;
    const char*                 protocol;
    const char*                 trustStore;
    bool                        trustAll;

    int64_t                     timeout;

    websocket_open_cb           open_cb;
    websocket_text_cb           text_cb;
    websocket_binary_cb         binary_cb;
    websocket_error_cb          error_cb;
    websocket_close_cb          close_cb;

    void*                       context;

} websocket_options_t;

#define websocket_options_init(opts) \
    ((opts) = (websocket_options_t){ NULL, NULL, NULL, NULL, NULL, NULL, false, 10000, NULL, NULL, NULL, NULL, NULL, NULL })

websocket_t*
websocket_create(
    websocket_options_t*        opts);

void
websocket_destroy(
    websocket_t*                websocket);

int
websocket_set_timeout(
    websocket_t*                websocket,
    int                         timeout);

int
websocket_open(
    websocket_t*                websocket,
    url_t*                      url);

int
websocket_close(
    websocket_t*                websocket);

int
websocket_send_text(
    websocket_t*                websocket,
    const char*                 text);

int
websocket_send_binary(
    websocket_t*                websocket,
    const void*                 data,
    unsigned int                size);

url_t*
websocket_url(
    websocket_t*                websocket);

#endif /* INCLUDED_TIBEFTL_WS_H */
