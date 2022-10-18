/*
 * Copyright (c) 2010-2017 TIBCO Software Inc.
 * Licensed under a BSD-style license. Refer to [LICENSE]
 * For more information, please contact:
 * TIBCO Software Inc., Palo Alto, California, USA
 */

/*
 * This is a sample of a basic Hello world C FTL publisher program which sends a 
 * hello world msg. 
 */

#include <stdlib.h>
#include <stdio.h>
#include <stddef.h>
#include <string.h>
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

const char      *realmServer    = DEFAULT_URL;

int
main(int argc, char** argv)
{
    tibEx               ex;
    tibRealm            realm = NULL;
    tibPublisher        pub;
    tibMessage          msg;

    printf("#\n# %s\n#\n# %s\n#\n", argv[0], tib_Version());

    // see if a url is specified.
    if (argc > 1)
        realmServer = argv[1];

    ex = tibEx_Create();

    tib_Open(ex, TIB_COMPATIBILITY_VERSION);

    // connect to the realmserver to get a config, 'default' app is returned if the appName is NULL.
    realm = tibRealm_Connect(ex, realmServer, NULL, NULL);

    // Create the publisher object
    pub = tibPublisher_Create(ex, realm, NULL, NULL);
    CHECK(ex);

    // create the hello world msg.
    msg = tibMessage_Create(ex, realm, NULL);
    tibMessage_SetString(ex, msg, "type", "hello");
    tibMessage_SetString(ex, msg, "message", "hello world earth");

    // send hello world ftl msg
    printf("sending 'hello world' message\n"); 
    fflush(stdout);

    tibPublisher_Send(ex, pub, msg);

    // cleanup
    tibMessage_Destroy(ex, msg);

    tibPublisher_Close(ex, pub);
    tibRealm_Close(ex, realm);

    tib_Close(ex);

    // Check if it worked before destroying the exception
    CHECK(ex);
    tibEx_Destroy(ex);

    return 0;
}
