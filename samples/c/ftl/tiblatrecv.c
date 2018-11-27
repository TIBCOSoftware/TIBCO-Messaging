/*
 * Copyright (c) 2010-2017 TIBCO Software Inc.
 * Licensed under a BSD-style license. Refer to [LICENSE]
 * For more information, please contact:
 * TIBCO Software Inc., Palo Alto, California, USA
 */

/*
 * This is a sample C responder program which is used with the tiblatsend
 * program to measure basic latency between the publisher and subscriber.
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "tib/ftl.h"

#include "tibaux.h"

#define DEFAULT_URL "http://localhost:8080"

const char*     realmServer             = DEFAULT_URL;
const char*     appName                 = "tiblatrecv";
const char*     endpointName            = "tiblatrecv-sendendpoint";
const char*     recvEndpointName        = "tiblatrecv-recvendpoint";
const char*     durableName             = NULL;
const char*     user                    = "guest";
const char*     password                = "guest-pw";
const char      *trcLevel               = NULL;
const char      *identifier             = NULL;
tibbool_t       trustAll                = tibfalse;
const char      *trustFile              = NULL;
const char      *clientLabel            = NULL;

tibPublisher    pub                     = NULL;
tibbool_t       running                 = tibtrue;

void
usage(int argc, char** argv)
{
    printf("Usage: %s [options] url\n", argv[0]);
    printf("Default url is "DEFAULT_URL"\n");
    printf("  where options can be:\n");
    printf("   -a,  --application <name>        Application name\n"
           "        --client-label <label>      Set client label\n"
           "   -d,  --durableName <name>        Durable name for receive side.\n"
           "   -et, --tx-endpoint <name>        Transmit endpoint name\n"
           "   -er, --rx-endpoint <name>        Receive endpoint name\n"
           "   -h,  --help                      Print this help.\n"
           "   -id, --identifier                Choose instance, eg, \"rdma\"\n"
           "   -p,  --password <string>         Password for realm server.\n"
           "   -t,  --trace <level>             Set trace level\n"
           "           where <level> can be:\n"
           "                 off, severe, warn, info, debug, verbose\n"
           "   -u,  --user <string>             User for realm server.\n"
           "        --trustall\n"
           "        --trustfile <file>\n");

    exit(1);
}

void
parseArgs(int argc, char** argv)
{
    int     i;

    tibAux_InitGetArgs(argc, argv, usage);

    for (i = 1;  i < argc;  i++) {
        if (tibAux_GetString(&i, "--application", "-a", &appName))
            continue;
        if (tibAux_GetString(&i, "--client-label", "--client-label", &clientLabel))
            continue;
        if (tibAux_GetString(&i, "--tx-endpoint", "-et", &endpointName))
            continue;
        if (tibAux_GetString(&i, "--rx-endpoint", "-er", &recvEndpointName))
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
        if (tibAux_GetString(&i, "--durableName", "-d", &durableName))
            continue;
        if (tibAux_GetFlag(&i, "--trustall", "--trustall"))
        {
            if (trustFile)
            {
                printf("cannot specify both --trustall and --trustfile\n");
                usage(argc, argv);
            }
            trustAll = tibtrue;
            continue;
        }
        if (tibAux_GetString(&i, "--trustfile", "--trustfile", &trustFile))
        {
            if (trustAll)
            {
                printf("cannot specify both --trustall and --trustfile\n");
                usage(argc, argv);
            }
            continue;
        }
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

void
onMsg(
    tibEx                       ex,
    tibEventQueue               msgQueue,
    tibint32_t                  msgNum,
    tibMessage                  *msgs,
    void                        **closures)
{
    if (msgNum != 1) {
        printf("Received %d messages instead of one.\n", msgNum);
        running = tibfalse;
    }
    
    tibPublisher_Send(ex, pub, msgs[0]);

    if (tibEx_GetErrorCode(ex) != TIB_OK)
        tibAux_PrintException(ex);
}

int
main(int argc, char** argv)
{
    tibRealm                realm   = NULL;
    tibEventQueue           queue   = NULL;
    tibProperties           props   = NULL;
    tibSubscriber           subs    = NULL;
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
    tibProperties_SetString(ex, props,
                            TIB_REALM_PROPERTY_STRING_USERNAME, user);
    tibProperties_SetString(ex, props,
                            TIB_REALM_PROPERTY_STRING_USERPASSWORD, password);
    if (identifier)
        tibProperties_SetString(ex, props,
                                TIB_REALM_PROPERTY_STRING_APPINSTANCE_IDENTIFIER, identifier);
    if (trustAll)
        tibProperties_SetLong(ex, props, TIB_REALM_PROPERTY_LONG_TRUST_TYPE,
                              TIB_REALM_HTTPS_CONNECTION_TRUST_EVERYONE);
    if (trustFile)
    {
        tibProperties_SetLong(ex, props, TIB_REALM_PROPERTY_LONG_TRUST_TYPE,
                TIB_REALM_HTTPS_CONNECTION_USE_SPECIFIED_TRUST_FILE);
        tibProperties_SetString(ex, props, TIB_REALM_PROPERTY_STRING_TRUST_FILE, trustFile);
    }

    // set the client label
    if (clientLabel != NULL)
        tibProperties_SetString(ex, props, TIB_REALM_PROPERTY_STRING_CLIENT_LABEL, clientLabel);
    else
        tibProperties_SetString(ex, props, TIB_REALM_PROPERTY_STRING_CLIENT_LABEL, argv[0]);

    realm = tibRealm_Connect(ex, realmServer, appName, props);
    tibProperties_Destroy(ex, props);
    CHECK(ex);

    props = tibProperties_Create(ex);
    tibProperties_SetBoolean(ex, props, TIB_EVENTQUEUE_PROPERTY_BOOL_INLINE_MODE, tibtrue);
    CHECK(ex);
    queue = tibEventQueue_Create(ex, realm, props);
    tibProperties_Destroy(ex, props);
    CHECK(ex);

    props = tibProperties_Create(ex);
    if (durableName && *durableName)
        tibProperties_SetString(ex, props, TIB_SUBSCRIBER_PROPERTY_STRING_DURABLE_NAME, durableName);
    CHECK(ex);
    subs = tibSubscriber_Create(ex, realm, recvEndpointName, NULL, props);
    tibProperties_Destroy(ex, props);

    tibEventQueue_AddSubscriber(ex, queue, subs, onMsg, NULL);

    CHECK(ex);

    pub = tibPublisher_Create(ex, realm, endpointName, NULL);
    CHECK(ex);
    
    while (!tibEx_GetErrorCode(ex) && running)
        tibEventQueue_Dispatch(ex, queue, TIB_TIMEOUT_WAIT_FOREVER);
    CHECK(ex);

    // Clean up
    tibPublisher_Close(ex, pub);
    if (queue && subs)
        tibEventQueue_RemoveSubscriber(ex, queue, subs, NULL);
    tibSubscriber_Close(ex, subs);
    tibEventQueue_Destroy(ex, queue, NULL);
    tibRealm_Close(ex, realm);
    tibEx_Destroy(ex);

    return 0;
}
