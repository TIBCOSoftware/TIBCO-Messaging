/*
 * Copyright (c) 2010-2017 TIBCO Software Inc.
 * Licensed under a BSD-style license. Refer to [LICENSE]
 * For more information, please contact:
 * TIBCO Software Inc., Palo Alto, California, USA
 */

/*
 * This is a sample C program which can be used (with the tibthrurecv program)
 * to measure basic throughput between the publisher and subscriber.
 *
 * NOTE: This application is multi-threaded using a busy wait in senderThreadFunction,
 *       and should not be constrained to a single logical processor.
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>

#include "tib/ftl.h"

#include "tibaux.h"

#define DEFAULT_URL "http://localhost:8080"

const char*     realmServer             = DEFAULT_URL;
const char*     appName                 = "tiblatsend";
const char*     endpointName            = "tiblatsend-sendendpoint";
const char*     recvEndpointName        = "tiblatsend-recvendpoint";
const char*     durableName             = NULL;
tibint32_t      messageCount            = 5000000;
tibint32_t      payloadSize             = 16;
tibint32_t      warmupCount             = 100;
tibint32_t      batchSize               = 100;
const char      *trcLevel               = NULL;
const char      *user                   = "guest";
const char      *password               = "guest-pw";
tibbool_t       inline_mode             = tibfalse;
tibbool_t       report_mode             = tibtrue;
const char      *identifier             = NULL;
tibbool_t       singleSend              = tibfalse;
tibdouble_t     pingInterval            = 1.0;
volatile tibTimer pongTimer             = NULL;
tibbool_t       trustAll                = tibfalse;
const char      *trustFile              = NULL;
const char      *clientLabel            = NULL;

void*           payload                 = NULL;
volatile tibbool_t running              = tibtrue;
volatile tibbool_t received             = tibfalse;
volatile tibbool_t reportReceived       = tibfalse;
char            reportBuf[512];
tibint32_t      currentLimit            = 0;
tibint32_t      numSamples              = 0;
tibMessage*     sendMsgs                = NULL;
tibMessage      signalMsg               = NULL;
tibPublisher    pub                     = NULL;
tibSubscriber   sub                     = NULL;
tibSubscriber   upAdvisorySub           = NULL;
tibSubscriber   downAdvisorySub         = NULL;
tibFieldRef     dataRef                 = NULL;
tibEventQueue   queue                   = NULL;
tibint64_t      sendStartTime           = 0;
tibint64_t      sendStopTime            = 0;
volatile tibbool_t storeAvailable       = tibtrue;

void
usage(int argc, char** argv)
{
    printf("Usage: %s [options] url\n", argv[0]);
    printf("Default url is "DEFAULT_URL"\n");
    printf("   -a,  --application <name>        Application name\n"
           "   -b,  --batchsize <int>           Batch size\n"
           "        --client-label <label>      Set client label\n"
           "   -c,  --count <int>               Send n messages.\n"
           "   -d,  --durable                   Durable name for receive side.\n"
           "   -et, --tx-endpoint <name>        Transmit endpoint name\n"
           "   -er, --rx-endpoint <name>        Receive endpoint name\n"
           "   -h,  --help                      Print this help.\n"
           "   -i,  --inline                    Put EventQueue in INLINE mode.\n"
           "   -id, --identifier                Choose instance, eg, \"rdma\"\n"
           "   -p,  --password <string>         Password for realm server.\n"
           "        --ping <float>              Initial ping timeout value (in seconds).\n"
           "   -s,  --size <int>                Send payload of n bytes.\n"
           "   -t,  --trace <level>             Set trace level\n"
           "           where <level> can be:\n"
           "                 off, severe, warn, info, debug, verbose\n"
           "   -u,  --user <string>             User for realm server.\n"
           "   -w,  --warmup <int>              Warmup for n batches (Default 100)\n"
           "        --single                    Use multiple calls to Send instead of one call to SendMessages.\n"
           "        --trustall\n"
           "        --trustfile <file>\n");

    exit(1);
}

void
parseArgs(int argc, char** argv)
{
    int     i;

    tibAux_InitGetArgs(argc, argv, usage);

    for (i = 1;  i < argc;  i++)
    {
        if (tibAux_GetString(&i, "--application", "-a", &appName))
            continue;
        if (tibAux_GetString(&i, "--client-label", "--client-label", &clientLabel))
            continue;
        if (tibAux_GetInt(&i ,"--count", "-c", &messageCount))
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
        if (tibAux_GetDouble(&i, "--ping", "--ping", &pingInterval))
            continue;
        if (tibAux_GetInt(&i, "--size", "-s", &payloadSize))
            continue;
        if (tibAux_GetString(&i, "--trace", "-t", &trcLevel))
            continue;
        if (tibAux_GetString(&i, "--user", "-u", &user))
            continue;
        if (tibAux_GetInt(&i, "--warmup", "-w", &warmupCount))
            continue;
        if (tibAux_GetInt(&i, "--batchsize", "-b", &batchSize))
            continue;
        if (tibAux_GetString(&i, "--durable", "-d", &durableName))
            continue;
        if (tibAux_GetFlag(&i, "--inline", "-i")) {
            inline_mode = tibtrue;
            continue;
        }
        if (tibAux_GetFlag(&i, "--no-receiver-report", "-norpt")) {
            report_mode = tibfalse;
            continue;
        }
        if (tibAux_GetFlag(&i, "--single", "--single"))
        {
            singleSend = tibtrue;
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

    if (payloadSize < sizeof(tibint64_t))
    {
        printf("Payload size must be at least %d\n", (int)sizeof(tibint64_t));
        exit(1);
    }

    if (batchSize > messageCount)
    {
        printf("Count must be greater than or equal to batch size\n");
        exit(1);
    }

    printf("Invoked as: ");
    for (i = 0;  i < argc;  i++)
        printf("%s ", argv[i]);
    printf("\n");
}

void
protectedSend(
    tibEx           e,
    tibPublisher    id,
    tibMessage      msg)
{
    tibint64_t  stopTime;

    tibPublisher_Send(e, pub, msg);
    if (tibEx_GetErrorCode(e) == TIB_OK)
        return;

    // If send failed, we'll keep retrying for 60 seconds.
    stopTime = tibAux_GetTime() + (tibint64_t)(60.0 / tibAux_GetTimerScale());
    do
    {
        // Try 10 times per second.
        tibAux_SleepMillis(100);

        // Don't bother trying if we're using a tibstore and it is unavailable.
        if (storeAvailable)
        {
            tibEx_Clear(e);
            tibPublisher_Send(e, pub, msg);
        }

    } while (tibEx_GetErrorCode(e) != TIB_OK && tibAux_GetTime() < stopTime );
}

void
protectedSendMessages(
    tibEx           e,
    tibPublisher    id,
    tibint32_t      msgNum,
    tibMessage      *msgs)
{
    tibint64_t  stopTime;

    tibPublisher_SendMessages(e, id, msgNum, msgs);
    if (tibEx_GetErrorCode(e) == TIB_OK)
        return;

    // If send failed, we'll keep retrying for 60 seconds.
    stopTime = tibAux_GetTime() + (tibint64_t)(60.0 / tibAux_GetTimerScale());
    do
    {
        // Try 10 times per second.
        tibAux_SleepMillis(100);

        // Don't bother trying if we're using a tibstore and it is unavailable.
        if (storeAvailable)
        {
            tibEx_Clear(e);
            tibPublisher_SendMessages(e, id, msgNum, msgs);
        }

    } while (tibEx_GetErrorCode(e) != TIB_OK && tibAux_GetTime() < stopTime );
}

void
sendBatch(
    tibEx ex)
{
    tibint32_t              i;

    if (singleSend)
    {
        for (i = 0;  i < batchSize;  i++)
        {
            protectedSend(ex, pub, sendMsgs[i]);
        }
    }
    else
    {
        protectedSendMessages(ex, pub, batchSize, sendMsgs);
    }
    CHECK(ex);
}

void
senderThreadFunction(
    void*               arg)
{
    tibEx                   ex      = tibEx_Create();
    tibint64_t              signalNum;
    tibint64_t              batchNum;
    double                  sendTime;

    if (pongTimer)
    {
        // Send initial ping.
        received = tibfalse;
        signalNum = -4;
        tibMessage_SetOpaqueByRef(ex, signalMsg, dataRef,
                             &signalNum, sizeof(signalNum));
        protectedSend(ex, pub, signalMsg);
        CHECK(ex);
        while (running && !received)
        {
            TIBAUX_PAUSE();
        }
    }

    for (batchNum = 0;  batchNum < warmupCount;  batchNum++)
    {
        while (running && !received)
        {
            TIBAUX_PAUSE();
        }
        received = tibfalse;
        sendBatch(ex);
    }

    signalNum = -1;
    tibMessage_SetOpaqueByRef(ex, signalMsg, dataRef,
                         &signalNum, sizeof(signalNum));
    protectedSend(ex, pub, signalMsg);
    CHECK(ex);

    sendStartTime = tibAux_GetTime();
    for (batchNum = 0;  batchNum < currentLimit;  batchNum++)
    {
        while (running && !received) 
        {
            TIBAUX_PAUSE();
            
            // Note: a thread yield is required here 
            //       for single-processor constrained use.
        }
        received = tibfalse;
        sendBatch(ex);
    }
    sendStopTime = tibAux_GetTime();

    if (report_mode)
        signalNum = -3;
    else
        signalNum = -2;
    tibMessage_SetOpaqueByRef(ex, signalMsg, dataRef,
                               &signalNum, sizeof(signalNum));
    protectedSend(ex, pub, signalMsg);

    sendTime = (sendStopTime-sendStartTime)*tibAux_GetTimerScale();
    printf("Sent ");
    tibAux_PrintNumber(batchSize * numSamples);
    printf(" messages in ");
    tibAux_PrintNumber(sendTime);
    printf(" seconds. (");
    tibAux_PrintNumber((batchSize * numSamples)/sendTime);
    printf(" msgs/sec)\n");

    if (report_mode)
        while (running && !reportReceived) { /* wait for receiver report */ }

    CHECK(ex);

    running = tibfalse;

    tibEventQueue_RemoveSubscriber(ex, queue, sub, NULL);
    tibEventQueue_RemoveSubscriber(ex, queue, upAdvisorySub, NULL);
    tibEventQueue_RemoveSubscriber(ex, queue, downAdvisorySub, NULL);
    tibEventQueue_Destroy(ex, queue, NULL);
    CHECK(ex);

    tibEx_Destroy(ex);
}

