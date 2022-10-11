/*
 * Copyright (c) 2010-$Date: 2020-05-27 09:56:45 -0700 (Wed, 27 May 2020) $ TIBCO Software Inc.
 * Licensed under a BSD-style license. Refer to [LICENSE]
 * For more information, please contact:
 * TIBCO Software Inc., Palo Alto, California, USA
 */

/*
 * This is a sample of a basic C eFTL key-value map remove.
 */

#include <stdio.h>
#include <time.h>

#include "tib/eftl.h"

#define DEFAULT_URL "ws://localhost:8585/map"

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

    printf("key-value remove\n");

    // create a key-value map
    map = tibeftl_CreateKVMap(err, conn, "sample_map");

    // remove the key-value pair from the map.
    tibeftlKVMap_Remove(err, map, "key1");

    // cleanup
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

