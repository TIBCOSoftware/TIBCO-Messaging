/*
 * Copyright (c) 2010-$Date: 2018-05-29 13:54:20 -0500 (Tue, 29 May 2018) $ TIBCO Software Inc.
 * Licensed under a BSD-style license. Refer to [LICENSE]
 * For more information, please contact:
 * TIBCO Software Inc., Palo Alto, California, USA
 */

/*
 * This is a sample of a basic C eFTL subscriber
 * using a shared durable subscription.
 */

#include <stdio.h>

#ifdef _WIN32
#include <winsock2.h>
#else
#include <sys/select.h>
#endif

#include "tib/eftl.h"

#define DEFAULT_URL "ws://localhost:9191/channel"

void
delay(int secs)
{
    struct timeval  tv = {secs, 0};

    select(0, NULL, NULL, NULL, &tv);
}

void
onMessage(
    tibeftlConnection   conn,
    tibeftlSubscription sub,
    int                 cnt,
    tibeftlMessage*     msg,
    void*               arg)
{
    tibeftlErr          err;
    char                buffer[1024];
    int                 i;

    err = tibeftlErr_Create();

    for (i = 0; i < cnt; i++) {
        tibeftlMessage_ToString(err, msg[i], buffer, sizeof(buffer));

        if (tibeftlErr_IsSet(err)) {
            printf("tibeftlMessage_ToString failed: %d - %s\n",
                    tibeftlErr_GetCode(err), tibeftlErr_GetDescription(err));
            tibeftlErr_Clear(err);
        } else {
            printf("received message\n  %s\n", buffer);
        }
    }

    tibeftlErr_Destroy(err);
}

void
onError(
    tibeftlConnection   conn,
    tibeftlErr          cause,
    void*               arg)
{
    printf("connection error: %d - %s\n",
            tibeftlErr_GetCode(cause), tibeftlErr_GetDescription(cause));

    exit(1);
}

int
main(int argc, char** argv)
{
    tibeftlErr                  err;
    tibeftlConnection           conn;
    tibeftlSubscription         sub;
    tibeftlOptions              opts = tibeftlOptionsDefault;
    tibeftlSubscriptionOptions  subOpts = tibeftlSubscriptionOptionsDefault;
    const char*                 url = DEFAULT_URL;
    char                        matcher[64];

    printf("#\n# %s\n#\n# %s\n#\n", argv[0], tibeftl_Version());

    if (argc > 1)
        url = argv[1];

    err = tibeftlErr_Create();

    // configure options
    opts.username = NULL;
    opts.password = NULL;

    // connect to the server
    conn = tibeftl_Connect(err, url, &opts, onError, NULL);

    // configure subscription options
    subOpts.durableType = TIBEFTL_DURABLE_TYPE_SHARED;

    // create a matcher to receive messages published to destination "sample"
    snprintf(matcher, sizeof(matcher), "{\"%s\":\"%s\"}",
        TIBEFTL_FIELD_NAME_DESTINATION, "sample");

    // create a subscription using the matcher
    sub = tibeftl_SubscribeWithOptions(err, conn, matcher, "sample-shared", &subOpts, onMessage, NULL);

    // wait for messages
    if (!tibeftlErr_IsSet(err)) {
        delay(30);
    }

    // cleanup
    tibeftl_Unsubscribe(err, conn, sub);

    // disconnect from server
    tibeftl_Disconnect(err, conn);

    // check for errors
    if (tibeftlErr_IsSet(err)) {
        printf("error: %d - %s\n",
                tibeftlErr_GetCode(err), tibeftlErr_GetDescription(err));
    }

    tibeftlErr_Destroy(err);

    return 0;
}

