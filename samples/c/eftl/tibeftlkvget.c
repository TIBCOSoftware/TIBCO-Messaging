/*
 * Copyright (c) 2010-$Date: 2018-06-06 13:30:42 -0500 (Wed, 06 Jun 2018) $ TIBCO Software Inc.
 * Licensed under a BSD-style license. Refer to [LICENSE]
 * For more information, please contact:
 * TIBCO Software Inc., Palo Alto, California, USA
 */

/*
 * This is a sample of a basic C eFTL key-value map get.
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
    tibeftlKVMap        map;
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

    // create a key-value map
    map = tibeftl_CreateKVMap(err, conn, "MyMap");

    // get the key-value pair from the map.
    msg = tibeftlKVMap_Get(err, map, "MyKey");

    if (msg) {
        char buffer[1024];

        tibeftlMessage_ToString(err, msg, buffer, sizeof(buffer));

        if (tibeftlErr_IsSet(err)) {
            printf("tibeftlMessage_ToString failed: %d - %s\n",
                    tibeftlErr_GetCode(err), tibeftlErr_GetDescription(err));
            tibeftlErr_Clear(err);
        } else {
            printf("key-value get\n  %s\n", buffer);
        }
    } else {
        printf("key-value get: not found\n");
    }

    // cleanup
    tibeftlMessage_Destroy(err, msg);
    tibeftlKVMap_Destroy(err, map);

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

