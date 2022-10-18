/*
 * Copyright (c) 2010-$Date: 2020-07-09 13:18:24 -0700 (Thu, 09 Jul 2020) $ TIBCO Software Inc.
 * Licensed under a BSD-style license. Refer to [LICENSE]
 * For more information, please contact:
 * TIBCO Software Inc., Palo Alto, California, USA
 */

/*
 * This is a sample of a basic C eFTL subscriber.
 */

#include <stdio.h>

#ifdef _WIN32
#include <winsock2.h>
#else
#include <sys/select.h>
#endif

#include "tib/eftl.h"

#define DEFAULT_URL "ws://localhost:8585/channel"

void
delay(int secs)
{
#ifdef _WIN32
    Sleep(secs * 1000);
#else
    struct timeval  tv = {secs, 0};

    select(0, NULL, NULL, NULL, &tv);
#endif
}

void
onMessage(
    tibeftlConnection   conn,
    tibeftlSubscription sub,
    int                 cnt,
    tibeftlMessage*     msg,
    void*               arg)
{
    int*                done = (int*)arg;
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

    *done = 1;
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
    tibeftlErr                 err;
    tibeftlOptions             opts = tibeftlOptionsDefault;
    tibeftlConnection          conn;
    tibeftlSubscription        sub;
    tibeftlSubscriptionOptions subopts = tibeftlSubscriptionOptionsDefault;
    const char*                url = DEFAULT_URL;
    const char*                matcher = NULL;
    int                        done = 0;

    printf("#\n# %s\n#\n# %s\n#\n", argv[0], tibeftl_Version());

    if (argc > 1)
        url = argv[1];

    err = tibeftlErr_Create();

    // configure options
    opts.username = NULL;
    opts.password = NULL;
    opts.clientId = "sample-c-client";

    // connect to the server
    conn = tibeftl_Connect(err, url, &opts, onError, NULL);

    // create a subscription matcher to match message fields
    // with type string or long, or to test for the presences 
    // or absence of a named message field 
    //
    // this matcher matches all messages containing a field
    // named 'type' with a value of 'hello'
    matcher = "{\"type\":\"hello\"}";

    // configure subscription options
    subopts.acknowledgeMode = TIBEFTL_ACKNOWLEDGE_MODE_AUTO;

    // create a subscription using the matcher
    //
    // always set the client identifier when creating a durable subscription
    // as durable subscriptions are mapped to the client identifier
    //
    sub = tibeftl_SubscribeWithOptions(err, conn, matcher, "sample-durable", &subopts, onMessage, &done);

    printf("waiting for messages\n");

    // wait for a message
    do {
        delay(1);
    } while (!done && !tibeftlErr_IsSet(err));

    // unsubscribe, this will permanently remove a durable subscription
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

