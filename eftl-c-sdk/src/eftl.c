/*
 * Copyright (c) 2001-2021 TIBCO Software Inc.
 * Licensed under a BSD-style license. Refer to [LICENSE]
 * For more information, please contact:
 * TIBCO Software Inc., Palo Alto, California, USA
 *
 * $Id: eftl.c 131938 2021-02-18 17:31:00Z $
 *
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include <inttypes.h>
#include <time.h>
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
#include <sys/timeb.h>
#define strcasecmp _stricmp
#else
#include <sys/signal.h>
#endif

#define TIBEFTL_PROTOCOL        "v1.eftl.tibco.com"
#define TIBEFTL_PROTOCOL_VER    (1)
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
#define OP_REQUEST              (13)
#define OP_REQUEST_REPLY        (14)
#define OP_REPLY                (15)
#define OP_MAP_CREATE           (16)
#define OP_MAP_DESTROY          (18)
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
    bool                        notified;

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

typedef struct tibeftlErrorContextStruct
{
    tibeftlConnection           conn;
    tibeftlErr                  err;

} tibeftlErrorContextRec, *tibeftlErrorContext; 

struct tibeftlConnectionStruct
{
    int                         ref;
    url_t**                     urlList;
    int                         urlIndex;
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
    bool                        trustAll;
    char*                       reconnectToken;
    thread_t                    reconnectThread;
    int                         protocol;
    int                         maxMsgSize;
    int                         maxPendingAcks;
    int64_t                     timeout;
    int64_t                     pubSeqNum;
    int64_t                     lastSubId;
    void*                       onConnectArg;
    int64_t                     autoReconnectAttempts;
    int64_t                     autoReconnectMaxDelay;
    int64_t                     reconnectAttempts;
    tibeftlCompletionRec        reconnectCompletion;
    tibeftlStateCallback        stateChangeCallback;
    void*                       stateChangeCallbackArg;
    thread_t                    dispatchThread;
    tibeftlMessageList          msgList;
};

struct tibeftlSubscriptionStruct
{
    char*                       subId;
    char*                       matcher;
    char*                       durable;
    char*                       durableType;
    char*                       durableKey;
    int64_t                     lastSeqNum;
    tibeftlAcknowledgeMode      ackMode;
    tibeftlMessageCallback      msgCb;
    void*                       msgCbArg;
    tibeftlCompletionRec        completion;
    bool                        pending;
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

static int64_t
tibeftl_now(void)
{
#if defined(_WIN32)
    struct _timeb tb;
    _ftime_s(&tb);
    return (((int64_t)tb.time) * 1000 + tb.millitm);
#elif defined(CLOCK_MONOTONIC)
    struct timespec ts;
    clock_gettime(CLOCK_REALTIME, &ts);
    return ((int64_t)ts.tv_sec) * 1000 + (((int64_t)ts.tv_nsec) / 1000000);
#else
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return ((int64_t)tv.tv_sec) * 1000 + (((int64_t)tv.tv_usec) / 1000);
#endif
}

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

    sub->ackMode = TIBEFTL_ACKNOWLEDGE_MODE_AUTO;

    size = snprintf(NULL, 0, "%" PRId64, subId) + 1;
    sub->subId = malloc(size);
    snprintf(sub->subId, size, "%" PRId64, subId);

    if (matcher)
        sub->matcher = strdup(matcher);
    if (durable)
        sub->durable = strdup(durable);
    if (opts && opts->durableType)
        sub->durableType = strdup(opts->durableType);
    if (opts && opts->durableKey)
        sub->durableKey = strdup(opts->durableKey);
    if (opts)
        sub->ackMode = opts->acknowledgeMode;
    
    sub->msgCb = msgCb;
    sub->msgCbArg = msgCbArg;

    sub->pending = true;

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
tibeftlCompletion_reset(
    tibeftlCompletion           completion)
{
    completion->notified = false;
    completion->code = 0;

    if (completion->response)
    {
        tibeftlMessage_Destroy(NULL, completion->response);
        completion->response = NULL;
    }

    if (completion->reason)
    {
        free(completion->reason);
        completion->reason = NULL;
    }
}

static void
tibeftlCompletion_clear(
    tibeftlCompletion           completion)
{
    if (completion->response)
    {
        tibeftlMessage_Destroy(NULL, completion->response);
        completion->response = NULL;
    }

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

static bool
tibeftlCompletion_notify(
    tibeftlCompletion           completion,
    tibeftlMessage              response,
    int                         code,
    const char*                 reason)
{
    if (!completion || completion->notified)
        return false;

    completion->notified = true;

    if (completion->response)
    {
        tibeftlMessage_Destroy(NULL, completion->response);
        completion->reason = NULL;
    }

    if (completion->reason)
    {
        free(completion->reason);
        completion->reason = NULL;
    }

    completion->response = response;
    completion->code = code;

    if (reason)
        completion->reason = strdup(reason);

    if (completion->semaphore)
        semaphore_post(completion->semaphore);

    return true;
}

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
    if (context)
    {
        context->conn = conn;
        context->err = err;
    }

    thread_start(&thread, tibeftlConnection_errorCb, (void*)context);
    thread_detach(thread);
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

    // notify the completion only if the subscription is 
    // pending, otherwise, invoke the error callback
    if (sub && sub->pending)
    {
        sub->pending = false;
        tibeftlCompletion_notify(&sub->completion, NULL, code, reason);
    }
    else if (sub && code)
    {
        // subscription failed following a reconnect
        tibeftlConnection_invokeErrorCb(conn, code, reason);
    }
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

static bool
tibeftlConnection_notifyRequest(
    tibeftlConnection           conn,
    int64_t                     seqNum,
    tibeftlMessage              response,
    int                         code,
    const char*                 reason)
{
    tibeftlRequest              request;
    bool                        notified = false;

    if (!conn)
        return false;

    for (request = conn->requestList.head; request; request = request->next)
    {
        if (request->seqNum == seqNum)
            notified = tibeftlCompletion_notify(&request->completion, response, code, reason);
    }

    return notified;
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

static const char*
tibeftlAcknowledgeMode_toString(
    tibeftlAcknowledgeMode      ackMode)
{
    switch (ackMode)
    {
    case TIBEFTL_ACKNOWLEDGE_MODE_AUTO:
        return "auto";
    case TIBEFTL_ACKNOWLEDGE_MODE_CLIENT:
        return "client";
    case TIBEFTL_ACKNOWLEDGE_MODE_NONE:
        return "none";
    default:
        return "unknown";
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
    _cJSON_AddStringToObject(json, "ack", tibeftlAcknowledgeMode_toString(sub->ackMode));
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
tibeftlSubscription_reset(
    const char*                 key,
    void*                       value,
    void*                       context)
{
    tibeftlSubscription         sub = (tibeftlSubscription)value;

    sub->lastSeqNum = 0;
}

static void
tibeftlSubscription_close(
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
    _cJSON_AddFalseToObject(json, "del");

    text = _cJSON_PrintUnformatted(json);

    websocket_send_text(conn->websocket, text);

    tibeftlConnection_unregisterSubscription(conn, key);

    _cJSON_Delete(json);
    _cJSON_free(text);
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

static void
tibeftlConnection_setState(
    tibeftlConnection           conn,
    tibeftlConnectionState      state)
{
    if (!conn)
        return;

    if (conn->state != state)
    {
        conn->state = state;

        if (conn->stateChangeCallback)
            conn->stateChangeCallback(conn, conn->state, conn->stateChangeCallbackArg);
    }
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

    tibeftlConnection_setState(conn, STATE_CONNECTED);

    // reset reconnect logic
    conn->reconnectAttempts = 0;
    conn->urlIndex = 0;

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

    if ((item = _cJSON_GetObjectItem(json, "protocol")) && _cJSON_IsNumber(item))
        conn->protocol = item->valueint;

    if ((item = _cJSON_GetObjectItem(json, "max_size")) && _cJSON_IsNumber(item))
        conn->maxMsgSize = item->valueint;

    if ((item = _cJSON_GetObjectItem(json, "_resume")) && _cJSON_IsString(item))
        resume = (strcasecmp(item->valuestring, "true") == 0 ? 1 : 0);

    if (!resume)
    {
        // reset subscription last sequence number
        hashmap_iterate(conn->subscriptions, tibeftlSubscription_reset, conn);
    }

    // repair subscriptions
    hashmap_iterate(conn->subscriptions, tibeftlSubscription_subscribe, conn);

    // re-send unacknowledged messages
    tibeftlConnection_resendAllRequests(conn);

    mutex_unlock(&conn->mutex);

    tibeftlCompletion_notify(&conn->completion, NULL, 0, NULL);
}

static void
tibeftlConnection_acknowledge(
    tibeftlConnection           conn,
    long                        seqNum,
    const char*                 subId)
{
    _cJSON*                     json;
    char*                       text;

    if (conn && seqNum > 0)
    {
        json = _cJSON_CreateObject();
        _cJSON_AddNumberToObject(json, "op", OP_ACK);
        _cJSON_AddNumberToObject(json, "seq", seqNum);

        if (subId)
            _cJSON_AddStringToObject(json, "id", subId);

        text = _cJSON_PrintUnformatted(json);

        websocket_send_text(conn->websocket, text);

        _cJSON_Delete(json);
        _cJSON_free(text);
    }
}

static void
tibeftlConnection_handleMessage(
    tibeftlConnection           conn,
    _cJSON*                     json)
{
    tibeftlMessage              msg;
    _cJSON*                     item;
    int64_t                     seqNum = 0;
    char*                       subId = NULL;
    char*                       replyTo = NULL;
    int64_t                     reqId = 0;
    int64_t                     msgId = 0;
    int64_t                     deliveryCount = 0;
    _cJSON*                     body = NULL;

    if (!conn)
        return;

    if ((item = _cJSON_GetObjectItem(json, "seq")) && _cJSON_IsNumber(item))
        seqNum = (int64_t)item->valuedouble;

    if ((item = _cJSON_GetObjectItem(json, "to")) && _cJSON_IsString(item))
        subId = item->valuestring;

    if ((item = _cJSON_GetObjectItem(json, "reply_to")) && _cJSON_IsString(item))
        replyTo = item->valuestring;

    if ((item = _cJSON_GetObjectItem(json, "req")) && _cJSON_IsNumber(item))
        reqId = (int64_t)item->valuedouble;

    if ((item = _cJSON_GetObjectItem(json, "sid")) && _cJSON_IsNumber(item))
        msgId = (int64_t)item->valuedouble;

    if ((item = _cJSON_GetObjectItem(json, "cnt")) && _cJSON_IsNumber(item))
        deliveryCount = (int64_t)item->valuedouble;

    if ((item = _cJSON_GetObjectItem(json, "body")) && _cJSON_IsObject(item))
        body = item;

    if (!subId || !body)
        return;

    msg = tibeftlMessage_CreateWithJSON(body);
    if (msg)
    {
        _cJSON_DetachItemViaPointer(json, body);

        tibeftlMessage_SetReceipt(msg, seqNum, subId);
        tibeftlMessage_SetReplyTo(msg, replyTo, reqId);
        tibeftlMessage_SetStoreMessageId(msg, msgId);
        tibeftlMessage_SetDeliveryCount(msg, deliveryCount);

        if (!tibeftlMessageList_Add(conn->msgList, msg))
            tibeftlMessage_Destroy(NULL, msg);
    }
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
tibeftlConnection_handleReply(
    tibeftlConnection           conn,
    _cJSON*                     json)
{
    _cJSON*                     item;
    _cJSON*                     body = NULL;
    int64_t                     seqNum = 0;
    int                         code = 0;
    char*                       reason = NULL;
    tibeftlMessage              msg = NULL;
    bool                        notified;

    if (!conn)
        return;

    if ((item = _cJSON_GetObjectItem(json, "seq")) && _cJSON_IsNumber(item))
        seqNum = (int64_t)item->valuedouble;

    if ((item = _cJSON_GetObjectItem(json, "body")) && _cJSON_IsObject(item))
        body = _cJSON_DetachItemViaPointer(json, item);

    if ((item = _cJSON_GetObjectItem(json, "err")) && _cJSON_IsNumber(item))
        code = (int)item->valuedouble;

    if ((item = _cJSON_GetObjectItem(json, "reason")) && _cJSON_IsString(item))
        reason = item->valuestring;

    if (body)
        msg = tibeftlMessage_CreateWithJSON(body);

    mutex_lock(&conn->mutex);

    notified = tibeftlConnection_notifyRequest(conn, seqNum, msg, code, reason);

    mutex_unlock(&conn->mutex);

    if (!notified)
        tibeftlMessage_Destroy(NULL, msg);
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
    bool                        notified;

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

    notified = tibeftlConnection_notifyRequest(conn, seqNum, msg, code, reason);

    mutex_unlock(&conn->mutex);

    if (!notified)
        tibeftlMessage_Destroy(NULL, msg);
}

static void
tibeftlConnection_onWebSocketOpen(
    websocket_t*                websocket,
    void*                       context)
{
    tibeftlConnection           conn = (tibeftlConnection)context;
    url_t*                      url;
    _cJSON*                     opts;
    _cJSON*                     json;
    char*                       text;

    url = websocket_url(websocket);

    opts = _cJSON_CreateObject();
    _cJSON_AddStringToObject(opts, "_qos", "true");
    _cJSON_AddStringToObject(opts, "_resume", "true");

    json = _cJSON_CreateObject();
    _cJSON_AddNumberToObject(json, "op", OP_LOGIN);
    _cJSON_AddNumberToObject(json, "protocol", TIBEFTL_PROTOCOL_VER);
    _cJSON_AddStringToObject(json, "client_type", "C");
    _cJSON_AddStringToObject(json, "client_version", EFTL_VERSION_STRING_SHORT);
    _cJSON_AddItemToObject(json, "login_options", opts);

    if (url && url_username(url))
        _cJSON_AddStringToObject(json, "user", url_username(url));
    else if (conn->username && strlen(conn->username) > 0)
        _cJSON_AddStringToObject(json, "user", conn->username);
    if (url && url_password(url))
        _cJSON_AddStringToObject(json, "password", url_password(url));
    else if (conn->password && strlen(conn->password) > 0)
        _cJSON_AddStringToObject(json, "password", conn->password);
    if (url && url_query(url, "clientId"))
        _cJSON_AddStringToObject(json, "client_id", url_query(url, "clientId"));
    else if (conn->clientId && strlen(conn->clientId) > 0)
        _cJSON_AddStringToObject(json, "client_id", conn->clientId);
    if (conn->reconnectToken && strlen(conn->reconnectToken) > 0)
        _cJSON_AddStringToObject(json, "id_token", conn->reconnectToken);

    if (conn->maxPendingAcks > 0) 
        _cJSON_AddNumberToObject(json, "max_pending_acks", conn->maxPendingAcks);

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
        case OP_REQUEST_REPLY:
            tibeftlConnection_handleReply(conn, json);
            break;
        case OP_ERROR:
            tibeftlConnection_handleError(conn, json);
            break;
        case OP_MAP_RESPONSE:
            tibeftlConnection_handleMapResponse(conn, json);
            break;
        }
    }

    _cJSON_Delete(json);
}

bool
tibeftl_connect(
    tibeftlErr                  err,
    tibeftlConnection           conn,
    url_t*                      url)
{
    tibeftlCompletion_reset(&conn->completion);

    websocket_open(conn->websocket, url);

    if (tibeftlCompletion_wait(&conn->completion, conn->timeout) == TIBEFTL_ERR_TIMEOUT)
    {
        websocket_close(conn->websocket);

        if (err != NULL)
            tibeftlErr_Set(err, TIBEFTL_ERR_TIMEOUT, "connect request timed out");

        return false;
    }
    else if (conn->completion.code)
    {
        websocket_close(conn->websocket);

        if (err != NULL)
            tibeftlErr_Set(err, conn->completion.code, conn->completion.reason);

        return false;
    }

    return true;
}

static void
tibeftlConnection_cancelAutoReconnect(
    tibeftlConnection           conn)
{
    if (conn == NULL)
        return;

    if (conn->reconnectThread != INVALID_THREAD)
    {
        thread_t thread = conn->reconnectThread;

        mutex_unlock(&conn->mutex);

        tibeftlCompletion_notify(&conn->reconnectCompletion, NULL, -1, NULL);

        thread_join(thread);
        thread_destroy(thread);

        mutex_lock(&conn->mutex);
    }

    mutex_unlock(&conn->mutex);
}

typedef struct tibeftlReconnectContextStruct
{
    tibeftlConnection           conn;
    int64_t                     backoff;
    url_t*                      url;

} tibeftlReconnectContext;

static void *
tibeftlConnection_reconnectThread(
    void*                       arg)
{
    tibeftlReconnectContext*    context = (tibeftlReconnectContext*)arg;
    int                         rc;

    if (context == NULL)
        return NULL;

    if (context->conn)
    {
        rc = tibeftlCompletion_wait(&context->conn->reconnectCompletion, (int)context->backoff);

        mutex_lock(&context->conn->mutex);

        context->conn->reconnectThread = INVALID_THREAD;

        tibeftlCompletion_clear(&context->conn->reconnectCompletion);

        mutex_unlock(&context->conn->mutex);

        if (rc == TIBEFTL_ERR_TIMEOUT)
            tibeftl_connect(NULL, context->conn, context->url);
    }

    free(context);

    return NULL;
}

static bool
tibeftlConnection_scheduleReconnect(
    tibeftlConnection           conn)
{
    tibeftlReconnectContext*    context;
    int64_t                     backoff = 0;
    bool                        scheduled = false;

    if (conn != NULL)
    {
        mutex_lock(&conn->mutex);

        if (conn->reconnectAttempts < conn->autoReconnectAttempts)
        {
            tibeftlConnection_setState(conn, STATE_RECONNECTING);

            if (conn->urlIndex == 0)
            {
                // add jitter by applying a randomness factor of 0.5 
                double jitter = (rand() / (RAND_MAX + 1.0)) + 0.5;
                backoff = (int64_t) (pow(2.0, (double) (conn->reconnectAttempts++)) * 1000.0 * jitter);
                if (backoff > conn->autoReconnectMaxDelay || backoff <= 0)
                    backoff = conn->autoReconnectMaxDelay;
            }

            context = calloc(1, sizeof(tibeftlReconnectContext));
            context->conn = conn;
            context->backoff = backoff;
            context->url = conn->urlList[conn->urlIndex];
            if (++conn->urlIndex >= url_list_count(conn->urlList))
                conn->urlIndex = 0;

            tibeftlCompletion_init(&conn->reconnectCompletion);

            thread_start(&conn->reconnectThread, tibeftlConnection_reconnectThread, (void*)context);
            thread_detach(conn->reconnectThread);

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

    tibeftlConnection_setState(conn, STATE_DISCONNECTED);

    mutex_unlock(&conn->mutex);

    switch (state)
    {
    case STATE_CONNECTING:
        tibeftlCompletion_notify(&conn->completion, NULL, TIBEFTL_ERR_CONNECT_FAILED, reason);
        break;
    case STATE_RECONNECTING:
        tibeftlCompletion_notify(&conn->completion, NULL, TIBEFTL_ERR_CONNECT_FAILED, reason);
    case STATE_CONNECTED:
        if (!tibeftlConnection_scheduleReconnect(conn))
        {
            tibeftlConnection_invokeErrorCb(conn, TIBEFTL_ERR_CONNECTION_LOST, reason);
        }
        break;
    default:
        break;
    }

    mutex_lock(&conn->mutex);

    if (conn->state != STATE_RECONNECTING)
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

    tibeftlConnection_setState(conn, STATE_DISCONNECTED);

    mutex_unlock(&conn->mutex);

    switch (state)
    {
    case STATE_CONNECTING:
        tibeftlCompletion_notify(&conn->completion, NULL, code, reason);
        break;
    case STATE_RECONNECTING:
        tibeftlCompletion_notify(&conn->completion, NULL, code, reason);
    case STATE_CONNECTED:
        // Reconnect when the close code reflects a server restart.
        if (code != TIBEFTL_ERR_SERVICE_RESTART || !tibeftlConnection_scheduleReconnect(conn))
        {
            tibeftlConnection_invokeErrorCb(conn, code, reason);
        }
        break;
    default:
        break;
    }

    mutex_lock(&conn->mutex);

    if (conn->state != STATE_RECONNECTING)
    {
        tibeftlConnection_notifyAllRequests(conn, code, reason);
    }

    mutex_unlock(&conn->mutex);
}

static void*
tibeftlConnection_dispatchThread(
    void*                       arg)
{
    tibeftlConnection           conn = (tibeftlConnection)arg;
    tibeftlSubscription         sub;
    tibeftlMessage              msg;
    const char*                 subId;
    int64_t                     seqNum;

    for (;;)
    {
        tibeftlAcknowledgeMode  ackMode = TIBEFTL_ACKNOWLEDGE_MODE_AUTO;
        tibeftlMessageCallback  msgCb = NULL;
        void*                   msgCbArg = NULL;

        // get the next message
        msg = tibeftlMessageList_Next(conn->msgList);

        if (!msg)
            break;

        tibeftlMessage_GetReceipt(msg, &seqNum, &subId);

        mutex_lock(&conn->mutex);

        // find the subscription
        sub = hashmap_get(conn->subscriptions, subId);
        if (sub)
        {
            if (seqNum == 0 || seqNum > sub->lastSeqNum)
            {
                ackMode = sub->ackMode;
                msgCb = sub->msgCb;
                msgCbArg = sub->msgCbArg;

                // track the last received sequence number
                if (sub->ackMode == TIBEFTL_ACKNOWLEDGE_MODE_AUTO && seqNum != 0)
                    sub->lastSeqNum = seqNum;
            }
        }

        mutex_unlock(&conn->mutex);

        // invoke the message callback
        if (msgCb)
            msgCb(conn, sub, 1, &msg, msgCbArg);

        // acknowledge the message if necessary
        if (ackMode == TIBEFTL_ACKNOWLEDGE_MODE_AUTO) 
            tibeftl_Acknowledge(NULL, conn, msg);

        tibeftlMessage_Destroy(NULL, msg);
    }

    return NULL;
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

    conn->state = STATE_INITIAL;
    conn->ref = 1;
    conn->timeout = TIBEFTL_TIMEOUT;
    conn->urlList = url_list_parse(url);

#if defined(SIGPIPE)
    // ignore SIGPIPE 
    signal(SIGPIPE, SIG_IGN);
#endif

    // seed rand
    srand((unsigned int)tibeftl_now());

    // shuffle the URLs
    url_list_shuffle(conn->urlList);

    if (opts)
    {
        if (opts->username && !conn->username)
            conn->username = strdup(opts->username);
        if (opts->password && !conn->password)
            conn->password = strdup(opts->password);
        if (opts->clientId && !conn->clientId)
            conn->clientId = strdup(opts->clientId);
        if (opts->trustStore && !conn->trustStore)
            conn->trustStore = strdup(opts->trustStore);

        if (opts->trustAll == true)
            conn->trustAll = opts->trustAll;

        if (opts->timeout > 0)
            conn->timeout = opts->timeout;
        if (opts->autoReconnectAttempts > 0)
            conn->autoReconnectAttempts = opts->autoReconnectAttempts;
        if (opts->autoReconnectMaxDelay > 0)
            conn->autoReconnectMaxDelay = opts->autoReconnectMaxDelay;

        if (opts->ver > 0)
        {
            if (opts->stateChangeCallback)
                conn->stateChangeCallback = opts->stateChangeCallback;
            if (opts->stateChangeCallbackArg)
                conn->stateChangeCallbackArg = opts->stateChangeCallbackArg;
        }

        if (opts->ver > 1)
        {
            if (opts->maxPendingAcks != 0)
                conn->maxPendingAcks = opts->maxPendingAcks;
        }
    }

    conn->errCb = errCb;
    conn->errCbArg = errCbArg;

    mutex_init(&conn->mutex);

    conn->msgList = tibeftlMessageList_Create();

    thread_start(&conn->dispatchThread, tibeftlConnection_dispatchThread, (void*)conn);

    websocket_options_init(websocketOptions);

    websocketOptions.protocol = TIBEFTL_PROTOCOL;
    websocketOptions.trustStore = conn->trustStore;
    websocketOptions.trustAll = conn->trustAll;
    websocketOptions.timeout = conn->timeout;
    websocketOptions.open_cb = tibeftlConnection_onWebSocketOpen;
    websocketOptions.text_cb = tibeftlConnection_onWebSocketText;
    websocketOptions.error_cb = tibeftlConnection_onWebSocketError;
    websocketOptions.close_cb = tibeftlConnection_onWebSocketClose;
    websocketOptions.context = conn;

    conn->websocket = websocket_create(&websocketOptions);
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

    tibeftlMessageList_Close(conn->msgList);

    thread_join(conn->dispatchThread);
    thread_destroy(conn->dispatchThread);

    tibeftlMessageList_Destroy(conn->msgList);

    hashmap_iterate(conn->subscriptions, tibeftlSubscription_cleanup, NULL);

    hashmap_destroy(conn->subscriptions);

    websocket_destroy(conn->websocket);

    tibeftlCompletion_clear(&conn->completion);

    mutex_destroy(&conn->mutex);

    if (conn->urlList)
        url_list_destroy(conn->urlList);
    if (conn->username)
        free(conn->username);
    if (conn->password)
        free(conn->password);
    if (conn->clientId)
        free(conn->clientId);
    if (conn->trustStore)
        free(conn->trustStore);
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
    int                         i;

    if (tibeftlErr_IsSet(err))
        return NULL;

    conn = tibeftlConnection_create(url, opts, errCb, errCbArg);

    for (i = 0; conn->urlList[i]; i++)
    {
        tibeftlConnection_setState(conn, STATE_CONNECTING);

        tibeftlErr_Clear(err);

        if (tibeftl_connect(err, conn, conn->urlList[i]))
            return conn;
    }

    tibeftlConnection_destroy(conn);

    return NULL;
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
    case STATE_CONNECTED:
        tibeftlConnection_setState(conn, STATE_DISCONNECTING);

        mutex_unlock(&conn->mutex);

        json = _cJSON_CreateObject();
        _cJSON_AddNumberToObject(json, "op", OP_DISCONNECT);

        text = _cJSON_PrintUnformatted(json);

        websocket_send_text(conn->websocket, text);

        websocket_close(conn->websocket);

        _cJSON_Delete(json);
        _cJSON_free(text);

        mutex_lock(&conn->mutex);

        tibeftlConnection_setState(conn, STATE_DISCONNECTED);
        break;
    case STATE_RECONNECTING:
        tibeftlConnection_setState(conn, STATE_DISCONNECTING);

        tibeftlConnection_cancelAutoReconnect(conn);

        tibeftlConnection_setState(conn, STATE_DISCONNECTED);
        break;
    default:
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
    bool                        connected;
    int                         i;

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
    case STATE_RECONNECTING:
        tibeftlConnection_cancelAutoReconnect(conn);
    case STATE_DISCONNECTED:
        for (i = 0; conn->urlList[i]; i++)
        {
            tibeftlConnection_setState(conn, STATE_CONNECTING);

            mutex_unlock(&conn->mutex);

            tibeftlErr_Clear(err);

            connected = tibeftl_connect(err, conn, conn->urlList[i]);

            mutex_lock(&conn->mutex);

            if (connected)
                break;
            else
                tibeftlConnection_setState(conn, STATE_DISCONNECTED);
        }
        break;
    default:
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

    return (state == STATE_CONNECTED || state == STATE_RECONNECTING ? true : false);
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

    if (conn->state == STATE_CONNECTED || conn->state == STATE_RECONNECTING)
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

tibeftlMessage
tibeftl_SendRequest(
    tibeftlErr                  err,
    tibeftlConnection           conn,
    tibeftlMessage              request,
    int64_t                     timeout)
{
    tibeftlCompletion           completion = NULL;
    tibeftlMessage              reply = NULL;
    _cJSON*                     json;
    int64_t                     seqNum = 0;
    char*                       text;

    if (tibeftlErr_IsSet(err))
        return NULL;

    if (!conn)
    {
        tibeftlErr_Set(err, TIBEFTL_ERR_INVALID_ARG, "conn is NULL");
        return NULL;
    }

    if (!request)
    {
        tibeftlErr_Set(err, TIBEFTL_ERR_INVALID_ARG, "msg is NULL");
        return NULL;
    }

    mutex_lock(&conn->mutex);

    if (conn->state == STATE_CONNECTED || conn->state == STATE_RECONNECTING)
    {
        seqNum = ++conn->pubSeqNum;

        json = _cJSON_CreateObject();
        _cJSON_AddNumberToObject(json, "op", OP_REQUEST);
        _cJSON_AddNumberToObject(json, "seq", seqNum);
        _cJSON_AddItemReferenceToObject(json, "body", tibeftlMessage_GetJSON(request));

        text = _cJSON_PrintUnformatted(json);

        if (conn->protocol < 1)
        {
            tibeftlErr_Set(err, TIBEFTL_ERR_NOT_SUPPORTED, "send request is not supported with this server");
        }
        else if (conn->maxMsgSize > 0 && strlen(text) > conn->maxMsgSize)
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
        if (tibeftlCompletion_wait(completion, timeout) == TIBEFTL_ERR_TIMEOUT)
        {
            tibeftlErr_Set(err, TIBEFTL_ERR_TIMEOUT, "request timed out");
        }
        else if (completion->code)
        {
            tibeftlErr_Set(err, completion->code, completion->reason);
        }

        mutex_lock(&conn->mutex);

        reply = completion->response;

        completion->response = NULL;

        tibeftlConnection_unregisterRequest(conn, seqNum);

        mutex_unlock(&conn->mutex);
    }

    return reply;
}

void
tibeftl_SendReply(
    tibeftlErr                  err,
    tibeftlConnection           conn,
    tibeftlMessage              reply,
    tibeftlMessage              request)
{
    tibeftlCompletion           completion = NULL;
    _cJSON*                     json;
    int64_t                     seqNum = 0;
    const char*                 replyTo = NULL;
    int64_t                     reqId = 0;
    char*                       text;

    if (tibeftlErr_IsSet(err))
        return;

    if (!conn)
    {
        tibeftlErr_Set(err, TIBEFTL_ERR_INVALID_ARG, "conn is NULL");
        return;
    }

    if (!reply)
    {
        tibeftlErr_Set(err, TIBEFTL_ERR_INVALID_ARG, "reply is NULL");
        return;
    }

    if (!request)
    {
        tibeftlErr_Set(err, TIBEFTL_ERR_INVALID_ARG, "request is NULL");
        return;
    }

    tibeftlMessage_GetReplyTo(request, &replyTo, &reqId);

    if (!replyTo)
    {
        tibeftlErr_Set(err, TIBEFTL_ERR_INVALID_ARG, "not a request message");
        return;
    }

    mutex_lock(&conn->mutex);

    if (conn->state == STATE_CONNECTED || conn->state == STATE_RECONNECTING)
    {
        seqNum = ++conn->pubSeqNum;

        json = _cJSON_CreateObject();
        _cJSON_AddNumberToObject(json, "op", OP_REPLY);
        _cJSON_AddNumberToObject(json, "seq", seqNum);
        _cJSON_AddStringToObject(json, "to", replyTo);
        _cJSON_AddNumberToObject(json, "req", reqId);
        _cJSON_AddItemReferenceToObject(json, "body", tibeftlMessage_GetJSON(reply));

        text = _cJSON_PrintUnformatted(json);

        if (conn->protocol < 1)
        {
            tibeftlErr_Set(err, TIBEFTL_ERR_NOT_SUPPORTED, "send reply is not supported with this server");
        }
        else if (conn->maxMsgSize > 0 && strlen(text) > conn->maxMsgSize)
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
            tibeftlErr_Set(err, TIBEFTL_ERR_TIMEOUT, "reply timed out");
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
    int64_t                     subId;

    if (tibeftlErr_IsSet(err))
        return NULL;

    if (!conn)
    {
        tibeftlErr_Set(err, TIBEFTL_ERR_INVALID_ARG, "conn is NULL");
        return NULL;
    }

    mutex_lock(&conn->mutex);

    if (conn->state == STATE_CONNECTED || conn->state == STATE_RECONNECTING)
    {
        subId = ++conn->lastSubId;

        sub = tibeftlSubscription_create(subId, matcher, durable, opts, msgCb, msgCbArg);

        completion = tibeftlConnection_registerSubscription(conn, sub);

        tibeftlSubscription_subscribe(sub->subId, sub, conn);
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

        if (tibeftlErr_IsSet(err))
        {
            mutex_lock(&conn->mutex);

            tibeftlConnection_unregisterSubscription(conn, (const char*)sub->subId);

            mutex_unlock(&conn->mutex);

            sub = NULL;
        }
    }

    return sub;
}

void
tibeftl_CloseSubscription(
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

    if (conn->protocol < 1)
    {
        tibeftlErr_Set(err, TIBEFTL_ERR_NOT_SUPPORTED, "close subscription is not supported with this server");
    }
    else if (conn->state == STATE_CONNECTED || conn->state == STATE_RECONNECTING)
    {
        tibeftlSubscription_close((const char*)sub->subId, (void*)sub, (void*)conn);
    }
    else
    {
        tibeftlErr_Set(err, TIBEFTL_ERR_NOT_CONNECTED, "not connected");
    }

    mutex_unlock(&conn->mutex);
}

void
tibeftl_CloseAllSubscriptions(
    tibeftlErr                  err,
    tibeftlConnection           conn)
{
    if (!conn)
    {
        tibeftlErr_Set(err, TIBEFTL_ERR_INVALID_ARG, "connection is NULL");
        return;
    }

    mutex_lock(&conn->mutex);

    if (conn->protocol < 1)
    {
        tibeftlErr_Set(err, TIBEFTL_ERR_NOT_SUPPORTED, "close subscription is not supported with this server");
    }
    else if (conn->state == STATE_CONNECTED || conn->state == STATE_RECONNECTING)
    {
        hashmap_t* copy = hashmap_copy(conn->subscriptions);
        hashmap_iterate(copy, tibeftlSubscription_close, (void*)conn);
        hashmap_destroy(copy);
    }
    else
    {
        tibeftlErr_Set(err, TIBEFTL_ERR_NOT_CONNECTED, "not connected");
    }

    mutex_unlock(&conn->mutex);
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

    if (conn->state == STATE_CONNECTED || conn->state == STATE_RECONNECTING)
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

    if (conn->state == STATE_CONNECTED || conn->state == STATE_RECONNECTING)
    {
        hashmap_t* copy = hashmap_copy(conn->subscriptions);
        hashmap_iterate(copy, tibeftlSubscription_unsubscribe, (void*)conn);
        hashmap_destroy(copy);
    }
    else
    {
        tibeftlErr_Set(err, TIBEFTL_ERR_NOT_CONNECTED, "not connected");
    }

    mutex_unlock(&conn->mutex);
}

void
tibeftl_Acknowledge(
    tibeftlErr                  err,
    tibeftlConnection           conn,
    tibeftlMessage              msg)
{
    int64_t                     seqNum;

    if (!conn)
    {
        tibeftlErr_Set(err, TIBEFTL_ERR_INVALID_ARG, "connection is NULL");
        return;
    }

    if (!msg)
    {
        tibeftlErr_Set(err, TIBEFTL_ERR_INVALID_ARG, "message is NULL");
        return;
    }

    mutex_lock(&conn->mutex);

    if (conn->state == STATE_CONNECTED || conn->state == STATE_RECONNECTING)
    {
        tibeftlMessage_GetReceipt(msg, &seqNum, NULL);

        tibeftlConnection_acknowledge(conn, seqNum, NULL);
    }
    else
    {
        tibeftlErr_Set(err, TIBEFTL_ERR_NOT_CONNECTED, "not connected");
    }

    mutex_unlock(&conn->mutex);
}

void
tibeftl_AcknowledgeAll(
    tibeftlErr                  err,
    tibeftlConnection           conn,
    tibeftlMessage              msg)
{
    int64_t                     seqNum;
    const char*                 subId;

    if (!conn)
    {
        tibeftlErr_Set(err, TIBEFTL_ERR_INVALID_ARG, "connection is NULL");
        return;
    }

    if (!msg)
    {
        tibeftlErr_Set(err, TIBEFTL_ERR_INVALID_ARG, "message is NULL");
        return;
    }

    mutex_lock(&conn->mutex);

    if (conn->state == STATE_CONNECTED || conn->state == STATE_RECONNECTING)
    {
        tibeftlMessage_GetReceipt(msg, &seqNum, &subId);

        tibeftlConnection_acknowledge(conn, seqNum, subId);
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
tibeftl_RemoveKVMap(
    tibeftlErr                  err,
    tibeftlConnection           conn,
    const char*                 name)
{
    _cJSON*                     json;
    char*                       text;

    if (tibeftlErr_IsSet(err))
        return;

    if (!conn)
    {
        tibeftlErr_Set(err, TIBEFTL_ERR_INVALID_ARG, "conn is NULL");
        return;
    }

    if (!name)
    {
        tibeftlErr_Set(err, TIBEFTL_ERR_INVALID_ARG, "name is NULL");
        return;
    }

    mutex_lock(&conn->mutex);

    if (conn->state == STATE_CONNECTED || conn->state == STATE_RECONNECTING)
    {
        json = _cJSON_CreateObject();
        _cJSON_AddNumberToObject(json, "op", OP_MAP_DESTROY);
        _cJSON_AddStringToObject(json, "map", name);

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

    if (map->conn->state == STATE_CONNECTED || map->conn->state == STATE_RECONNECTING)
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

    if (map->conn->state == STATE_CONNECTED || map->conn->state == STATE_RECONNECTING)
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

        mutex_lock(&map->conn->mutex);

        msg = completion->response;

        completion->response = NULL;

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

    if (map->conn->state == STATE_CONNECTED || map->conn->state == STATE_RECONNECTING)
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

const char*
tibeftlConnectionState_ToString(
    tibeftlConnectionState      state)
{
    switch(state)
    {
    case STATE_INITIAL:
        return "initial";
    case STATE_DISCONNECTED:
        return "disconnected";
    case STATE_CONNECTING:
        return "connecting";
    case STATE_CONNECTED:
        return "connected";
    case STATE_DISCONNECTING:
        return "disconnecting";
    case STATE_RECONNECTING:
        return "reconnecting";
    default:
        return "unknown";
    }
}
