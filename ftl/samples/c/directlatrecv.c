/*
 * Copyright (c) 2010-2017 TIBCO Software Inc.
 * Licensed under a BSD-style license. Refer to [LICENSE]
 * For more information, please contact:
 * TIBCO Software Inc., Palo Alto, California, USA
 */

/*
 * This is a sample C responder program which is used with the directlatsend
 * program to measure basic latency between the publisher and subscriber.
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "tib/ftl.h"

#include "tibaux.h"

#define DEFAULT_URL "http://localhost:8080"

const char              *realmServer            = DEFAULT_URL;
const char              *appName                = "directlatrecv";
const char              *sendEndpointName       = "directlatrecv-sendendpoint";
const char              *receiveEndpointName    = "directlatrecv-recvendpoint";
const char              *user                   = "guest";
const char              *password               = "guest-pw";
const char              *trcLevel               = NULL;
const char              *identifier             = NULL;
const char              *clientLabel            = NULL;

tibDirectPublisher      pub                     = NULL;

void
usage(
    int         argc,
    char        ** argv)
{
    printf("Usage: %s [options] url\n", argv[0]);
    printf("Default url is "DEFAULT_URL"\n");
    printf("  where options can be:\n");
    printf("   -a,  --application <name>        Application name\n"
           "        --client-label <label>      Set client label\n"
           "   -et, --tx-endpoint <name>        Transmit endpoint name\n"
           "   -er, --rx-endpoint <name>        Receive endpoint name\n"
           "   -h,  --help                      Print this help.\n"
           "   -id, --identifier                Choose instance, eg, \"dshm\"\n"
           "   -p,  --password <string>         Password for realm server.\n"
           "   -t,  --trace <level>             Set trace level\n"
           "           where <level> can be:\n"
           "                 off, severe, warn, info, debug, verbose\n"
           "   -u,  --user <string>             User for realm server.\n");

    exit(1);
}

void
parseArgs(
    int         argc,
    char        **argv)
{
    int     i;

    tibAux_InitGetArgs(argc, argv, usage);

    for (i = 1;  i < argc;  i++) {
        if (tibAux_GetString(&i, "--application", "-a", &appName))
            continue;
        if (tibAux_GetString(&i, "--client-label", "--client-label", &clientLabel))
            continue;
        if (tibAux_GetString(&i, "--tx-endpoint", "-et", &sendEndpointName))
            continue;
        if (tibAux_GetString(&i, "--rx-endpoint", "-er", &receiveEndpointName))
            continue;
        if (tibAux_GetFlag(&i, "--help", "-h"))
           usage(argc, argv);
        if (tibAux_GetString(&i, "--identifier", "-id", &identifier))
            continue;
        if (tibAux_GetString(&i, "--password", "-p", &password))
            continue;
        if (tibAux_GetString(&i, "--trace", "-t", &trcLevel))
            continue;
        if (tibAux_GetString(&i, "--user", "-u", &user))
            continue;
        if (strncmp(argv[i], "-", 1)==0)
        {
            printf("invalid option: %s\n", argv[i]);
            usage(argc, argv);
        }
        realmServer = argv[i];
    }

    printf("Invoked as: ");
    for (i = 0;  i < argc;  i++)
        printf("%s ", argv[i]);
    printf("\n");
}

static void
onDirectBuf(
    tibEx       e,
    tibint64_t  count,
    tibint64_t  size,
    tibint64_t  *sizeList,
    tibint8_t   *incoming,
    void        *closure)
{
    tibint8_t   *buf;

    if ((buf=tibDirectPublisher_Reserve(e, pub, 1, size, NULL)) != NULL)
    {
        memcpy(buf, incoming, size);

        tibDirectPublisher_SendReserved(e, pub);
    }
} // onDirectBuf

int
main(int argc, char** argv)
{
    tibRealm                realm   = NULL;
    tibProperties           props   = NULL;
    tibDirectSubscriber     sub     = NULL;
    tibEx                   ex      = tibEx_Create();

    printf("#\n# %s\n#\n# %s\n#\n", argv[0], tib_Version());

    parseArgs(argc, argv);

    tib_Open(ex, TIB_COMPATIBILITY_VERSION);
    CHECK(ex);

    // Set global trace to specified level
    if (trcLevel != NULL)
    {
        tib_SetLogLevel(ex, trcLevel);
        CHECK(ex);
    }

    // Get a single realm per process
    props = tibProperties_Create(ex);
    tibProperties_SetString(ex, props,  TIB_REALM_PROPERTY_STRING_USERNAME,
                            user);
    tibProperties_SetString(ex, props, TIB_REALM_PROPERTY_STRING_USERPASSWORD,
                            password);
    if (identifier != NULL)
        tibProperties_SetString(ex, props,
                                TIB_REALM_PROPERTY_STRING_APPINSTANCE_IDENTIFIER,
                                identifier);

    // set the client label
    if (clientLabel != NULL)
        tibProperties_SetString(ex, props, TIB_REALM_PROPERTY_STRING_CLIENT_LABEL, clientLabel);
    else
        tibProperties_SetString(ex, props, TIB_REALM_PROPERTY_STRING_CLIENT_LABEL, argv[0]);

    realm = tibRealm_Connect(ex, realmServer, appName, props);
    tibProperties_Destroy(ex, props);
    CHECK(ex);

    sub = tibDirectSubscriber_Create(ex, realm, receiveEndpointName, NULL);
    CHECK(ex);

    pub = tibDirectPublisher_Create(ex, realm, sendEndpointName, NULL);
    CHECK(ex);

    while (tibEx_GetErrorCode(ex) == TIB_OK)
        tibDirectSubscriber_Dispatch(ex, sub, TIB_TIMEOUT_WAIT_FOREVER,
                                     onDirectBuf, NULL);

    CHECK(ex);

    // Clean up
    tibDirectPublisher_Close(ex, pub);
    tibDirectSubscriber_Close(ex, sub);
    tibRealm_Close(ex, realm);
    tibEx_Destroy(ex);

    return 0;
}