static void
onPongTimeout(
    tibEx            e,
    tibEventQueue    msgQueue,
    tibTimer         timer,
    void*            closure)
{
    tibMessage       ping = (tibMessage)closure;

    // Resend the initial ping message if we haven't received a pong.
    if (pongTimer)
    {
        if (trcLevel)
            printf("Resending initial message\n");
        protectedSend(e, pub, ping);
    }
}

void
onUpAdvisory(
    tibEx               e,
    tibEventQueue       msgQueue,
    tibint32_t          msgNum,
    tibMessage          *msgs,
    void                **closures)
{
    storeAvailable = tibtrue;
    printf("Setting store availability to true.\n");
}

void
onDownAdvisory(
    tibEx               e,
    tibEventQueue       msgQueue,
    tibint32_t          msgNum,
    tibMessage          *msgs,
    void                **closures)
{
    storeAvailable = tibfalse;
    printf("Setting store availability to false.\n");
}

void
onMsg(
    tibEx               e,
    tibEventQueue       msgQueue,
    tibint32_t          msgNum,
    tibMessage          *msgs,
    void                **closures)
{
    const char*         recvReport;
    int                 i;

    received = tibtrue;

    for (i = 0;  i < msgNum;  i++)
    {
        if (tibMessage_IsFieldSet(e,msgs[i],"receiverReport") )
        {
            recvReport = tibMessage_GetString(e, msgs[i], "receiverReport");
            tibAux_StringCopy(reportBuf,sizeof(reportBuf),recvReport);
            reportReceived = tibtrue;
        }
    }

    if (reportReceived == tibfalse && msgNum != 1) {
        printf("Received %d messages instead of one.\n", msgNum);
        running = tibfalse;
    }
}

