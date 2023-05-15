/*
 * Copyright (c) 2010-2020 Cloud Software Group, Inc.
 * Licensed under a BSD-style license. Refer to [LICENSE]
 */

/*
 * This is a sample of a basic C eFTL publisher.
 */

#include <stdio.h>
#include <time.h>

#include "tib/eftl.h"

#define DEFAULT_URL "ws://localhost:8585/channel"

void
onError(
    tibeftlConnection   conn,
    tibeftlErr          err,
    void*               arg)
{
    printf("connection error: %d - %s\n",
        tibeftlErr_GetCode(err), tibeftlErr_GetDescription(err));
}

int
main(int argc, char** argv)
{
    tibeftlErr          err;
    tibeftlConnection   conn;
    tibeftlMessage      request;
    tibeftlMessage      reply;
    tibeftlOptions      opts = tibeftlOptionsDefault;
    const char*         url = DEFAULT_URL;
    char                buffer[1024];

    printf("#\n# %s\n#\n# %s\n#\n", argv[0], tibeftl_Version());

    if (argc > 1)
        url = argv[1];

    err = tibeftlErr_Create();

    // configure options
    opts.username = NULL;
    opts.password = NULL;

    // connect to the server
    conn = tibeftl_Connect(err, url, &opts, onError, NULL);

    // create the request message 
    request = tibeftlMessage_Create(err);
    tibeftlMessage_SetString(err, request, "type", "request");
    tibeftlMessage_SetString(err, request, "text", "This is a sample eFTL request message");

    // send the request and wait up to 10 seconds for the reply
    reply = tibeftl_SendRequest(err, conn, request, 10000);

    if (reply)
    {
        tibeftlMessage_ToString(err, reply, buffer, sizeof(buffer));
        printf("received reply message\n  %s\n", buffer);
    }
    else
    {
        printf("request timed out\n");
    }

    // cleanup
    tibeftlMessage_Destroy(err, request);
    tibeftlMessage_Destroy(err, reply);

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

