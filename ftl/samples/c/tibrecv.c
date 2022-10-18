/*
 * Copyright (c) 2010-2017 TIBCO Software Inc.
 * Licensed under a BSD-style license. Refer to [LICENSE]
 * For more information, please contact:
 * TIBCO Software Inc., Palo Alto, California, USA
 */

/*
 * This is a sample of a basic C Hello World FTL subscriber program which receives a
 * message and then cleans up and exits.
 */

#include <stdlib.h>
#include <stdio.h>
#include <stddef.h>
#include <string.h>
#include <ctype.h>

#include "tib/ftl.h"

#define DEFAULT_URL "http://localhost:8080"
#define CHECK(ex) \
{                              \
    if(tibEx_GetErrorCode(ex)) \
    {                          \
       char exStr[1024];       \
       fprintf(stderr, "%s: %d\n", __FILE__, __LINE__); \
       tibEx_ToString(ex, exStr, sizeof(exStr));        \
       fprintf(stderr, "%s\n", exStr);                  \
       tib_Close(ex);                                   \
       exit(-1);                                        \
    }                                                   \
}

tibbool_t       done            = tibfalse;
const char      *realmServer    = DEFAULT_URL;

void
onMessages(
    tibEx               ex,
    tibEventQueue       msgQueue,
    tibint32_t          msgNum,
    tibMessage          *msgs,
    void                **closures)
{
    tibint32_t          i;
    char                buffer[1024];

    for (i = 0; i < msgNum; i++)
    {
        printf("received message\n");
        (void)tibMessage_ToString(ex, msgs[i], buffer, sizeof(buffer));
        printf("  %s\n", buffer);
        fflush(stdout);
    }
    done = tibtrue;
}

int
main(int argc, char** argv)
{
    tibEx               ex;
    tibRealm            realm;
    tibEventQueue       queue = NULL;
    tibSubscriber       sub;
    tibContentMatcher   cm;

    printf("#\n# %s\n#\n# %s\n#\n", argv[0], tib_Version());

    if (argc > 1)
        realmServer = argv[1];

    ex = tibEx_Create();

    tib_Open(ex, TIB_COMPATIBILITY_VERSION);

    // connect to the realmserver to get a configuration
    realm = tibRealm_Connect(ex, realmServer, NULL, NULL);

    // The content matcher string is essentially a json string
    // with the following format {"type":"hello"}
    cm = tibContentMatcher_Create(ex, realm, "{\"type\":\"hello\"}");

    // create a subscriber
    sub = tibSubscriber_Create(ex, realm, NULL, cm, NULL);

    // Create a queue and add subscribers to it
    queue = tibEventQueue_Create(ex, realm, NULL);

    tibEventQueue_AddSubscriber(ex, queue, sub, onMessages, NULL);
    CHECK(ex);

    // keep receving until we are not done.
    printf("waiting for message(s)\n");
    do
    {
        tibEventQueue_Dispatch(ex, queue, TIB_TIMEOUT_WAIT_FOREVER);

    } while (!done && tibEx_GetErrorCode(ex) == TIB_OK);

    // cleanup
    tibEventQueue_RemoveSubscriber(ex, queue, sub, NULL);

    tibEventQueue_Destroy(ex, queue, NULL);

    tibSubscriber_Close(ex, sub);

    tibContentMatcher_Destroy(ex, cm);

    tibRealm_Close(ex, realm);
    tib_Close(ex);

    // Check if it worked before destroying the exception
    CHECK(ex);
    tibEx_Destroy(ex);

    return 0;
}

