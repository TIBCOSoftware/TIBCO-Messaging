/*
 * Copyright (c) 2010-$Date: 2020-05-27 09:56:45 -0700 (Wed, 27 May 2020) $ TIBCO Software Inc.
 * Licensed under a BSD-style license. Refer to [LICENSE]
 * For more information, please contact:
 * TIBCO Software Inc., Palo Alto, California, USA
 */

/*
 * This is a sample of a basic C eFTL subscriber
 * using a last-value durable subscription.
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
    const char*                 matcher = NULL;

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
    //
    // last-value durable subscriptions require the key,
    // on which the last-value durable messages will be
    // indexed, to be present in the matcher
    subOpts.durableType = TIBEFTL_DURABLE_TYPE_LAST_VALUE;
    subOpts.durableKey = "type";

    // create a subscription matcher to match message fields
    // with type string or long, or to test for the presences 
    // or absence of a named message field 
    //
    // this matcher matches all messages containing a field
    // named 'type' with a value of 'hello' 
    matcher = "{\"type\":\"hello\"}";

    // create a subscription using the matcher
    sub = tibeftl_SubscribeWithOptions(err, conn, matcher, "sample-lastvalue-durable", &subOpts, onMessage, NULL);

    printf("waiting for messages\n");

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

