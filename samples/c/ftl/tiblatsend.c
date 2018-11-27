/*
 * Copyright (c) 2010-2017 TIBCO Software Inc.
 * Licensed under a BSD-style license. Refer to [LICENSE]
 * For more information, please contact:
 * TIBCO Software Inc., Palo Alto, California, USA
 */

/*
 * This is a sample C program which can be used (with the tiblatrecv program) to
 * measure basic latency between the publisher and subscriber.
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
tibbool_t       explicitAck             = tibfalse;
tibint32_t      messageCount            = 5000000;
tibint32_t      payloadSize             = 16;
tibint32_t      warmupCount             = 10000;
const char      *trcLevel               = NULL;
const char      *user                   = "guest";
const char      *password               = "guest-pw";
tibint32_t      sampleInterval          = 0;
const char*     csvFileName             = NULL;
tibbool_t       quiet                   = tibfalse;
const char      *identifier             = NULL;
tibbool_t       trustAll                = tibfalse;
const char      *trustFile              = NULL;
const char      *clientLabel            = NULL;

tibbool_t       running                 = tibtrue;
tibdouble_t     pingInterval            = 1.0;
tibint32_t      messageProgress         = 0;
tibint32_t      currentLimit            = 0;
tibMessage      msg                     = NULL;
tibPublisher    pub                     = NULL;
tibEventQueue   queue                   = NULL;
tibSubscriber   subs                    = NULL;
tibint32_t      sampleCountdown         = 0;
FILE*           csvFile                 = NULL;

tibint64_t      *samples                = NULL;
tibint32_t      nextSample              = 0;

// Used for timing results
tibAux_StatRecord   latencyStats        = {0, 0, 0, 0.0, 0.0};

volatile tibTimer   pongTimer           = NULL;

void
usage(int argc, char** argv)
{
    printf("Usage: %s [options] url\n", argv[0]);
    printf("Default url is "DEFAULT_URL"\n");
    printf(" where options can be:\n");
    printf("   -a,  --application <name>        Application name\n"
           "   -c,  --count <int>               Send n messages.\n"
           "        --client-label <label>      Set client label\n"
           "   -d,  --durableName <name>        Durable name for receive side.\n"
           "   -et, --tx-endpoint <name>        Transmit endpoint name\n"
           "   -er, --rx-endpoint <name>        Receive endpoint name\n"
           "   -h,  --help                      Print this help.\n"
           "   -id, --identifier                Choose instance, eg, \"rdma\"\n"
           "   -i,  --interval <int>            Sample latency every n messages.\n"
           "   -o,  --out <file name>           Output samples to csv file\n"
           "   -p,  --password <string>         Password for realm server.\n"
           "        --ping <float>              Initial ping timeout value (in seconds).\n"
           "   -q,  --quiet                     Don't print intermediate results\n"
           "   -s,  --size <int>                Send payload of n bytes.\n"
           "   -t,  --trace <level>             Set trace level\n"
           "           where <level> can be:\n"
           "                 off, severe, warn, info, debug, verbose\n"
           "   -u,  --user <string>             User for realm server.\n"
           "   -w,  --warmup <int>              Warmup for n msgs (Default 10000)\n"
           "   -x,  --explicit-ack              Use explicit acks.\n"
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
        if (tibAux_GetInt(&i, "--interval", "-i", &sampleInterval))
            continue;
        if (tibAux_GetString(&i, "--out", "-o", &csvFileName))
            continue;
        if (tibAux_GetString(&i, "--durableName", "-d", &durableName))
            continue;
        if (tibAux_GetFlag(&i, "--explicit-ack", "-x")) {
            explicitAck = tibtrue;
            continue;
        }
        if (tibAux_GetFlag(&i, "--quiet", "-q")) {
            quiet = tibtrue;
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

    if (!sampleInterval)
        sampleInterval = messageCount;

    printf("Invoked as: ");
    for (i = 0;  i < argc;  i++)
        printf("%s ", argv[i]);
    printf("\n");
}

void
onMsg(
    tibEx                       e,
    tibEventQueue               msgQueue,
    tibint32_t                  msgNum,
    tibMessage                  *msgs,
    void                        **closures)
{
    tibint64_t      stop = 0;

    if (msgNum != 1) {
        printf("Received %d messages instead of one.\n", msgNum);
        running = tibfalse;
    }

    if (!--sampleCountdown) {
        stop = tibAux_GetTime();
    }

    if (++messageProgress < currentLimit) {
        tibPublisher_Send(e, pub, msg);
    } else {
        running = tibfalse;
    }

    if (!sampleCountdown) {
        samples[nextSample++] = stop;
        sampleCountdown = sampleInterval;
    }
}

void
onMsgWithAck(
    tibEx                       e,
    tibEventQueue               msgQueue,
    tibint32_t                  msgNum,
    tibMessage                  *msgs,
    void                        **closures)
{
    tibint64_t      stop = 0;

    if (msgNum != 1) {
        printf("Received %d messages instead of one.\n", msgNum);
        running = tibfalse;
    }

    if (!--sampleCountdown) {
        stop = tibAux_GetTime();
    }

    if (++messageProgress < currentLimit) {
        tibPublisher_Send(e, pub, msg);
    } else {
        if (messageProgress == currentLimit)
            running = tibfalse;
    }

    if (!sampleCountdown) {
        samples[nextSample++] = stop;
        sampleCountdown = sampleInterval;
    }

    tibMessage_Acknowledge(e, msgs[0]);
}

static void
onSubRemoved(
    tibEx            e,
    tibSubscriber    rsub,
    void*            closure)
{
    // Subscriber has been successfully removed from the queue.
    // Now add it back with the callback to be used for the actual test.
    if (explicitAck)
        tibEventQueue_AddSubscriber(e, queue, rsub, onMsgWithAck, NULL);
    else
        tibEventQueue_AddSubscriber(e, queue, rsub, onMsg, NULL);
    CHECK(e);

    // Send the first real message.
    tibPublisher_Send(e, pub, msg);
}

void
onPongMsg(
    tibEx                       e,
    tibEventQueue               msgQueue,
    tibint32_t                  msgNum,
    tibMessage                  *msgs,
    void                        **closures)
{
    if (msgNum != 1) {
        printf("Received %d messages instead of one.\n", msgNum);
        running = tibfalse;
    }

    if (pongTimer)
    {
        tibEventQueue_DestroyTimer(e, msgQueue, pongTimer, NULL);
        pongTimer = NULL;
    }

    // Change our subscription before starting the real test interval.
    tibEventQueue_RemoveSubscriber(e, msgQueue, subs, onSubRemoved);
    CHECK(e);

    if (explicitAck)
        tibMessage_Acknowledge(e, msgs[0]);
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
        tibPublisher_Send(e, pub, ping);
    }
}

int
main(int argc, char** argv)
{
    tibRealm                realm   = NULL;
    void*                   payload = NULL;
    tibProperties           props   = NULL;
    tibEx                   ex      = tibEx_Create();
    tibint32_t              numSamples;
    tibint32_t              i;

    printf("#\n# %s\n#\n# %s\n#\n", argv[0], tib_Version());

    parseArgs(argc, argv);
    
    tibAux_CalibrateTimer();

    if (csvFileName && *csvFileName) {
        csvFile = tibAux_OpenCsvFile(csvFileName);
        printf("Producing a csv file named %s.  The columns in the file are:\n", csvFileName);
        printf("Sample number, latency, total time of sample, number of messages in sample,\n");
        printf("and the time offset of the sample.\n");
        printf("The output can be analyzed with the analyze-tiblatsend script.\n\n");
    }
    
    // Allocate buffer for samples
    numSamples = messageCount/sampleInterval;
    samples = calloc((numSamples+1), sizeof(tibint64_t));
    if (!samples) {
        printf("Unable to malloc area for samples. Reduce your message count or\n"
               "increase your sample interval.");
        exit(1);
    }

    // Make sure the buffer is fully swapped in
    memset(samples, 1, numSamples*sizeof(tibint64_t));

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
    if (explicitAck)
        tibProperties_SetBoolean(ex, props, TIB_SUBSCRIBER_PROPERTY_BOOL_EXPLICIT_ACK, tibtrue);
    CHECK(ex);
    subs = tibSubscriber_Create(ex, realm, recvEndpointName, NULL, props);
    tibProperties_Destroy(ex, props);
    CHECK(ex);

    if (pingInterval)
    {
        tibEventQueue_AddSubscriber(ex, queue, subs, onPongMsg, NULL);
    }
    else
    {
        if (explicitAck)
            tibEventQueue_AddSubscriber(ex, queue, subs, onMsgWithAck, NULL);
        else
            tibEventQueue_AddSubscriber(ex, queue, subs, onMsg, NULL);
    }
    CHECK(ex);

    pub = tibPublisher_Create(ex, realm, endpointName, NULL);
    CHECK(ex);

    payload = calloc(payloadSize, 1);
    msg = tibMessage_Create(ex, realm, TIB_BUILTIN_MSG_FMT_OPAQUE);
    tibMessage_SetOpaque(ex, msg, TIB_BUILTIN_MSG_FMT_OPAQUE_FIELDNAME, payload, payloadSize);
    CHECK(ex);
    
    printf("Sending %d messages with payload size %d\n", messageCount, payloadSize);
    printf("Sampling latency every %d messages.\n\n", sampleInterval);

    if (pingInterval)
        pongTimer = tibEventQueue_CreateTimer(ex, queue, pingInterval, onPongTimeout, msg);

    if (warmupCount) {
        messageProgress = 0;
        running = tibtrue;
        currentLimit = warmupCount;
        tibPublisher_Send(ex, pub, msg);
        while (tibEx_GetErrorCode(ex) == TIB_OK && running)
            tibEventQueue_Dispatch(ex, queue, TIB_TIMEOUT_WAIT_FOREVER);
    }
    
    messageProgress = 0;
    running = tibtrue;
    currentLimit = messageCount;
    sampleCountdown = sampleInterval;
    samples[nextSample++] = tibAux_GetTime();
    tibPublisher_Send(ex, pub, msg);
    while (tibEx_GetErrorCode(ex) == TIB_OK && running)
        tibEventQueue_Dispatch(ex, queue, TIB_TIMEOUT_WAIT_FOREVER);

    for (i = 1;  i <= numSamples;  i++) {
        double      totalTime   = (samples[i] - samples[i-1]) * tibAux_GetTimerScale();
        double      latency     = 0.5 * totalTime / (double)sampleInterval;
        
        tibAux_StatUpdate(&latencyStats, samples[i]-samples[i-1]);
        
        if (!quiet) {
            printf("Total time: ");
            tibAux_PrintNumber(totalTime);
            printf(" sec. ");
            
            printf("for %11d messages      ", sampleInterval);
            
            printf("One way latency: ");
            tibAux_PrintNumber(latency);
            printf(" sec.\n");
        }
        
        if (csvFile) {
            fprintf(csvFile, "%d,%.12e,%.12e,%d,%.12e\n", latencyStats.N, latency, totalTime,
                    sampleInterval, (samples[i-1]-samples[0])*tibAux_GetTimerScale());
        }
    }
    
    if (latencyStats.N > 1) {
        printf("\n%d samples taken, each one averaging results from %d round trips.\n",
               latencyStats.N, sampleInterval);
        printf("Aggregate Latency: ");
        tibAux_PrintStats(&latencyStats, 0.5*tibAux_GetTimerScale()/sampleInterval);
        printf("\n\n");
    }
    
    if (csvFile)
        fclose(csvFile);
    
    if (subs && queue)
        tibEventQueue_RemoveSubscriber(ex, queue, subs, NULL);
    tibSubscriber_Close(ex, subs);
    if (pongTimer)
        tibEventQueue_DestroyTimer(ex, queue, pongTimer, NULL);
    tibEventQueue_Destroy(ex, queue, NULL);
    tibPublisher_Close(ex, pub);
    tibMessage_Destroy(ex, msg);
    free(payload);
    tibRealm_Close(ex, realm);
    tib_Close(ex);
    CHECK(ex);
    
    tibEx_Destroy(ex);
    
    free(samples);
    
    return 0;
}
