/*
 * Copyright (c) 2001-$Date: 2018-06-06 13:30:42 -0500 (Wed, 06 Jun 2018) $ TIBCO Software Inc.
 * Licensed under a BSD-style license. Refer to [LICENSE]
 * For more information, please contact:
 * TIBCO Software Inc., Palo Alto, California, USA
 *
 * $Id: eftl.c 101572 2018-06-06 18:30:42Z bpeterse $
 *
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include <inttypes.h>
#include <math.h>

#include "websocket.h"
#include "hashmap.h"
#include "thread.h"
#include "cJSON.h"
#include "eftl.h"
#include "url.h"
#include "msg.h"
#include "version.h"

#if defined(_WIN32)
#define strcasecmp _stricmp
#endif

#define TIBEFTL_PROTOCOL        "v1.eftl.tibco.com"

#define TIBEFTL_TIMEOUT         (10000)

#define OP_HEARTBEAT            (0)
#define OP_LOGIN                (1)
#define OP_WELCOME              (2)
#define OP_SUBSCRIBE            (3)
#define OP_SUBSCRIBED           (4)
#define OP_UNSUBSCRIBE          (5)
#define OP_UNSUBSCRIBED         (6)
#define OP_MESSAGE              (7)
#define OP_PUBLISH              (8)
#define OP_ACK                  (9)
#define OP_ERROR                (10)
#define OP_DISCONNECT           (11)
#define OP_MAP_SET              (20)
#define OP_MAP_GET              (22)
#define OP_MAP_REMOVE           (24)
#define OP_MAP_RESPONSE         (26)

typedef struct tibeftlCompletionStruct
{
    int                         code;
    char*                       reason;
    tibeftlMessage              response;
    semaphore_t                 semaphore;

} tibeftlCompletionRec, *tibeftlCompletion;

typedef void tibeftlConnection_OnCompletion(
    tibeftlConnection           conn,
    int                         code,
    const char*                 reason,
    void*                       arg);

typedef struct tibeftlRequestStruct tibeftlRequestRec, *tibeftlRequest;

struct tibeftlRequestStruct
{
    tibeftlRequest              next;
    tibeftlCompletionRec        completion;
    int64_t                     seqNum;
    char*                       text;
};

typedef struct tibeftlRequestListStruct
{
    tibeftlRequest              head;
    tibeftlRequest              tail;

} tibeftlRequestListRec;

typedef enum tibeftlConnectionStateEnum
{
    STATE_UNCONNECTED,
    STATE_DISCONNECTED,
    STATE_CONNECTING,
    STATE_CONNECTED,
    STATE_DISCONNECTING,
    STATE_AUTORECONNECTING,
    STATE_RECONNECTING

} tibeftlConnectionState;

struct tibeftlConnectionStruct
{
    int                         ref;
    url_t*                      url;
    tibeftlErrorCallback        errCb;
    void*                       errCbArg;
    tibeftlCompletionRec        completion;
    tibeftlConnectionState      state;
    mutex_t                     mutex;
    websocket_t*                websocket;
    tibeftlRequestListRec       requestList;
    hashmap_t*                  subscriptions;
    char*                       username;
    char*                       password;
    char*                       clientId;
    char*                       trustStore;
    char*                       reconnectToken;
    thread_t                    autoReconnectThread;
    int                         maxMsgSize;
    int64_t                     timeout;
    int64_t                     pubSeqNum;
    int64_t                     lastSubId;
    int64_t                     lastSeqNum;
    void*                       onConnectArg;
    int64_t                     autoReconnectAttempts;
    int64_t                     autoReconnectMaxDelay;
    int64_t                     reconnectAttempts;
    tibeftlCompletionRec        autoReconnectCompletion;
};

struct tibeftlSubscriptionStruct
{
    char*                       subId;
    char*                       matcher;
    char*                       durable;
    char*                       durableType;
    char*                       durableKey;
    tibeftlMessageCallback      msgCb;
    void*                       msgCbArg;
    tibeftlCompletionRec        completion;
};

struct tibeftlErrStruct
{
    int                         code;
    char*                       desc;
};

struct tibeftlKVMapStruct
{
    char*                       name;
    tibeftlConnection           conn;
};

static tibeftlSubscription
tibeftlSubscription_create(
    int64_t                     subId,
    const char*                 matcher,
    const char*                 durable,
    tibeftlSubscriptionOptions* opts,
    tibeftlMessageCallback      msgCb,
    void*                       msgCbArg)
{
    tibeftlSubscription         sub;
    size_t                      size;

    sub = calloc(1, sizeof(*sub));

    size = snprintf(NULL, 0, "%" PRId64, subId);
    sub->subId = malloc(size + 1);
    sprintf(sub->subId, "%" PRId64, subId);

    if (matcher)
        sub->matcher = strdup(matcher);
    if (durable)
        sub->durable = strdup(durable);
    if (opts && opts->durableType)
        sub->durableType = strdup(opts->durableType);
    if (opts && opts->durableKey)
        sub->durableKey = strdup(opts->durableKey);

    sub->msgCb = msgCb;
    sub->msgCbArg = msgCbArg;

    return sub;
}

static void
tibeftlSubscription_destroy(
    tibeftlSubscription         sub)
{
    if (!sub)
        return;

    if (sub->subId)
        free(sub->subId);
    if (sub->matcher)
        free(sub->matcher);
    if (sub->durable)
        free(sub->durable);
    if (sub->durableType)
        free(sub->durableType);
    if (sub->durableKey)
        free(sub->durableKey);

    free(sub);
}

static void
tibeftlCompletion_init(
    tibeftlCompletion           completion)
{
    memset(completion, 0, sizeof(*completion));
    completion->semaphore = semaphore_create();
}

static void
tibeftlCompletion_clear(
    tibeftlCompletion           completion)
{
    if (completion->reason)
    {
        free(completion->reason);
        completion->reason = NULL;
    }
    if (completion->semaphore)
    {
        semaphore_destroy(completion->semaphore);
        completion->semaphore = NULL;
    }
}

static int
tibeftlCompletion_wait(
    tibeftlCompletion           completion,
    int                         millis)
{
    return (semaphore_wait(completion->semaphore, millis) ? TIBEFTL_ERR_TIMEOUT : completion->code);
}

static void
tibeftlCompletion_notify(
    tibeftlCompletion           completion,
    tibeftlMessage              response,
    int                         code,
    const char*                 reason)
{
    completion->response = response;
    completion->code = code;

    if (completion->reason)
    {
        free(completion->reason);
        completion->reason = NULL;
    }

    if (reason)
        completion->reason = strdup(reason);

    if (completion->semaphore)
        semaphore_post(completion->semaphore);
}

static tibeftlCompletion
tibeftlConnection_registerSubscription(
    tibeftlConnection           conn,
    tibeftlSubscription         sub)
{
    if (!conn || !sub)
        return NULL;

    tibeftlCompletion_init(&sub->completion);

    hashmap_put(conn->subscriptions, sub->subId, sub);

    return &sub->completion;
}

static void
tibeftlConnection_unregisterSubscription(
    tibeftlConnection           conn,
    const char*                 subId)
{
    tibeftlSubscription         sub;

    if (!conn || !subId)
        return;

    sub = hashmap_remove(conn->subscriptions, subId);

    if (sub)
    {
        tibeftlCompletion_clear(&sub->completion);

        tibeftlSubscription_destroy(sub);
    }
}

static void
tibeftlConnection_notifySubscribed(
    tibeftlConnection           conn,
    const char*                 subId,
    int                         code,
    const char*                 reason)
{
    tibeftlSubscription         sub;

    if (!conn || !subId)
        return;

    sub = hashmap_get(conn->subscriptions, subId);

    if (sub)
        tibeftlCompletion_notify(&sub->completion, NULL, code, reason);
}


static tibeftlCompletion
tibeftlConnection_registerRequest(
    tibeftlConnection           conn,
    int64_t                     seqNum,
    char*                       text)
{
    tibeftlRequest              request;

    if (!conn || !seqNum)
        return NULL;

    request = calloc(1, sizeof(*request));

    request->seqNum = seqNum;
    request->text = text;
    request->next = NULL;

    tibeftlCompletion_init(&request->completion);

    if (conn->requestList.tail)
    {
        conn->requestList.tail->next = request;
        conn->requestList.tail = request;
    }
    else
    {
        conn->requestList.head = request;
        conn->requestList.tail = request;
    }

    return &request->completion;
}

static void
tibeftlConnection_unregisterRequest(
    tibeftlConnection           conn,
    int64_t                     seqNum)
{
    tibeftlRequest              current;

    if (!conn || !seqNum)
        return;

    if (conn->requestList.head && conn->requestList.head->seqNum == seqNum)
    {
        current = conn->requestList.head;
        conn->requestList.head = current->next;
        if (conn->requestList.head == NULL)
            conn->requestList.tail = NULL;
    }
    else
    {
        for (current = conn->requestList.head; current; current = current->next)
        {
            if (current->seqNum == seqNum)
            {
                break;
            }
        }
    }

    if (current)
    {
        tibeftlCompletion_clear(&current->completion);

        _cJSON_free(current->text);

        free(current);
    }
}

static void
tibeftlConnection_notifyRequest(
    tibeftlConnection           conn,
    int64_t                     seqNum,
    tibeftlMessage              response,
    int                         code,
    const char*                 reason)
{
    tibeftlRequest              request;

    if (!conn)
        return;

    for (request = conn->requestList.head; request; request = request->next)
    {
        if (request->seqNum == seqNum)
            tibeftlCompletion_notify(&request->completion, response, code, reason);
    }
}

static void
tibeftlConnection_notifyAllRequests(
    tibeftlConnection           conn,
    int                         code,
    const char*                 reason)
{
    tibeftlRequest              request;

    if (!conn)
        return;

    for (request = conn->requestList.head; request; request = request->next)
    {
        tibeftlCompletion_notify(&request->completion, NULL, code, reason);
    }
}

static void
tibeftlConnection_resendAllRequests(
    tibeftlConnection           conn)
{
    tibeftlRequest              publish;

    if (!conn)
        return;

    for (publish = conn->requestList.head; publish; publish = publish->next)
    {
        websocket_send_text(conn->websocket, publish->text);
    }
}

static void
tibeftlSubscription_subscribe(
    const char*                 key,
    void*                       value,
    void*                       context)
{
    tibeftlConnection           conn = (tibeftlConnection)context;
    tibeftlSubscription         sub = (tibeftlSubscription)value;
    _cJSON*                     json;
    char*                       text;

    json = _cJSON_CreateObject();
    _cJSON_AddNumberToObject(json, "op", OP_SUBSCRIBE);
    _cJSON_AddStringToObject(json, "id", sub->subId);
    if (sub->matcher)
        _cJSON_AddStringToObject(json, "matcher", sub->matcher);
    if (sub->durable)
        _cJSON_AddStringToObject(json, "durable", sub->durable);

    text = _cJSON_PrintUnformatted(json);

    websocket_send_text(conn->websocket, text);

    _cJSON_Delete(json);
    _cJSON_free(text);
}

static void
tibeftlSubscription_cleanup(
    const char*                 key,
    void*                       value,
    void*                       context)
{
    tibeftlSubscription         sub = (tibeftlSubscription)value;

    tibeftlCompletion_clear(&sub->completion);

    tibeftlSubscription_destroy(sub);
}

static void
tibeftlSubscription_unsubscribe(
    const char*                 key,
    void*                       value,
    void*                       context)
{
    tibeftlConnection           conn = (tibeftlConnection)context;
    _cJSON*                     json;
    char*                       text;

    json = _cJSON_CreateObject();
    _cJSON_AddNumberToObject(json, "op", OP_UNSUBSCRIBE);
    _cJSON_AddStringToObject(json, "id", key);

    text = _cJSON_PrintUnformatted(json);

    websocket_send_text(conn->websocket, text);

    tibeftlConnection_unregisterSubscription(conn, key);

    _cJSON_Delete(json);
    _cJSON_free(text);
}

typedef struct tibeftlErrorContextStruct
{
    tibeftlConnection           conn;
    tibeftlErr                  err;

} tibeftlErrorContextRec, *tibeftlErrorContext;

static void*
tibeftlConnection_errorCb(
    void*                       arg)
{
    tibeftlErrorContext         context = (tibeftlErrorContext)arg;

    websocket_close(context->conn->websocket);

    if (context->conn->errCb)
        context->conn->errCb(context->conn, context->err, context->conn->errCbArg);

    tibeftlErr_Destroy(context->err);

    free(context);

    return NULL;
}

static void
tibeftlConnection_invokeErrorCb(
    tibeftlConnection           conn,
    int                         code,
    const char*                 reason)
{
    tibeftlErrorContext         context;
    tibeftlErr                  err;
    thread_t                    thread;

    err = tibeftlErr_Create();
    tibeftlErr_Set(err, code, reason);

    context = malloc(sizeof(*context));
    context->conn = conn;
    context->err = err;

    thread_start(&thread, tibeftlConnection_errorCb, context);
    thread_detach(thread);
}

static void
tibeftlConnection_handleWelcome(
    tibeftlConnection           conn,
    _cJSON*                     json)
{
    _cJSON*                     item;
    int                         resume = 0;

    if (!conn)
        return;

    mutex_lock(&conn->mutex);

    if ((item = _cJSON_GetObjectItem(json, "client_id")) && _cJSON_IsString(item))
    {
        if (conn->clientId == NULL || strlen(conn->clientId) == 0)
        {
            if (conn->clientId)
                free(conn->clientId);
            conn->clientId = strdup(item->valuestring);
        }
    }

    if ((item = _cJSON_GetObjectItem(json, "id_token")) && _cJSON_IsString(item))
    {
        if (conn->reconnectToken == NULL || strlen(conn->reconnectToken) == 0)
        {
            if (conn->reconnectToken)
                free(conn->reconnectToken);
            conn->reconnectToken = strdup(item->valuestring);
        }
    }

    if ((item = _cJSON_GetObjectItem(json, "timeout")) && _cJSON_IsNumber(item))
        websocket_set_timeout(conn->websocket, item->valueint);

    if ((item = _cJSON_GetObjectItem(json, "max_size")) && _cJSON_IsNumber(item))
        conn->maxMsgSize = item->valueint;

    if ((item = _cJSON_GetObjectItem(json, "_resume")) && _cJSON_IsString(item))
        resume = (strcasecmp(item->valuestring, "true") == 0 ? 1 : 0);

    // repair subscriptions
    hashmap_iterate(conn->subscriptions, tibeftlSubscription_subscribe, conn);

    if (resume)
    {
        // re-send unacknowledged messages
        tibeftlConnection_resendAllRequests(conn);
    }
    else
    {
        // notify unacknowledged messages
        tibeftlConnection_notifyAllRequests(conn, TIBEFTL_ERR_PUBLISH_FAILED, "Reconnect");

        conn->lastSeqNum = 0;
    }

    mutex_unlock(&conn->mutex);

    tibeftlCompletion_notify(&conn->completion, NULL, 0, NULL);
}

static void
tibeftlConnection_handleMessage(
    tibeftlConnection           conn,
    _cJSON*                     json)
{
    tibeftlSubscription         sub;
    _cJSON*                     item;
    int64_t                     seqNum = 0;
    char*                       subId = NULL;
    _cJSON*                     body = NULL;
    char*                       text = NULL;

    if (!conn)
        return;

    if ((item = _cJSON_GetObjectItem(json, "seq")) && _cJSON_IsNumber(item))
        seqNum = (int64_t)item->valuedouble;

    if ((item = _cJSON_GetObjectItem(json, "to")) && _cJSON_IsString(item))
        subId = item->valuestring;

    if ((item = _cJSON_GetObjectItem(json, "body")) && _cJSON_IsObject(item))
        body = item;

    mutex_lock(&conn->mutex);

    if (subId && (seqNum == 0 || seqNum > conn->lastSeqNum))
    {
        sub = hashmap_get(conn->subscriptions, subId);

        if (sub && body)
        {
            if (sub->msgCb)
            {
                tibeftlMessage msg = tibeftlMessage_CreateWithJSON(body);

                if (msg)
                {
                    _cJSON_DetachItemViaPointer(json, body);

                    sub->msgCb(conn, sub, 1, &msg, sub->msgCbArg);

                    tibeftlMessage_Destroy(NULL, msg);
                }
            }
        }

        if (seqNum > 0)
            conn->lastSeqNum = seqNum;
    }

    if (seqNum > 0)
    {
        json = _cJSON_CreateObject();
        _cJSON_AddNumberToObject(json, "op", OP_ACK);
        _cJSON_AddNumberToObject(json, "seq", seqNum);

        text = _cJSON_PrintUnformatted(json);

        websocket_send_text(conn->websocket, text);

        _cJSON_Delete(json);
        _cJSON_free(text);
    }

    mutex_unlock(&conn->mutex);
}

static void
tibeftlConnection_handleSubscribed(
    tibeftlConnection           conn,
    _cJSON*                     json)
{
    _cJSON*                     item;
    const char*                 subId = NULL;

    if (!conn)
        return;

    if ((item = _cJSON_GetObjectItem(json, "id")) && _cJSON_IsString(item))
        subId = item->valuestring;

    mutex_lock(&conn->mutex);

    tibeftlConnection_notifySubscribed(conn, subId, 0, "");

    mutex_unlock(&conn->mutex);
}

static void
tibeftlConnection_handleUnsubscribed(
    tibeftlConnection           conn,
    _cJSON*                     json)
{
    _cJSON*                     item;
    const char*                 subId = NULL;
    int                         code = 0;
    char*                       reason = NULL;

    if (!conn)
        return;

    if ((item = _cJSON_GetObjectItem(json, "id")) && _cJSON_IsString(item))
        subId = item->valuestring;

    if ((item = _cJSON_GetObjectItem(json, "err")) && _cJSON_IsNumber(item))
        code = (int)item->valuedouble;

    if ((item = _cJSON_GetObjectItem(json, "reason")) && _cJSON_IsString(item))
        reason = item->valuestring;

    mutex_lock(&conn->mutex);

    tibeftlConnection_notifySubscribed(conn, subId, code, reason);

    mutex_unlock(&conn->mutex);
}

static void
tibeftlConnection_handleAck(
    tibeftlConnection           conn,
    _cJSON*                     json)
{
    _cJSON*                     item;
    int64_t                     seqNum = 0;
    int                         code = 0;
    char*                       reason = NULL;

    if (!conn)
        return;

    if ((item = _cJSON_GetObjectItem(json, "seq")) && _cJSON_IsNumber(item))
        seqNum = (int64_t)item->valuedouble;

    if ((item = _cJSON_GetObjectItem(json, "err")) && _cJSON_IsNumber(item))
        code = (int)item->valuedouble;

    if ((item = _cJSON_GetObjectItem(json, "reason")) && _cJSON_IsString(item))
        reason = item->valuestring;

    mutex_lock(&conn->mutex);

    tibeftlConnection_notifyRequest(conn, seqNum, NULL, code, reason);

    mutex_unlock(&conn->mutex);
}

static void
tibeftlConnection_handleError(
    tibeftlConnection           conn,
    _cJSON*                     json)
{
    _cJSON*                     item;
    int                         code = 0;
    char*                       reason = NULL;

    if (!conn)
        return;

    if ((item = _cJSON_GetObjectItem(json, "err")) && _cJSON_IsNumber(item))
        code = (int)item->valuedouble;

    if ((item = _cJSON_GetObjectItem(json, "reason")) && _cJSON_IsString(item))
        reason = item->valuestring;

    tibeftlConnection_invokeErrorCb(conn, code, reason);
}

static void
tibeftlConnection_handleMapResponse(
    tibeftlConnection           conn,
    _cJSON*                     json)
{
    _cJSON*                     item;
    _cJSON*                     value = NULL;
    int64_t                     seqNum = 0;
    int                         code = 0;
    char*                       reason = NULL;
    tibeftlMessage              msg = NULL;

    if (!conn)
        return;

    if ((item = _cJSON_GetObjectItem(json, "seq")) && _cJSON_IsNumber(item))
        seqNum = (int64_t)item->valuedouble;

    if ((item = _cJSON_GetObjectItem(json, "value")) && _cJSON_IsObject(item))
        value = _cJSON_DetachItemViaPointer(json, item);

    if ((item = _cJSON_GetObjectItem(json, "err")) && _cJSON_IsNumber(item))
        code = (int)item->valuedouble;

    if ((item = _cJSON_GetObjectItem(json, "reason")) && _cJSON_IsString(item))
        reason = item->valuestring;

    if (value)
        msg = tibeftlMessage_CreateWithJSON(value);

    mutex_lock(&conn->mutex);

    tibeftlConnection_notifyRequest(conn, seqNum, msg, code, reason);

    mutex_unlock(&conn->mutex);
}

static void
tibeftlConnection_onWebSocketOpen(
    websocket_t*                websocket,
    void*                       context)
{
    tibeftlConnection           conn = (tibeftlConnection)context;
    _cJSON*                     opts;
    _cJSON*                     json;
    char*                       text;

    opts = _cJSON_CreateObject();
    _cJSON_AddStringToObject(opts, "_qos", "true");
    _cJSON_AddStringToObject(opts, "_resume", "true");

    json = _cJSON_CreateObject();
    _cJSON_AddNumberToObject(json, "op", OP_LOGIN);
    _cJSON_AddStringToObject(json, "client_type", "C");
    _cJSON_AddStringToObject(json, "client_version", EFTL_VERSION_STRING_SHORT);
    _cJSON_AddItemToObject(json, "login_options", opts);

    if (conn->username && strlen(conn->username) > 0)
        _cJSON_AddStringToObject(json, "user", conn->username);
    if (conn->password && strlen(conn->password) > 0)
        _cJSON_AddStringToObject(json, "password", conn->password);
    if (conn->clientId && strlen(conn->clientId) > 0)
        _cJSON_AddStringToObject(json, "client_id", conn->clientId);
    if (conn->reconnectToken && strlen(conn->reconnectToken) > 0)
        _cJSON_AddStringToObject(json, "id_token", conn->reconnectToken);

    text = _cJSON_PrintUnformatted(json);

    websocket_send_text(conn->websocket, text);

    _cJSON_Delete(json);
    _cJSON_free(text);
}

static void
tibeftlConnection_onWebSocketText(
    websocket_t*                websocket,
    const char*                 text,
    void*                       context)
{
    tibeftlConnection           conn = (tibeftlConnection)context;
    _cJSON*                     json;
    _cJSON*                     op;

    if (!(json = _cJSON_Parse(text)))
        return;

    if ((op = _cJSON_GetObjectItem(json, "op")) && _cJSON_IsNumber(op))
    {
        switch (op->valueint)
        {
        case OP_HEARTBEAT:
            // echo heartbeat back to server
            websocket_send_text(websocket, text);
            break;
        case OP_WELCOME:
            tibeftlConnection_handleWelcome(conn, json);
            break;
        case OP_MESSAGE:
            tibeftlConnection_handleMessage(conn, json);
            break;
        case OP_SUBSCRIBED:
            tibeftlConnection_handleSubscribed(conn, json);
            break;
        case OP_UNSUBSCRIBED:
            tibeftlConnection_handleUnsubscribed(conn, json);
            break;
        case OP_ACK:
            tibeftlConnection_handleAck(conn, json);
            break;
        case OP_ERROR:
            tibeftlConnection_handleError(conn, json);
            break;
        case OP_MAP_RESPONSE:
            tibeftlConnection_handleMapResponse(conn, json);
        }
    }

    _cJSON_Delete(json);
}

static void
tibeftlConnection_cancelAutoReconnect(
    tibeftlConnection  conn)
{
    if (conn == NULL)
        return;

    mutex_lock(&conn->mutex);

    if (conn->state == STATE_AUTORECONNECTING)
    {
        if (conn->autoReconnectThread != INVALID_THREAD)
        {
            thread_t thread = conn->autoReconnectThread;
            mutex_unlock(&conn->mutex);
            tibeftlCompletion_notify(&conn->autoReconnectCompletion, NULL, -1, NULL);
            thread_join(thread);
            thread_destroy(thread);
            mutex_lock(&conn->mutex);
            conn->state = STATE_DISCONNECTED;
        }
    }

    mutex_unlock(&conn->mutex);
}

typedef struct
{
    tibeftlConnection conn;
    int64_t           backoff;
} tibeftlConnection_autoReconnectInfo;

bool
tibeftl_doConnect(
    tibeftlErr                  err,
    tibeftlConnection           conn)
{
    bool errorDetected = false;

    websocket_open(conn->websocket);

    if (tibeftlCompletion_wait(&conn->completion, conn->timeout) == TIBEFTL_ERR_TIMEOUT)
    {
        if (err != NULL)
            tibeftlErr_Set(err, TIBEFTL_ERR_TIMEOUT, "connect request timed out");
        errorDetected = true;
    }
    else if (conn->completion.code)
    {
        if (err != NULL)
            tibeftlErr_Set(err, conn->completion.code, conn->completion.reason);
        errorDetected = true;
    }

    if (errorDetected)
    {
        websocket_close(conn->websocket);
    }
    mutex_lock(&conn->mutex);
    if (!errorDetected)
    {
        conn->state = STATE_CONNECTED;
        conn->reconnectAttempts = 0;
    }
    mutex_unlock(&conn->mutex);

    return !errorDetected;
}

static void *
tibeftlConnection_autoReconnectThreadProc(
    void*             data)
{
    tibeftlConnection conn;
    int64_t backoff;
    int rc;
    tibeftlConnection_autoReconnectInfo * info = (tibeftlConnection_autoReconnectInfo *) data;

    if (info == NULL)
        return NULL;
    conn = info->conn;
    backoff = info->backoff;
    free((void *) data);
    if (conn == NULL)
        return NULL;

    rc = tibeftlCompletion_wait(&conn->autoReconnectCompletion, (int) backoff);
    mutex_lock(&conn->mutex);
    conn->autoReconnectThread = INVALID_THREAD;
    tibeftlCompletion_clear(&conn->autoReconnectCompletion);
    mutex_unlock(&conn->mutex);
    if (rc == TIBEFTL_ERR_TIMEOUT)
        tibeftl_doConnect(NULL, conn);

    return NULL;
}

static bool
tibeftlConnection_scheduleReconnect(
    tibeftlConnection  conn)
{
    bool scheduled = false;

    if (conn != NULL)
    {
        mutex_lock(&conn->mutex);
        if (conn->reconnectAttempts < conn->autoReconnectAttempts)
        {
            tibeftlConnection_autoReconnectInfo * info;
            int64_t backoff;

            conn->state = STATE_AUTORECONNECTING;
            backoff = (int64_t) (pow(2.0, (double) (conn->reconnectAttempts++)) * 1000.0);
            if (backoff > conn->autoReconnectMaxDelay)
                backoff = conn->autoReconnectMaxDelay;
            info = calloc(1, sizeof(tibeftlConnection_autoReconnectInfo));
            info->conn = conn;
            info->backoff = backoff;
            tibeftlCompletion_init(&conn->autoReconnectCompletion);
            thread_start(&conn->autoReconnectThread, tibeftlConnection_autoReconnectThreadProc, (void *) info);
            thread_detach(conn->autoReconnectThread);
            scheduled = true;
        }
        mutex_unlock(&conn->mutex);
    }
    return scheduled;
}

static void
tibeftlConnection_onWebSocketError(
    websocket_t*                websocket,
    int                         code,
    const char*                 reason,
    void*                       context)
{
    tibeftlConnection           conn = (tibeftlConnection)context;
    tibeftlConnectionState      state;

    if (!conn)
        return;

    mutex_lock(&conn->mutex);

    state = conn->state;

    conn->state = STATE_DISCONNECTED;

    mutex_unlock(&conn->mutex);

    switch (state)
    {
    case STATE_UNCONNECTED:
    case STATE_DISCONNECTED:
    case STATE_DISCONNECTING:
    default:
        break;

    case STATE_CONNECTING:
    case STATE_RECONNECTING:
        tibeftlCompletion_notify(&conn->completion, NULL, TIBEFTL_ERR_CONNECT_FAILED, reason);
        break;

    case STATE_AUTORECONNECTING:
    case STATE_CONNECTED:
        if (state == STATE_AUTORECONNECTING)
            tibeftlCompletion_notify(&conn->completion, NULL, TIBEFTL_ERR_CONNECT_FAILED, reason);
        if (!tibeftlConnection_scheduleReconnect(conn))
            tibeftlConnection_invokeErrorCb(conn, TIBEFTL_ERR_CONNECTION_LOST, reason);
        break;
    }

    mutex_lock(&conn->mutex);

    if (conn->state != STATE_AUTORECONNECTING)
    {
        tibeftlConnection_notifyAllRequests(conn, code, reason);
    }

    mutex_unlock(&conn->mutex);
}

static void
tibeftlConnection_onWebSocketClose(
    websocket_t*                websocket,
    int                         code,
    const char*                 reason,
    void*                       context)
{
    tibeftlConnection           conn = (tibeftlConnection)context;
    tibeftlConnectionState      state;

    if (!conn)
        return;

    mutex_lock(&conn->mutex);

    state = conn->state;

    conn->state = STATE_DISCONNECTED;

    mutex_unlock(&conn->mutex);

    switch (state)
    {
    case STATE_UNCONNECTED:
    case STATE_DISCONNECTED:
    case STATE_DISCONNECTING:
    case STATE_AUTORECONNECTING:
    case STATE_RECONNECTING:
    default:
        break;

    case STATE_CONNECTING:
        tibeftlCompletion_notify(&conn->completion, NULL, code, reason);
        break;

    case STATE_CONNECTED:
        tibeftlConnection_invokeErrorCb(conn, code, reason);
        break;
    }

    mutex_lock(&conn->mutex);

    tibeftlConnection_notifyAllRequests(conn, code, reason);

    mutex_unlock(&conn->mutex);
}

static tibeftlConnection
tibeftlConnection_create(
    const char*                 url,
    tibeftlOptions*             opts,
    tibeftlErrorCallback        errCb,
    void*                       errCbArg)
{
    tibeftlConnection           conn;
    websocket_options_t         websocketOptions;

    conn = calloc(1, sizeof(*conn));

    conn->state = STATE_UNCONNECTED;
    conn->url = url_parse(url);
    conn->ref = 1;

    if (url_username(conn->url))
        conn->username = strdup(url_username(conn->url));
    if (url_password(conn->url))
        conn->password = strdup(url_password(conn->url));
    if (url_query(conn->url, "clientId"))
        conn->clientId = strdup(url_query(conn->url, "clientId"));
    if (url_query(conn->url, "trustStore"))
        conn->trustStore = strdup(url_query(conn->url, "trustStore"));

    conn->timeout = TIBEFTL_TIMEOUT;

    if (opts && opts->ver == 0)
    {
        if (opts->username && !conn->username)
            conn->username = strdup(opts->username);
        if (opts->password && !conn->password)
            conn->password = strdup(opts->password);
        if (opts->clientId && !conn->clientId)
            conn->clientId = strdup(opts->clientId);
        if (opts->trustStore && !conn->trustStore)
            conn->trustStore = strdup(opts->trustStore);

        if (opts->timeout > 0)
            conn->timeout = opts->timeout;
        if (opts->autoReconnectAttempts > 0)
            conn->autoReconnectAttempts = opts->autoReconnectAttempts;
        if (opts->autoReconnectMaxDelay > 0)
            conn->autoReconnectMaxDelay = opts->autoReconnectMaxDelay;
    }

    conn->errCb = errCb;
    conn->errCbArg = errCbArg;

    mutex_init(&conn->mutex);

    websocket_options_init(websocketOptions);

    websocketOptions.protocol = TIBEFTL_PROTOCOL;
    websocketOptions.trustStore = conn->trustStore;
    websocketOptions.timeout = conn->timeout;
    websocketOptions.open_cb = tibeftlConnection_onWebSocketOpen;
    websocketOptions.text_cb = tibeftlConnection_onWebSocketText;
    websocketOptions.error_cb = tibeftlConnection_onWebSocketError;
    websocketOptions.close_cb = tibeftlConnection_onWebSocketClose;
    websocketOptions.context = conn;

    conn->websocket = websocket_create(url, &websocketOptions);
    conn->subscriptions = hashmap_create(128);
    tibeftlCompletion_init(&conn->completion);

    return conn;
}

static void
tibeftlConnection_destroy(
    tibeftlConnection           conn)
{
    if (!conn)
        return;

    hashmap_iterate(conn->subscriptions, tibeftlSubscription_cleanup, NULL);

    hashmap_destroy(conn->subscriptions);

    websocket_destroy(conn->websocket);

    tibeftlCompletion_clear(&conn->completion);

    mutex_destroy(&conn->mutex);

    if (conn->url)
        url_destroy(conn->url);
    if (conn->username)
        free(conn->username);
    if (conn->password)
        free(conn->password);
    if (conn->clientId)
        free(conn->clientId);
    if (conn->reconnectToken)
        free(conn->reconnectToken);

    free(conn);
}

static void
tibeftlConnection_retain(
    tibeftlConnection           conn)
{
    if (!conn)
        return;

    mutex_lock(&conn->mutex);

    conn->ref++;

    mutex_unlock(&conn->mutex);
}

static void
tibeftlConnection_release(
    tibeftlConnection           conn)
{
    int                         ref;

    if (!conn)
        return;

    mutex_lock(&conn->mutex);

    ref = conn->ref--;

    mutex_unlock(&conn->mutex);

    if (ref == 1)
        tibeftlConnection_destroy(conn);
}

const char*
tibeftl_Version(void)
{
    return EFTL_VERSION_STRING_LONG;
}

tibeftlConnection
tibeftl_Connect(
    tibeftlErr                  err,
    const char*                 url,
    tibeftlOptions*             opts,
    tibeftlErrorCallback        errCb,
    void*                       errCbArg)
{
    tibeftlConnection           conn;

    if (tibeftlErr_IsSet(err))
        return NULL;

    conn = tibeftlConnection_create(url, opts, errCb, errCbArg);

    conn->state = STATE_CONNECTING;

    if (!tibeftl_doConnect(err, conn))
    {
        tibeftlConnection_destroy(conn);
        conn = NULL;
    }
    return conn;
}

void
tibeftl_Disconnect(
    tibeftlErr                  err,
    tibeftlConnection           conn)
{
    _cJSON*                     json;
    char*                       text;

    if (!conn)
        return;

    mutex_lock(&conn->mutex);

    switch (conn->state)
    {
    case STATE_UNCONNECTED:
    case STATE_DISCONNECTED:
    case STATE_CONNECTING:
    case STATE_DISCONNECTING:
    default:
        break;

    case STATE_CONNECTED:
    case STATE_RECONNECTING:
        conn->state = STATE_DISCONNECTING;

        mutex_unlock(&conn->mutex);

        json = _cJSON_CreateObject();
        _cJSON_AddNumberToObject(json, "op", OP_DISCONNECT);

        text = _cJSON_PrintUnformatted(json);

        websocket_send_text(conn->websocket, text);

        websocket_close(conn->websocket);

        _cJSON_Delete(json);
        _cJSON_free(text);

        mutex_lock(&conn->mutex);

        conn->state = STATE_DISCONNECTED;
        break;

    case STATE_AUTORECONNECTING:
        conn->state = STATE_DISCONNECTING;
        mutex_unlock(&conn->mutex);
        tibeftlConnection_cancelAutoReconnect(conn);
        mutex_lock(&conn->mutex);
        break;
    }

    mutex_unlock(&conn->mutex);

    tibeftlConnection_release(conn);
}

void
tibeftl_Reconnect(
    tibeftlErr                  err,
    tibeftlConnection           conn)
{
    bool rc;

    if (tibeftlErr_IsSet(err))
        return;

    if (!conn)
    {
        tibeftlErr_Set(err, TIBEFTL_ERR_INVALID_ARG, "connection is NULL");
        return;
    }

    mutex_lock(&conn->mutex);

    switch (conn->state)
    {
    case STATE_UNCONNECTED:
    case STATE_CONNECTING:
    case STATE_CONNECTED:
    case STATE_DISCONNECTING:
    case STATE_RECONNECTING:
    default:
        break;

    case STATE_AUTORECONNECTING:
    case STATE_DISCONNECTED:
        if (conn->state == STATE_AUTORECONNECTING)
        {
            mutex_unlock(&conn->mutex);

            tibeftlConnection_cancelAutoReconnect(conn);

            mutex_lock(&conn->mutex);
        }

        conn->state = STATE_RECONNECTING;

        mutex_unlock(&conn->mutex);

        rc = tibeftl_doConnect(err, conn);

        mutex_lock(&conn->mutex);
        if (!rc)
            conn->state = STATE_DISCONNECTED;
        break;
    }

    mutex_unlock(&conn->mutex);
}

bool
tibeftl_IsConnected(
    tibeftlErr                  err,
    tibeftlConnection           conn)
{
    tibeftlConnectionState      state;

    if (!conn)
        return false;

    mutex_lock(&conn->mutex);

    state = conn->state;

    mutex_unlock(&conn->mutex);

    return (state == STATE_CONNECTED || state == STATE_AUTORECONNECTING ? true : false);
}

void
tibeftl_Publish(
    tibeftlErr                  err,
    tibeftlConnection           conn,
    tibeftlMessage              msg)
{
    tibeftlCompletion           completion = NULL;
    _cJSON*                     json;
    int64_t                     seqNum = 0;
    char*                       text;

    if (tibeftlErr_IsSet(err))
        return;

    if (!conn)
    {
        tibeftlErr_Set(err, TIBEFTL_ERR_INVALID_ARG, "conn is NULL");
        return;
    }

    if (!msg)
    {
        tibeftlErr_Set(err, TIBEFTL_ERR_INVALID_ARG, "msg is NULL");
        return;
    }

    mutex_lock(&conn->mutex);

    if (conn->state == STATE_CONNECTED || conn->state == STATE_AUTORECONNECTING)
    {
        seqNum = ++conn->pubSeqNum;

        json = _cJSON_CreateObject();
        _cJSON_AddNumberToObject(json, "op", OP_PUBLISH);
        _cJSON_AddNumberToObject(json, "seq", seqNum);
        _cJSON_AddItemReferenceToObject(json, "body", tibeftlMessage_GetJSON(msg));

        text = _cJSON_PrintUnformatted(json);

        if (conn->maxMsgSize > 0 && strlen(text) > conn->maxMsgSize)
        {
            tibeftlErr_Set(err, TIBEFTL_ERR_INVALID_ARG, "maximum message size exceeded");
        }
        else
        {
            completion = tibeftlConnection_registerRequest(conn, seqNum, text);

            websocket_send_text(conn->websocket, text);
        }

        _cJSON_Delete(json);
    }
    else
    {
        tibeftlErr_Set(err, TIBEFTL_ERR_NOT_CONNECTED, "not connected");
    }

    mutex_unlock(&conn->mutex);

    if (completion)
    {
        if (tibeftlCompletion_wait(completion, WAIT_FOREVER) == TIBEFTL_ERR_TIMEOUT)
        {
            tibeftlErr_Set(err, TIBEFTL_ERR_TIMEOUT, "publish request timed out");
        }
        else if (completion->code)
        {
            tibeftlErr_Set(err, completion->code, completion->reason);
        }

        mutex_lock(&conn->mutex);

        tibeftlConnection_unregisterRequest(conn, seqNum);

        mutex_unlock(&conn->mutex);
    }
}

tibeftlSubscription
tibeftl_Subscribe(
    tibeftlErr                  err,
    tibeftlConnection           conn,
    const char*                 matcher,
    const char*                 durable,
    tibeftlMessageCallback      msgCb,
    void*                       msgCbArg)
{
    return tibeftl_SubscribeWithOptions(err, conn, matcher, durable, NULL, msgCb, msgCbArg);
}

tibeftlSubscription
tibeftl_SubscribeWithOptions(
    tibeftlErr                  err,
    tibeftlConnection           conn,
    const char*                 matcher,
    const char*                 durable,
    tibeftlSubscriptionOptions* opts,
    tibeftlMessageCallback      msgCb,
    void*                       msgCbArg)
{
    tibeftlSubscription         sub = NULL;
    tibeftlCompletion           completion = NULL;
    _cJSON*                     json;
    char*                       text;
    int64_t                     subId;

    if (tibeftlErr_IsSet(err))
        return NULL;

    if (!conn)
    {
        tibeftlErr_Set(err, TIBEFTL_ERR_INVALID_ARG, "conn is NULL");
        return NULL;
    }

    mutex_lock(&conn->mutex);

    if (conn->state == STATE_CONNECTED || conn->state == STATE_AUTORECONNECTING)
    {
        subId = ++conn->lastSubId;

        sub = tibeftlSubscription_create(subId, matcher, durable, opts, msgCb, msgCbArg);

        completion = tibeftlConnection_registerSubscription(conn, sub);

        json = _cJSON_CreateObject();
        _cJSON_AddNumberToObject(json, "op", OP_SUBSCRIBE);
        _cJSON_AddStringToObject(json, "id", sub->subId);
        if (sub->matcher)
            _cJSON_AddStringToObject(json, "matcher", sub->matcher);
        if (sub->durable)
            _cJSON_AddStringToObject(json, "durable", sub->durable);
        if (sub->durableType)
            _cJSON_AddStringToObject(json, "type", sub->durableType);
        if (sub->durableKey)
            _cJSON_AddStringToObject(json, "key", sub->durableKey);

        text = _cJSON_PrintUnformatted(json);

        websocket_send_text(conn->websocket, text);

        _cJSON_Delete(json);
        _cJSON_free(text);
    }
    else
    {
        tibeftlErr_Set(err, TIBEFTL_ERR_NOT_CONNECTED, "not connected");
    }

    mutex_unlock(&conn->mutex);

    if (completion)
    {
        if (tibeftlCompletion_wait(completion, WAIT_FOREVER) == TIBEFTL_ERR_TIMEOUT)
        {
            tibeftlErr_Set(err, TIBEFTL_ERR_TIMEOUT, "subscription request timed out");
        }
        else if (completion->code)
        {
            tibeftlErr_Set(err, completion->code, completion->reason);
        }
    }

    return sub;
}

void
tibeftl_Unsubscribe(
    tibeftlErr                  err,
    tibeftlConnection           conn,
    tibeftlSubscription         sub)
{
    if (!conn)
    {
        tibeftlErr_Set(err, TIBEFTL_ERR_INVALID_ARG, "connection is NULL");
        return;
    }

    if (!sub)
    {
        tibeftlErr_Set(err, TIBEFTL_ERR_INVALID_ARG, "subscription is NULL");
        return;
    }

    mutex_lock(&conn->mutex);

    if (conn->state == STATE_CONNECTED || conn->state == STATE_AUTORECONNECTING)
    {
        tibeftlSubscription_unsubscribe((const char*)sub->subId, (void*)sub, (void*)conn);
    }
    else
    {
        tibeftlErr_Set(err, TIBEFTL_ERR_NOT_CONNECTED, "not connected");
    }

    mutex_unlock(&conn->mutex);
}

void
tibeftl_UnsubscribeAll(
    tibeftlErr                  err,
    tibeftlConnection           conn)
{
    if (!conn)
    {
        tibeftlErr_Set(err, TIBEFTL_ERR_INVALID_ARG, "connection is NULL");
        return;
    }

    mutex_lock(&conn->mutex);

    if (conn->state == STATE_CONNECTED || conn->state == STATE_AUTORECONNECTING)
    {
        hashmap_iterate(conn->subscriptions, tibeftlSubscription_unsubscribe, (void*)conn);
    }
    else
    {
        tibeftlErr_Set(err, TIBEFTL_ERR_NOT_CONNECTED, "not connected");
    }

    mutex_unlock(&conn->mutex);
}

tibeftlKVMap
tibeftl_CreateKVMap(
    tibeftlErr                  err,
    tibeftlConnection           conn,
    const char*                 name)
{
    tibeftlKVMap                map;

    if (tibeftlErr_IsSet(err))
        return NULL;

    if (!conn)
    {
        tibeftlErr_Set(err, TIBEFTL_ERR_INVALID_ARG, "conn is NULL");
        return NULL;
    }

    if (!name)
    {
        tibeftlErr_Set(err, TIBEFTL_ERR_INVALID_ARG, "name is NULL");
        return NULL;
    }

    map = calloc(1, sizeof(struct tibeftlKVMapStruct));

    if (!map)
    {
        tibeftlErr_Set(err, TIBEFTL_ERR_NO_MEMORY, "no memory");
    }
    else
    {
        tibeftlConnection_retain(conn);

        map->name = strdup(name);
        map->conn = conn;
    }

    return map;
}

void
tibeftlKVMap_Destroy(
    tibeftlErr                  err,
    tibeftlKVMap                map)
{
    if (!map)
        return;

    tibeftlConnection_release(map->conn);

    free(map->name);
    free(map);
}

void
tibeftlKVMap_Set(
    tibeftlErr                  err,
    tibeftlKVMap                map,
    const char*                 key,
    tibeftlMessage              msg)
{
    tibeftlCompletion           completion = NULL;
    _cJSON*                     json;
    int64_t                     seqNum = 0;
    char*                       text;

    if (tibeftlErr_IsSet(err))
        return;

    if (!map)
    {
        tibeftlErr_Set(err, TIBEFTL_ERR_INVALID_ARG, "map is NULL");
        return;
    }

    if (!key)
    {
        tibeftlErr_Set(err, TIBEFTL_ERR_INVALID_ARG, "key is NULL");
        return;
    }

    if (!msg)
    {
        tibeftlErr_Set(err, TIBEFTL_ERR_INVALID_ARG, "msg is NULL");
        return;
    }

    mutex_lock(&map->conn->mutex);

    if (map->conn->state == STATE_CONNECTED || map->conn->state == STATE_AUTORECONNECTING)
    {
        seqNum = ++map->conn->pubSeqNum;

        json = _cJSON_CreateObject();
        _cJSON_AddNumberToObject(json, "op", OP_MAP_SET);
        _cJSON_AddNumberToObject(json, "seq", seqNum);
        _cJSON_AddStringToObject(json, "map", map->name);
        _cJSON_AddStringToObject(json, "key", key);
        _cJSON_AddItemReferenceToObject(json, "value", tibeftlMessage_GetJSON(msg));

        text = _cJSON_PrintUnformatted(json);

        if (map->conn->maxMsgSize > 0 && strlen(text) > map->conn->maxMsgSize)
        {
            tibeftlErr_Set(err, TIBEFTL_ERR_INVALID_ARG, "maximum message size exceeded");
        }
        else
        {
            completion = tibeftlConnection_registerRequest(map->conn, seqNum, text);

            websocket_send_text(map->conn->websocket, text);
        }

        _cJSON_Delete(json);
    }
    else
    {
        tibeftlErr_Set(err, TIBEFTL_ERR_NOT_CONNECTED, "not connected");
    }

    mutex_unlock(&map->conn->mutex);

    if (completion)
    {
        if (tibeftlCompletion_wait(completion, WAIT_FOREVER) == TIBEFTL_ERR_TIMEOUT)
        {
            tibeftlErr_Set(err, TIBEFTL_ERR_TIMEOUT, "key-value map request timed out");
        }
        else if (completion->code)
        {
            tibeftlErr_Set(err, completion->code, completion->reason);
        }

        mutex_lock(&map->conn->mutex);

        tibeftlConnection_unregisterRequest(map->conn, seqNum);

        mutex_unlock(&map->conn->mutex);
    }
}

tibeftlMessage
tibeftlKVMap_Get(
    tibeftlErr                  err,
    tibeftlKVMap                map,
    const char*                 key)
{
    tibeftlCompletion           completion = NULL;
    tibeftlMessage              msg = NULL;
    _cJSON*                     json;
    int64_t                     seqNum = 0;
    char*                       text;

    if (tibeftlErr_IsSet(err))
        return NULL;

    if (!map)
    {
        tibeftlErr_Set(err, TIBEFTL_ERR_INVALID_ARG, "map is NULL");
        return NULL;
    }

    if (!key)
    {
        tibeftlErr_Set(err, TIBEFTL_ERR_INVALID_ARG, "key is NULL");
        return NULL;
    }

    mutex_lock(&map->conn->mutex);

    if (map->conn->state == STATE_CONNECTED || map->conn->state == STATE_AUTORECONNECTING)
    {
        seqNum = ++map->conn->pubSeqNum;

        json = _cJSON_CreateObject();
        _cJSON_AddNumberToObject(json, "op", OP_MAP_GET);
        _cJSON_AddNumberToObject(json, "seq", seqNum);
        _cJSON_AddStringToObject(json, "map", map->name);
        _cJSON_AddStringToObject(json, "key", key);

        text = _cJSON_PrintUnformatted(json);

        completion = tibeftlConnection_registerRequest(map->conn, seqNum, text);

        websocket_send_text(map->conn->websocket, text);

        _cJSON_Delete(json);
    }
    else
    {
        tibeftlErr_Set(err, TIBEFTL_ERR_NOT_CONNECTED, "not connected");
    }

    mutex_unlock(&map->conn->mutex);

    if (completion)
    {
        if (tibeftlCompletion_wait(completion, WAIT_FOREVER) == TIBEFTL_ERR_TIMEOUT)
        {
            tibeftlErr_Set(err, TIBEFTL_ERR_TIMEOUT, "key-value map request timed out");
        }
        else if (completion->code)
        {
            tibeftlErr_Set(err, completion->code, completion->reason);
        }

        msg = completion->response;

        mutex_lock(&map->conn->mutex);

        tibeftlConnection_unregisterRequest(map->conn, seqNum);

        mutex_unlock(&map->conn->mutex);
    }

    return msg;
}

void
tibeftlKVMap_Remove(
    tibeftlErr                  err,
    tibeftlKVMap                map,
    const char*                 key)
{
    tibeftlCompletion           completion = NULL;
    _cJSON*                     json;
    int64_t                     seqNum = 0;
    char*                       text;

    if (tibeftlErr_IsSet(err))
        return;

    if (!map)
    {
        tibeftlErr_Set(err, TIBEFTL_ERR_INVALID_ARG, "map is NULL");
        return;
    }

    if (!key)
    {
        tibeftlErr_Set(err, TIBEFTL_ERR_INVALID_ARG, "key is NULL");
        return;
    }

    mutex_lock(&map->conn->mutex);

    if (map->conn->state == STATE_CONNECTED || map->conn->state == STATE_AUTORECONNECTING)
    {
        seqNum = ++map->conn->pubSeqNum;

        json = _cJSON_CreateObject();
        _cJSON_AddNumberToObject(json, "op", OP_MAP_REMOVE);
        _cJSON_AddNumberToObject(json, "seq", seqNum);
        _cJSON_AddStringToObject(json, "map", map->name);
        _cJSON_AddStringToObject(json, "key", key);

        text = _cJSON_PrintUnformatted(json);

        completion = tibeftlConnection_registerRequest(map->conn, seqNum, text);

        websocket_send_text(map->conn->websocket, text);

        _cJSON_Delete(json);
    }
    else
    {
        tibeftlErr_Set(err, TIBEFTL_ERR_NOT_CONNECTED, "not connected");
    }

    mutex_unlock(&map->conn->mutex);

    if (completion)
    {
        if (tibeftlCompletion_wait(completion, WAIT_FOREVER) == TIBEFTL_ERR_TIMEOUT)
        {
            tibeftlErr_Set(err, TIBEFTL_ERR_TIMEOUT, "key-value map request timed out");
        }
        else if (completion->code)
        {
            tibeftlErr_Set(err, completion->code, completion->reason);
        }

        mutex_lock(&map->conn->mutex);

        tibeftlConnection_unregisterRequest(map->conn, seqNum);

        mutex_unlock(&map->conn->mutex);
    }
}

tibeftlErr
tibeftlErr_Create(void)
{
    tibeftlErr                  err;

    err = calloc(1, sizeof(*err));

    return err;
}

void
tibeftlErr_Destroy(
    tibeftlErr                  err)
{
    if (!err)
        return;

    if (err->desc)
    {
        free(err->desc);
    }

    free(err);
}

bool
tibeftlErr_IsSet(
    tibeftlErr                  err)
{
    if (!err)
        return false;

    return (err->code ? true : false);
}

int
tibeftlErr_GetCode(
    tibeftlErr                  err)
{
    if (!err)
        return 0;

    return err->code;
}

const char*
tibeftlErr_GetDescription(
    tibeftlErr                  err)
{
    if (!err)
        return NULL;

    return err->desc;
}

void
tibeftlErr_Set(
    tibeftlErr                  err,
    int                         code,
    const char*                 desc)
{
    if (!err || err->code)
        return;

    err->code = code;

    if (desc)
    {
        err->desc = strdup(desc);
    }
}

void
tibeftlErr_Clear(
    tibeftlErr                  err)
{
    if (!err)
        return;

    err->code = 0;

    if (err->desc)
    {
        free(err->desc);

        err->desc = NULL;
    }
}
