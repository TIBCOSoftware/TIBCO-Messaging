/*
 * Copyright (c) 2010-2017 TIBCO Software Inc.
 * Licensed under a BSD-style license. Refer to [LICENSE]
 * For more information, please contact:
 * TIBCO Software Inc., Palo Alto, California, USA
 */

/*
 * This is a sample C program which can be used (with the tibthrurecvd program)
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
const char*     appName                 = "tibsend-shared";
const char*     endpointName            = "tibsend-endpoint";
const char*     recvEndpointName        = "tiblatsend-recvendpoint";
const char*     durableName             = NULL;
tibint32_t      messageCount            = 5000000;
tibint32_t      payloadSize             = 16;
tibint32_t      warmupCount             = 100;
tibint32_t      batchSize               = 100;
tibint32_t      subscriberCount         = 1;
const char      *trcLevel               = NULL;
const char      *user                   = "guest";
const char      *password               = "guest-pw";
tibbool_t       inline_mode             = tibfalse;
const char      *identifier             = NULL;
tibbool_t       singleSend              = tibfalse;
const char      *clientLabel            = NULL;

void*               payload             = NULL;
volatile tibint32_t distinctSubscribers = 0;
volatile tibbool_t  running             = tibtrue;
volatile tibint32_t receiveCnt          = 0;
tibAux_Mutex        receiveCntMutex;

char            reportBuf[512];
tibint32_t      currentLimit            = 0;
tibint32_t      numSamples              = 0;
tibMessage*     sendMsgs                = NULL;
tibPublisher    dataPub                 = NULL;
tibSubscriber   sub                     = NULL;
tibSubscriber   upAdvisorySub           = NULL;
tibSubscriber   downAdvisorySub         = NULL;
tibEventQueue   queue                   = NULL;

volatile tibbool_t storeAvailable       = tibtrue;

char*           subscriberHayStack      = NULL;
tibint32_t      hayStackSize            = 0;

#define MAX_SUBSCRIBER_ID               (256)

void
usage(int argc, char** argv)
{
    printf("Usage: %s [options] url\n", argv[0]);
    printf("Default url is "DEFAULT_URL"\n");
    printf("   -a,  --application <name>        Application name\n"
           "   -b,  --batchsize <int>           Batch size\n"
           "        --client-label <label>      Set client label\n"
           "   -c,  --count <int>               Send n messages.\n"
           "   -d,  --durableName <name>        Durable name for receive side.\n"
           "   -et, --tx-endpoint <name>        Transmit endpoint name\n"
           "   -er, --rx-endpoint <name>        Receive endpoint name\n"
           "   -h,  --help                      Print this help.\n"
           "   -i,  --inline                    Put EventQueue in INLINE mode.\n"
           "   -id, --identifier                Choose instance, eg, \"rdma\"\n"
           "   -p,  --password <string>         Password for realm server.\n"
           "        --ping <float>              Initial ping timeout value (in seconds).\n"
           "   -s,  --size <int>                Send payload of n bytes.\n"
           "   -sc  --subscriber-count <int>    Number of subscribers. (Default 1)\n"
           "   -t,  --trace <level>             Set trace level\n"
           "           where <level> can be:\n"
           "                 off, severe, warn, info, debug, verbose\n"
           "   -u,  --user <string>             User for realm server.\n"
           "   -w,  --warmup <int>              Warmup for n batches (Default 100)\n"
           "        --single                    Use multiple calls to Send instead of one call to SendMessages.\n");

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
        if (tibAux_GetString(&i, "--durableName", "-d", &durableName))
            continue;
        if (tibAux_GetInt(&i, "--subscriber-count", "-sc", &subscriberCount))
            continue;
        if (tibAux_GetFlag(&i, "--inline", "-i")) {
            inline_mode = tibtrue;
            continue;
        }
        if (tibAux_GetFlag(&i, "--single", "--single"))
        {
            singleSend = tibtrue;
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

    if (warmupCount < subscriberCount)
    {
        printf("Count must be greater than or equal to subscriberCount\n");
        exit(1);
    }

    if (messageCount/batchSize < subscriberCount)
    {
        printf("Number of batches to be sent must be greater than or equal to subscriberCount\n");
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

    tibPublisher_Send(e, dataPub, msg);
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
            tibPublisher_Send(e, dataPub, msg);
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
            protectedSend(ex, dataPub, sendMsgs[i]);
        }
    }
    else
    {
        protectedSendMessages(ex, dataPub, batchSize, sendMsgs);
    }
    CHECK(ex);
}

void
senderThreadFunction(
    void*               arg)
{
    tibEx                   ex      = tibEx_Create();
    tibint64_t              batchNum;
    tibint64_t              sendStartTime           = 0;
    tibint64_t              sendStopTime            = 0;
    tibint64_t              startBatchNum           = 0;

    for (batchNum = 0;  batchNum < currentLimit;  batchNum++)
    {
        while (running && !receiveCnt)
        {
            TIBAUX_PAUSE();
        }

        if (!sendStartTime && distinctSubscribers == subscriberCount)
        {
            // defer actual start a bit
            --warmupCount;

            if (warmupCount<0)
            {
                startBatchNum = batchNum;
                sendStartTime = tibAux_GetTime();

                printf("All subscribers are up and running\n");
            }
        }

        tibAux_Mutex_Lock(receiveCntMutex);
        receiveCnt--;
        tibAux_Mutex_Unlock(receiveCntMutex);

        sendBatch(ex);
    }

    sendStopTime = tibAux_GetTime();

    if (sendStartTime)
    {
        tibint64_t allSubscriberBatches = (batchNum - startBatchNum);
        double     sendTime             = (sendStopTime-sendStartTime)*tibAux_GetTimerScale();
        double     totalConsumed        = (double)(batchSize * allSubscriberBatches);

        // Due to end to end flow control send time is sufficient for throughput numbers

        printf("Sent ");
        tibAux_PrintNumber(totalConsumed);
        printf(" messages in ");
        tibAux_PrintNumber(sendTime);
        printf(" seconds. (");
        tibAux_PrintNumber(totalConsumed/sendTime);
        printf(" msgs/sec)\n");
    }
    else
    {
       printf("No contact with enough distinct subscribers\n");
    }

    fflush(stdout);

    CHECK(ex);

    running = tibfalse;

    tibEventQueue_RemoveSubscriber(ex, queue, sub, NULL);
    tibEventQueue_RemoveSubscriber(ex, queue, upAdvisorySub, NULL);
    tibEventQueue_RemoveSubscriber(ex, queue, downAdvisorySub, NULL);
    tibEventQueue_Destroy(ex, queue, NULL);
    CHECK(ex);

    tibEx_Destroy(ex);
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
    tibint32_t i;

    if (distinctSubscribers != subscriberCount)
    {
        for (i=0; i<msgNum; i++)
        {
            if (tibMessage_IsFieldSet(e,msgs[i],"receiverId"))
            {
                const char* id = tibMessage_GetString(e, msgs[i], "receiverId");

                if (strlen(id) <= MAX_SUBSCRIBER_ID && !strstr(id, ":"))
                {
                    if (!strstr(subscriberHayStack, id))
                    {
                        printf("got contacted by subscriber %s\n", id);

                        distinctSubscribers++;

                        tibAux_StringCat(subscriberHayStack, hayStackSize, ":");
                        tibAux_StringCat(subscriberHayStack, hayStackSize, id);
                    }
                }
                else
                {
                    printf("identifier wrong or too long\n");

                    running = tibfalse;
                }
            }
        }

        if (distinctSubscribers == subscriberCount)
        {
            printf("Connected to all subscribers - start sending\n");

            tibAux_Mutex_Lock(receiveCntMutex);

            receiveCnt = subscriberCount;

            tibAux_Mutex_Unlock(receiveCntMutex);
        }

        fflush(stdout);
    }
    else
    {
        tibAux_Mutex_Lock(receiveCntMutex);

        if (receiveCnt + msgNum <= subscriberCount)
        {
            receiveCnt += msgNum;
        }
        else
        {
            // Due to timer in tibthrurecv we can exceed subscriberCount.
            // Therefore limit it.

            receiveCnt = subscriberCount;
        }

        tibAux_Mutex_Unlock(receiveCntMutex);
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
    tibFieldRef             dataRef;

    printf("#\n# %s\n#\n# %s\n#\n", argv[0], tib_Version());

    parseArgs(argc, argv);

    hayStackSize = 1+subscriberCount*(MAX_SUBSCRIBER_ID+1);
    subscriberHayStack = malloc(hayStackSize);
    subscriberHayStack[0] = '\0';

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

    // set the client label
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

    props = tibProperties_Create(ex);
    if (durableName && *durableName)
        tibProperties_SetString(ex, props, TIB_SUBSCRIBER_PROPERTY_STRING_DURABLE_NAME, durableName);
    CHECK(ex);
    sub = tibSubscriber_Create(ex, realm, recvEndpointName, NULL, props);
    tibProperties_Destroy(ex, props);
    CHECK(ex);
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

    dataPub = tibPublisher_Create(ex, realm, endpointName, NULL);
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

    printf("\nSender Report:\n");
    printf("Requested %d messages.\n", messageCount);
    printf("Sending %d messages (%d batches of %d) with payload size %d\n", 
           batchSize * numSamples, numSamples, batchSize, payloadSize);
    fflush(stdout);

    tibAux_InitializeMutex(&receiveCntMutex);

    currentLimit = numSamples + warmupCount;

    senderThread = tibAux_LaunchThread(senderThreadFunction, NULL);
    while (tibEx_GetErrorCode(ex) == TIB_OK && running)
        tibEventQueue_Dispatch(ex, queue, TIB_TIMEOUT_WAIT_FOREVER);
    if (tibEx_GetErrorCode(ex) == TIB_INVALID_ARG)
        // Queue was destroyed in sender thread.
        tibEx_Clear(ex);
    else
        CHECK(ex);

    tibAux_JoinThread(senderThread);

    tibAux_CleanupMutex(&receiveCntMutex);

    tibSubscriber_Close(ex, sub);
    tibSubscriber_Close(ex, upAdvisorySub);
    tibSubscriber_Close(ex, downAdvisorySub);
    tibPublisher_Close(ex, dataPub);

    if (sendMsgs)
    {
        for (i = 0;  i < batchSize;  i++)
            tibMessage_Destroy(ex, sendMsgs[i]);
        free(sendMsgs);
    }

    free(payload);
    tibFieldRef_Destroy(ex, dataRef);
    tibRealm_Close(ex, realm);
    tib_Close(ex);
    CHECK(ex);

    free(subscriberHayStack);

    tibEx_Destroy(ex);

    return 0;
}
