/*
 * Copyright (c) 2020 Cloud Software Group, Inc.
 * Licensed under a BSD-style license. Refer to [LICENSE]
 *
 * $Id$
 *
 */

#ifndef INCLUDED_TIBEFTL_MSG_H
#define INCLUDED_TIBEFTL_MSG_H

#include "cJSON.h"

typedef struct tibeftlMessageListStruct
               tibeftlMessageListRec, 
              *tibeftlMessageList;

tibeftlMessageList
tibeftlMessageList_Create(void);

void
tibeftlMessageList_Destroy(
    tibeftlMessageList          list);

void
tibeftlMessageList_Close(
    tibeftlMessageList          list);

bool
tibeftlMessageList_Add(
    tibeftlMessageList          list,
    tibeftlMessage              message);

tibeftlMessage
tibeftlMessageList_Next(
    tibeftlMessageList          list);

tibeftlMessage
tibeftlMessage_CreateWithJSON(
    _cJSON*                     json);

_cJSON*
tibeftlMessage_GetJSON(
    tibeftlMessage              message);

void
tibeftlMessage_SetReceipt(
    tibeftlMessage              message,
    int64_t                     seqNum,
    const char*                 subId);

void
tibeftlMessage_GetReceipt(
    tibeftlMessage              message,
    int64_t*                    seqNum,
    const char**                subId);

void
tibeftlMessage_SetReplyTo(
    tibeftlMessage              message,
    const char*                 replyTo,
    int64_t                     reqId);

void
tibeftlMessage_GetReplyTo(
    tibeftlMessage              message,
    const char**                replyTo,
    int64_t*                    reqId);

void
tibeftlMessage_SetStoreMessageId(
    tibeftlMessage              message,
    int64_t                     msgId);

void
tibeftlMessage_SetDeliveryCount(
    tibeftlMessage              message,
    int64_t                     deliveryCount);

#endif /* INCLUDED_TIBEFTL_MSG_H */