static void
onSubRemoved(
    tibEx            e,
    tibSubscriber    rsub,
    void*            closure)
{
    // Subscriber has been successfully removed from the queue.
    // Now add it back with the callback to be used for the actual test.
    tibEventQueue_AddSubscriber(e, queue, rsub, onMsg, NULL);
    CHECK(e);

    // Resume message sequence.
    received = tibtrue;
}

void
onPongMsg(
    tibEx               e,
    tibEventQueue       msgQueue,
    tibint32_t          msgNum,
    tibMessage          *msgs,
    void                **closures)
{
    if (pongTimer)
    {
        tibEventQueue_DestroyTimer(e, queue, pongTimer, NULL);
        pongTimer = NULL;
    }

    // Change our subscription before starting the real test interval.
    tibEventQueue_RemoveSubscriber(e, queue, sub, onSubRemoved);
    CHECK(e);

    if (msgNum != 1) {
        printf("Received %d messages instead of one.\n", msgNum);
        running = tibfalse;
    }
}

int
main(int argc, char** argv)
{
    tibRealm                realm   = NULL;
    tibProperties           props   = NULL;
    tibint32_t              i;
    tibEx                   ex      = tibEx_Create();
    tibAux_ThreadHandle     senderThread;
    tibContentMatcher       upAdvisoryMatcher;
    tibContentMatcher       downAdvisoryMatcher;

    printf("#\n# %s\n#\n# %s\n#\n", argv[0], tib_Version());

    parseArgs(argc, argv);
    
    tibAux_CalibrateTimer();

    numSamples = messageCount / batchSize;

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
    if (clientLabel != NULL)
        tibProperties_SetString(ex, props, TIB_REALM_PROPERTY_STRING_CLIENT_LABEL, clientLabel);
    else
        tibProperties_SetString(ex, props, TIB_REALM_PROPERTY_STRING_CLIENT_LABEL, argv[0]);

    realm = tibRealm_Connect(ex, realmServer, appName, props);
    tibProperties_Destroy(ex, props);
    CHECK(ex);

    dataRef = tibFieldRef_Create(ex, TIB_BUILTIN_MSG_FMT_OPAQUE_FIELDNAME);
    CHECK(ex);

    props = tibProperties_Create(ex);
    tibProperties_SetBoolean(ex, props, TIB_EVENTQUEUE_PROPERTY_BOOL_INLINE_MODE, inline_mode);
    CHECK(ex);
    queue = tibEventQueue_Create(ex, realm, props);
    tibProperties_Destroy(ex, props);
    CHECK(ex);

    if (durableName && *durableName)
    {
        props = tibProperties_Create(ex);
        tibProperties_SetString(ex, props, TIB_SUBSCRIBER_PROPERTY_STRING_DURABLE_NAME, durableName);
    }
    else
    {
        props = NULL;
    }
    CHECK(ex);
    sub = tibSubscriber_Create(ex, realm, recvEndpointName, NULL, props);
    tibProperties_Destroy(ex, props);
    CHECK(ex);
    if (pingInterval)
        tibEventQueue_AddSubscriber(ex, queue, sub, onPongMsg, NULL);
    else
        tibEventQueue_AddSubscriber(ex, queue, sub, onMsg, NULL);
    CHECK(ex);

    upAdvisoryMatcher = tibContentMatcher_Create(ex, realm,
            "{\"" TIB_ADVISORY_FIELD_ADVISORY "\":1,"
            " \"" TIB_ADVISORY_FIELD_NAME "\":\"" TIB_ADVISORY_NAME_RESOURCE_AVAILABLE "\","
            " \"" TIB_ADVISORY_FIELD_REASON "\":\"" TIB_ADVISORY_REASON_PERSISTENCE_STORE_AVAILABLE "\"}");
    upAdvisorySub = tibSubscriber_Create(ex, realm, TIB_ADVISORY_ENDPOINT_NAME, upAdvisoryMatcher, NULL);
    tibContentMatcher_Destroy(ex, upAdvisoryMatcher);
    CHECK(ex);
    tibEventQueue_AddSubscriber(ex, queue, upAdvisorySub, onUpAdvisory, NULL);
    CHECK(ex);

    downAdvisoryMatcher = tibContentMatcher_Create(ex, realm,
            "{\"" TIB_ADVISORY_FIELD_ADVISORY "\":1,"
            " \"" TIB_ADVISORY_FIELD_NAME "\":\"" TIB_ADVISORY_NAME_RESOURCE_UNAVAILABLE "\","
            " \"" TIB_ADVISORY_FIELD_REASON "\":\"" TIB_ADVISORY_REASON_PERSISTENCE_STORE_UNAVAILABLE "\"}");
    downAdvisorySub = tibSubscriber_Create(ex, realm, TIB_ADVISORY_ENDPOINT_NAME, downAdvisoryMatcher, NULL);
    tibContentMatcher_Destroy(ex, downAdvisoryMatcher);
    CHECK(ex);
    tibEventQueue_AddSubscriber(ex, queue, downAdvisorySub, onDownAdvisory, NULL);
    CHECK(ex);

    pub = tibPublisher_Create(ex, realm, endpointName, NULL);
    CHECK(ex);

    payload = calloc(payloadSize, 1);
    sendMsgs = calloc(batchSize, sizeof(tibMessage));
    for (i = 0;  i < batchSize;  i++)
    {
        sendMsgs[i] = tibMessage_Create(ex, realm, TIB_BUILTIN_MSG_FMT_OPAQUE);
        tibMessage_SetOpaqueByRef(ex, sendMsgs[i], dataRef, 
                             payload, payloadSize);
    }
    *(tibint64_t*)payload = 1;
    tibMessage_SetOpaqueByRef(ex, sendMsgs[0], dataRef,
                              payload, payloadSize);
    CHECK(ex);
    
    signalMsg = tibMessage_Create(ex, realm, TIB_BUILTIN_MSG_FMT_OPAQUE);
    
    printf("\nSender Report:\n");
    printf("Requested %d messages.\n", messageCount);
    printf("Sending %d messages (%d batches of %d) with payload size %d\n", 
           batchSize * numSamples,
           numSamples, batchSize, payloadSize);
    fflush(stdout);

    currentLimit = numSamples;
    received = tibtrue;

    if (pingInterval)
        pongTimer = tibEventQueue_CreateTimer(ex, queue, pingInterval,
                                              onPongTimeout, signalMsg);

    senderThread = tibAux_LaunchThread(senderThreadFunction, NULL);
    while (tibEx_GetErrorCode(ex) == TIB_OK && running)
        tibEventQueue_Dispatch(ex, queue, TIB_TIMEOUT_WAIT_FOREVER);
    if (tibEx_GetErrorCode(ex) == TIB_INVALID_ARG)
        // Queue was destroyed in sender thread.
        tibEx_Clear(ex);
    else
        CHECK(ex);

    tibAux_JoinThread(senderThread);
    
    if(report_mode == tibtrue)
    {
        printf("\nReceiver Report:\n%s\n",reportBuf);
    }

    fflush(stdout);

    tibSubscriber_Close(ex, sub);
    tibSubscriber_Close(ex, upAdvisorySub);
    tibSubscriber_Close(ex, downAdvisorySub);
    tibPublisher_Close(ex, pub);

    if (sendMsgs)
    {
        for (i = 0;  i < batchSize;  i++)
            tibMessage_Destroy(ex, sendMsgs[i]);
        free(sendMsgs);
    }
    
    tibMessage_Destroy(ex, signalMsg);
    free(payload);
    tibFieldRef_Destroy(ex, dataRef);
    tibRealm_Close(ex, realm);
    tib_Close(ex);
    CHECK(ex);
    
    tibEx_Destroy(ex);
    
    return 0;
}
