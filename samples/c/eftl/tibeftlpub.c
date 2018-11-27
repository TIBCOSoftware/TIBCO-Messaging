/*
 * Copyright (c) 2010-$Date: 2018-05-29 13:54:20 -0500 (Tue, 29 May 2018) $ TIBCO Software Inc.
 * Licensed under a BSD-style license. Refer to [LICENSE]
 * For more information, please contact:
 * TIBCO Software Inc., Palo Alto, California, USA
 */

/*
 * This is a sample of a basic C eFTL publisher.
 */

#include <stdio.h>
#include <time.h>

#include "tib/eftl.h"

#define DEFAULT_URL "ws://localhost:9191/channel"

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
    tibeftlMessage      msg;
    tibeftlOptions      opts = tibeftlOptionsDefault;
    const char*         url = DEFAULT_URL;

    printf("#\n# %s\n#\n# %s\n#\n", argv[0], tibeftl_Version());

    if (argc > 1)
        url = argv[1];

    err = tibeftlErr_Create();

    // configure options
    opts.username = NULL;
    opts.password = NULL;

    // connect to the server
    conn = tibeftl_Connect(err, url, &opts, onError, NULL);

    // publish a message to destination "sample"
    msg = tibeftlMessage_Create(err);
    tibeftlMessage_SetString(err, msg, TIBEFTL_FIELD_NAME_DESTINATION, "sample");
    tibeftlMessage_SetString(err, msg, "text", "This is a sample eFTL message");
    tibeftlMessage_SetTime(err, msg, "time", (int64_t)(time(NULL) * 1000));

    tibeftl_Publish(err, conn, msg);

    // cleanup
    tibeftlMessage_Destroy(err, msg);

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

