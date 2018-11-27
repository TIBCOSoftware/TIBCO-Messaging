/*
 * Copyright (c) 2010-2017 TIBCO Software Inc.
 * Licensed under a BSD-style license. Refer to [LICENSE]
 * For more information, please contact:
 * TIBCO Software Inc., Palo Alto, California, USA
 */

/*
 * This is a sample C responder program which is used with the tibthrusendd
 * program to measure basic throughput between the publisher and subscriber.
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <signal.h>

#ifndef _WIN32
#include <unistd.h>
#endif

#include "tib/ftl.h"

#include "tibaux.h"

#define DEFAULT_URL "http://localhost:8080"

const char*     realmServer             = DEFAULT_URL;
const char*     appName                 = "tibrecv-shared";
const char*     endpointName            = "tiblatrecv-sendendpoint";
const char*     recvEndpointName        = "tibrecv-endpoint";
const char*     durableName             = "tibsendrecv-shared";
const char*     user                    = "guest";
const char*     password                = "guest-pw";
const char      *trcLevel               = NULL;
tibbool_t       inline_mode             = tibfalse;
tibbool_t       doEcho                  = tibtrue;
tibbool_t       doAdvisory              = tibtrue;
const char      *identifier             = NULL;
const char      *clientLabel            = NULL;

tibPublisher    pub                     = NULL;
tibFieldRef     dataRef                 = NULL;
tibMessage      idMsg                   = NULL;
volatile tibbool_t  running             = tibtrue;

tibTimer        contactTimer            = NULL;

tibint64_t      start                   = 0;
tibint64_t      count                   = 0;
tibint64_t      bytes                   = 0;
tibAux_StatRecord    blockSizeStats     = {0, 0, 0, 0.0, 0.0};

tibdouble_t     pingInterval            = 1;
tibdouble_t     reportInterval          = 0;

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
           "   -na, --no-advisory               Do not create a subscriber for dataloss advisories\n"
           "   -p,  --password <string>         Password for realm server.\n"
           "        --ping <float>              Initial ping timeout value (in seconds).\n"
           "   -t,  --trace <level>             Set trace level\n"
           "           where <level> can be:\n"
           "                 off, severe, warn, info, debug, verbose\n"
           "   -u,  --user <string>             User for realm server.\n"
           "   -ri  --report-interval <float>   Interval at which consumer reports will be printed (in seconds).\n");

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
        if (tibAux_GetDouble(&i, "--ping", "--ping", &pingInterval))
            continue;
        if (tibAux_GetDouble(&i, "--report-interval", "-ri", &reportInterval))
            continue;
        if (tibAux_GetFlag(&i, "--inline", "-i")) {
            inline_mode = tibtrue;
            continue;
        }
        if (tibAux_GetFlag(&i, "--no-advisory", "-na")) {
            doAdvisory = tibfalse;
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
    if (start)
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
    else
    {
        tibAux_StringCat(outbuf,outsize,"Nothing happened\n");
    }
}

static void
printLastResult(void)
{
    char             reportBuf[512];

    *reportBuf = '\0';
    getResult(reportBuf, sizeof(reportBuf), tibAux_GetTime());

#ifdef _WIN32
    printf("%s", reportBuf);
#else
    if (fileno(stdout) > -1)
        printf("%s", reportBuf);
#endif
}

void
onReportTimer(
    tibEx            ex,
    tibEventQueue    msgQueue,
    tibTimer         timer,
    void*            closure)
{
    char             reportBuf[512];

    *reportBuf = '\0';
    getResult(reportBuf, sizeof(reportBuf), tibAux_GetTime());

    printf("%s\n",reportBuf);
    fflush(stdout);
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

    for (i = 0;  i < msgNum;  i++)
    {
        buf = tibMessage_GetOpaqueByRef(ex, msgs[i], dataRef, &bufsize);
        if (tibEx_GetErrorCode(ex) == TIB_OK)    // There was no error getting the buffer
        {
            if (contactTimer)
            {
                start = tibAux_GetTime();
                count = 0;
                bytes = 0;
                blockSizeStats.N = 0;

                tibEventQueue_DestroyTimer(ex, msgQueue, contactTimer, NULL);
                contactTimer = NULL;
            }

            switch (*(tibint64_t*)buf)
            {
                case 0:
                    count++;
                    bytes += bufsize;
                    break;

                case 1:
                    tibPublisher_Send(ex, pub, msgs[i]);
                    count++;
                    bytes += bufsize;
                    break;

                default:
                    printf("ERROR: unknown type\n");
                    fflush(stdout);

                    running = tibfalse;
                    break;
            }
        }
    }

    if (tibEx_GetErrorCode(ex) != TIB_OK)
    {
        tibAux_PrintException(ex);
        running = tibfalse;
    }

    tibAux_StatUpdate(&blockSizeStats, msgNum);
}

void
onContactTimer(
    tibEx            ex,
    tibEventQueue    msgQueue,
    tibTimer         timer,
    void*            closure)
{
    tibPublisher_Send(ex, pub, idMsg);

    // any exception that happens in the callback does not
    // propagate to the caller of the tibEventQueue_Dispatch
    if (tibEx_GetErrorCode(ex) != TIB_OK)
        tibAux_PrintException(ex);
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
    tibTimer                reportTimer = NULL;
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

    // set the client label
    if (clientLabel != NULL)
        tibProperties_SetString(ex, props, TIB_REALM_PROPERTY_STRING_CLIENT_LABEL, clientLabel);
    else
        tibProperties_SetString(ex, props, TIB_REALM_PROPERTY_STRING_CLIENT_LABEL, argv[0]);

    realm = tibRealm_Connect(ex, realmServer, appName, props);
    tibProperties_Destroy(ex, props);
    CHECK(ex);
    
    dataRef = tibFieldRef_Create(ex, TIB_BUILTIN_MSG_FMT_OPAQUE_FIELDNAME);
    idMsg   = tibMessage_Create(ex, realm, "ReportMsgFmt");

    // if two instances of tibthrurecv start at exactly the same time
    // tibthrusend wont't be able to tell them apart.
    tibAux_snprintf(str, sizeof(str), "%"auxPRId64"-%s", tibAux_GetTime(), durableName);

    printf("picked durable id:%s\n", str);
    fflush(stdout);

    tibMessage_SetString(ex, idMsg, "receiverId", str);
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

    props = tibProperties_Create(ex);
    if (durableName && *durableName)
        tibProperties_SetString(ex, props, TIB_SUBSCRIBER_PROPERTY_STRING_DURABLE_NAME, durableName);

    subs = tibSubscriber_Create(ex, realm, recvEndpointName, NULL, props);
    tibProperties_Destroy(ex, props);
    CHECK(ex);
    tibEventQueue_AddSubscriber(ex, queue, subs, onMsg, NULL);
    CHECK(ex);

    pub = tibPublisher_Create(ex, realm, endpointName, NULL);
    CHECK(ex);
    
    contactTimer = tibEventQueue_CreateTimer(ex, queue, pingInterval, onContactTimer, NULL);

    if (reportInterval)
        reportTimer = tibEventQueue_CreateTimer(ex, queue, reportInterval, onReportTimer, NULL);

    while (!tibEx_GetErrorCode(ex) && running)
        tibEventQueue_Dispatch(ex, queue, TIB_TIMEOUT_WAIT_FOREVER);
    CHECK(ex);

    printLastResult();

    tibPublisher_Close(ex, pub);
    if (queue && subs)
        tibEventQueue_RemoveSubscriber(ex, queue, subs, NULL);
    if (queue && adv)
        tibEventQueue_RemoveSubscriber(ex, queue, adv, NULL);
    if (queue && contactTimer)
        tibEventQueue_DestroyTimer(ex, queue, contactTimer, NULL);
    if (queue && reportTimer)
        tibEventQueue_DestroyTimer(ex, queue, reportTimer, NULL);
    tibSubscriber_Close(ex, subs);
    tibSubscriber_Close(ex, adv);
    tibContentMatcher_Destroy(ex, am);
    tibEventQueue_Destroy(ex, queue, NULL);
    tibFieldRef_Destroy(ex, dataRef);
    tibMessage_Destroy(ex, idMsg);
    tibRealm_Close(ex, realm);
    tib_Close(ex);
    tibEx_Destroy(ex);

    return 0;
}
