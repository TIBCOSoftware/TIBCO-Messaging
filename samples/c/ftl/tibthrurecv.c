/*
 * Copyright (c) 2010-2018 TIBCO Software Inc.
 * Licensed under a BSD-style license. Refer to [LICENSE]
 * For more information, please contact:
 * TIBCO Software Inc., Palo Alto, California, USA
 */

/*
 * This is a sample C responder program which is used with the tibthrusend
 * program to measure basic throughput between the publisher and subscriber.
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
tibbool_t       inline_mode             = tibfalse;
tibbool_t       doEcho                  = tibtrue;
tibbool_t       doAdvisory              = tibtrue;
const char      *identifier             = NULL;
tibbool_t       trustAll                = tibfalse;
const char      *trustFile              = NULL;
const char      *clientLabel            = NULL;

tibPublisher    pub                     = NULL;
tibFieldRef     dataRef                 = NULL;
tibMessage      reportMsg               = NULL;
volatile tibbool_t  running             = tibtrue;

tibint64_t      start;
tibint64_t      count;
tibint64_t      bytes;
tibAux_StatRecord    blockSizeStats          = {0, 0, 0, 0.0, 0.0};


void
usage(int argc, char** argv)
{
    printf("Usage: %s [options] url\n", argv[0]);
    printf("Default url is "DEFAULT_URL"\n");
    printf("   -a,  --application <name>        Application name\n"
           "        --client-label <label>      Set client label\n"
           "   -d,  --durableName <name>        Durable name for receive side.\n"
           "   -et, --tx-endpoint <name>        Transmit endpoint name\n"
           "   -er, --rx-endpoint <name>        Receive endpoint name\n"
           "   -h,  --help                      Print this help.\n"
           "   -i,  --inline                    Put EventQueue in INLINE mode.\n"
           "   -id, --identifier                Choose instance, eg, \"rdma\"\n"
           "   -n,  --no-echo                   Do not echo messages.\n"
           "   -na, --no-advisory               Do not create a subscriber for dataloss advisories\n"
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
        if (tibAux_GetFlag(&i, "--inline", "-i")) {
            inline_mode = tibtrue;
            continue;
        }
        if (tibAux_GetFlag(&i, "--no-echo", "-n")) {
            doEcho = tibfalse;
            continue;
        }
        if (tibAux_GetFlag(&i, "--no-advisory", "-na")) {
            doAdvisory = tibfalse;
            continue;
        }
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
getResult(char* outbuf, size_t outsize, tibint64_t stop)
{
    double  interval = (stop-start) * tibAux_GetTimerScale();
    tibAux_StringCat(outbuf,outsize,"Received ");
    tibAux_StringCatNumber(outbuf,outsize,(double)count);
    tibAux_StringCat(outbuf,outsize," messages in ");
    tibAux_StringCatNumber(outbuf,outsize,interval);
    tibAux_StringCat(outbuf,outsize," seconds. (");
    tibAux_StringCatNumber(outbuf,outsize,count/interval);
    tibAux_StringCat(outbuf,outsize," messages per second)\n");

    tibAux_StringCat(outbuf,outsize,"Received ");
    tibAux_StringCatNumber(outbuf,outsize,(double)bytes);
    tibAux_StringCat(outbuf,outsize," bytes in ");
    tibAux_StringCatNumber(outbuf,outsize,interval);
    tibAux_StringCat(outbuf,outsize," seconds. (");
    tibAux_StringCatNumber(outbuf,outsize,bytes/interval);
    tibAux_StringCat(outbuf,outsize," bytes per second)\n");
    
    tibAux_StringCat(outbuf,outsize,"Messages per callback: ");
    tibAux_StringCatStats(outbuf,outsize,&blockSizeStats, 1.0);
    tibAux_StringCat(outbuf,outsize,"\n");
}

void
onMsg(
    tibEx                       ex,
    tibEventQueue               msgQueue,
    tibint32_t                  msgNum,
    tibMessage                  *msgs,
    void                        **closures)
{
    int                     i;
    const void*             buf;
    tibint32_t              bufsize;
    tibint32_t              n = msgNum;
    char                    reportBuf[512];

    *reportBuf = '\0';
    
    for (i = 0;  i < msgNum;  i++)
    {
        buf = tibMessage_GetOpaqueByRef(ex, msgs[i], dataRef, &bufsize);
        if (tibEx_GetErrorCode(ex) == TIB_OK)    // There was no error getting the buffer
        {
            switch (*(tibint64_t*)buf)
            {
                case 0:
                    count++;
                    bytes += bufsize;
                    break;
                case -1:
                    start = tibAux_GetTime();
                    count = 0;
                    bytes = 0;
                    blockSizeStats.N = 0;
                    n--;
                    break;
                case -2:
                    getResult(reportBuf, sizeof(reportBuf), tibAux_GetTime());
                    printf("%s\n",reportBuf); fflush(stdout);
                    n--;
                    break;
                case -3:
                    getResult(reportBuf, sizeof(reportBuf), tibAux_GetTime());
                    printf("%s\n",reportBuf); fflush(stdout);

                    //Format and send back the report to the sender
                    tibMessage_SetString(ex, reportMsg, "receiverReport", reportBuf);
                    tibPublisher_Send(ex, pub, reportMsg);

                    *reportBuf = '\0';
                    n--;
                    break;
                case -4:
                    if (doEcho) // Send pong for initial ping.
                        tibPublisher_Send(ex, pub, msgs[i]);
                    n--;
                    break;
                default:
                    if (doEcho)
                        tibPublisher_Send(ex, pub, msgs[i]);
                    count++;
                    bytes += bufsize;
            }
        }
    }
    
    if (tibEx_GetErrorCode(ex) != TIB_OK)
    {
        tibAux_PrintException(ex);
        running = tibfalse;
    }
    
    if (n)
        tibAux_StatUpdate(&blockSizeStats, n);
}

void
onAdvisory(
    tibEx               ex,
    tibEventQueue       msgQueue,
    tibint32_t          msgNum,
    tibMessage          *msgs,
    void                **closures)
{
    tibint32_t          i;

    for (i = 0; i < msgNum; i++)
    {
        printf("received advisory\n");
        tibAux_PrintAdvisory(ex, msgs[i]);
    }

    // any exception that happens in the callback does not
    // propagate to the caller of the tibEventQueue_Dispatch
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
    tibSubscriber           adv     = NULL;
    tibContentMatcher       am      = NULL;
    char                    str[256];
    tibEx                   ex      = tibEx_Create();

    printf("#\n# %s\n#\n# %s\n#\n", argv[0], tib_Version());

    parseArgs(argc, argv);

    tibAux_CalibrateTimer();

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
    
    dataRef = tibFieldRef_Create(ex, TIB_BUILTIN_MSG_FMT_OPAQUE_FIELDNAME);
    reportMsg = tibMessage_Create(ex, realm, "ReportMsgFmt");
    CHECK(ex);

    props = tibProperties_Create(ex);
    tibProperties_SetBoolean(ex, props, TIB_EVENTQUEUE_PROPERTY_BOOL_INLINE_MODE, inline_mode);
    CHECK(ex);
    queue = tibEventQueue_Create(ex, realm, props);
    tibProperties_Destroy(ex, props);
    CHECK(ex);

    if (doAdvisory)
    {
        // Create a data loss advisory matcher
        tibAux_snprintf(str, sizeof(str), "{\"%s\":\"%s\"}", 
                      TIB_ADVISORY_FIELD_NAME, TIB_ADVISORY_NAME_DATALOSS);
        am = tibContentMatcher_Create(ex, realm, str);
        // Create an advisory subscriber
        adv = tibSubscriber_Create(ex, realm, TIB_ADVISORY_ENDPOINT_NAME, am, NULL);
        tibEventQueue_AddSubscriber(ex, queue, adv, onAdvisory, NULL);
    }

    if (durableName && *durableName)
    {
        props = tibProperties_Create(ex);
        tibProperties_SetString(ex, props, TIB_SUBSCRIBER_PROPERTY_STRING_DURABLE_NAME, durableName);
    }
    else
    {
        props = NULL;
    }

    subs = tibSubscriber_Create(ex, realm, recvEndpointName, NULL, props);
    tibProperties_Destroy(ex, props);
    CHECK(ex);
    tibEventQueue_AddSubscriber(ex, queue, subs, onMsg, NULL);
    CHECK(ex);

    pub = tibPublisher_Create(ex, realm, endpointName, NULL);
    CHECK(ex);
    
    while (!tibEx_GetErrorCode(ex) && running)
        tibEventQueue_Dispatch(ex, queue, TIB_TIMEOUT_WAIT_FOREVER);
    CHECK(ex);

    tibPublisher_Close(ex, pub);
    if (queue && subs)
        tibEventQueue_RemoveSubscriber(ex, queue, subs, NULL);
    if (queue && adv)
        tibEventQueue_RemoveSubscriber(ex, queue, adv, NULL);
    tibSubscriber_Close(ex, subs);
    tibSubscriber_Close(ex, adv);
    tibContentMatcher_Destroy(ex, am);
    tibEventQueue_Destroy(ex, queue, NULL);
    tibFieldRef_Destroy(ex, dataRef);
    tibMessage_Destroy(ex, reportMsg);
    tibRealm_Close(ex, realm);
    tib_Close(ex);
    tibEx_Destroy(ex);

    return 0;
}
