/*
 * Copyright (c) 2010-$Date: 2020-06-25 09:10:25 -0700 (Thu, 25 Jun 2020) $ TIBCO Software Inc.
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
onRequest(
    tibeftlConnection   conn,
    tibeftlSubscription sub,
    int                 cnt,
    tibeftlMessage*     msg,
    void*               arg)
{
    int*                done = (int*)arg;
    tibeftlErr          err;
    tibeftlMessage      reply;
    char                buffer[1024];
    int                 i;

    err = tibeftlErr_Create();

    for (i = 0; i < cnt; i++) {
        tibeftlMessage_ToString(err, msg[i], buffer, sizeof(buffer));
        printf("received request message\n  %s\n", buffer);

        // create the reply message
        reply = tibeftlMessage_Create(err);
        tibeftlMessage_SetString(err, reply, "type", "reply");
        tibeftlMessage_SetString(err, reply, "text", "This is a sample eFTL reply message");

        // send the reply
        tibeftl_SendReply(err, conn, reply, msg[i]);

        // destroy the reply message
        tibeftlMessage_Destroy(err, reply);

        // check for errors
        if (tibeftlErr_IsSet(err)) {
            printf("tibeftlConnection_SendReply failed: %d - %s\n",
                    tibeftlErr_GetCode(err), tibeftlErr_GetDescription(err));
            tibeftlErr_Clear(err);
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

    // connect to the server
    conn = tibeftl_Connect(err, url, &opts, onError, NULL);

    // create a matcher to receive request messages 
    matcher = "{\"type\":\"request\"}";

    // create a subscription using the matcher
    sub = tibeftl_SubscribeWithOptions(err, conn, matcher, NULL, NULL, onRequest, &done);

    printf("waiting for request messages\n");

    // wait for a message
    do {
        delay(1);
    } while (!done && !tibeftlErr_IsSet(err));

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

