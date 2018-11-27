/*
 * Copyright (c) 2010-2017 TIBCO Software Inc.
 * Licensed under a BSD-style license. Refer to [LICENSE]
 * For more information, please contact:
 * TIBCO Software Inc., Palo Alto, California, USA
 */

/*
 * This is a sample C program which can be used (with the directlatrecv program) to
 * measure basic latency between the publisher and subscriber.
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>

#include "tib/ftl.h"

#include "tibaux.h"

#define DEFAULT_URL "http://localhost:8080"

const char              *realmServer            = DEFAULT_URL;
const char              *appName                = "directlatsend";
const char              *sendEndpointName       = "directlatsend-sendendpoint";
const char              *receiveEndpointName    = "directlatsend-recvendpoint";
tibint32_t              messageCount            = 5000000;
tibint32_t              payloadSize             = 16;
void                    *payload                = NULL;
tibint32_t              warmupCount             = 10000;
const char              *trcLevel               = NULL;
const char              *user                   = "guest";
const char              *password               = "guest-pw";
const char              *csvFileName            = NULL;
const char              *identifier             = NULL;
const char              *clientLabel            = NULL;

tibbool_t               running                 = tibtrue;
tibint32_t              messageProgress         = 0;
tibint32_t              currentLimit            = 0;
tibDirectPublisher      pub       = NULL;
tibint32_t              sampleCountdown         = 0;
FILE                    *csvFile                = NULL;

tibint64_t              *samples                = NULL;
tibint32_t              nextSample              = 0;

tibAux_StatRecord       latencyStats            = {0, 0, 0, 0.0, 0.0};
tibint64_t              start                   = 0;

void
usage(int argc, char** argv)
{
    printf("Usage: %s [options] url\n", argv[0]);
    printf("Default url is "DEFAULT_URL"\n");
    printf(" where options can be:\n");
    printf("   -a,  --application <name>        Application name.\n"
           "        --client-label <label>      Set client label\n"
           "   -et, --tx-endpoint <name>        Transmit endpoint name\n"
           "   -er, --rx-endpoint <name>        Receive endpoint name\n"
           "   -c,  --count <int>               Send n messages.\n"
           "   -h,  --help                      Print this help.\n"
           "   -id, --identifier                Choose instance, eg, \"dshm\".\n"
           "   -o,  --out <file name>           Output samples to csv file.\n"
           "   -p,  --password <string>         Password for realm server.\n"
           "        --ping <float>              Initial ping timeout value (in seconds).\n"
           "   -s,  --size <int>                Send payload of n bytes.\n"
           "   -t,  --trace <level>             Set trace level\n"
           "           where <level> can be:\n"
           "                 off, severe, warn, info, debug, verbose.\n"
           "   -u,  --user <string>             User for realm server.\n"
           "   -w,  --warmup <int>              Warmup for n msgs (Default 10000).\n");

    exit(1);
} // usage

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
        if (tibAux_GetString(&i, "--tx-endpoint", "-et", &sendEndpointName))
            continue;
        if (tibAux_GetString(&i, "--rx-endpoint", "-er", &receiveEndpointName))
            continue;
        if (tibAux_GetInt(&i ,"--count", "-c", &messageCount))
            continue;
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
        if (tibAux_GetString(&i, "--out", "-o", &csvFileName))
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
} // parseArgs

static void
onDirectBuf(
    tibEx       e,
    tibint64_t  count,
    tibint64_t  size,
    tibint64_t  *sizeList,
    tibint8_t   *incoming,
    void        *closure)
{
    tibint64_t  stop;
    tibint8_t   *buf;

    stop = tibAux_GetTime();

    tibAux_StatUpdate(&latencyStats, (stop - start));

    if ((stop - start) * tibAux_GetTimerScale() > 0.001)
        printf("%g\n", (stop - start) * tibAux_GetTimerScale());

    if (++messageProgress < currentLimit)
    {
        if ((buf=tibDirectPublisher_Reserve(e, pub, 1, size, NULL)) != NULL)
        {
            memcpy(buf, payload, size);

            start = tibAux_GetTime();

            tibDirectPublisher_SendReserved(e, pub);
        }
    }
    else
        running = tibfalse;
} // onDirectBuf

static void
onSampledDirectBuf(
    tibEx       e,
    tibint64_t  count,
    tibint64_t  size,
    tibint64_t  *sizeList,
    tibint8_t   *incoming,
    void        *closure)
{
    tibint64_t  stop;
    tibint8_t   *buf;

    stop = tibAux_GetTime();

    samples[nextSample++] = stop - start;

    if (++messageProgress < currentLimit)
    {
        if ((buf=tibDirectPublisher_Reserve(e, pub, 1, size, NULL)) != NULL)
        {
            memcpy(buf, payload, size);

            start = tibAux_GetTime();

            tibDirectPublisher_SendReserved(e, pub);
        }
    }
    else
        running = tibfalse;
} // onSampledDirectBuf

int
main(int argc, char** argv)
{
    tibRealm            realm      = NULL;
    tibProperties       props      = NULL;
    tibEx               ex         = tibEx_Create();
    tibint32_t          numSamples = 0;
    tibint32_t          i;
    double              rttTime;
    double              latency;
    double              offset     = 0;
    tibDirectSubscriber sub        = NULL;
    tibint8_t           *buf;

    printf("#\n# %s\n#\n# %s\n#\n", argv[0], tib_Version());

    parseArgs(argc, argv);
    
    tibAux_CalibrateTimer();

    if (csvFileName != NULL && *csvFileName != '\0')
    {
        csvFile = tibAux_OpenCsvFile(csvFileName);

        printf("Producing a csv file named %s.  The columns in the file are:\n", csvFileName);
        printf("Sample number, latency, total time of sample, number of messages in sample,\n");
        printf("and the time offset of the sample.\n");
        printf("The output can be analyzed with the analyze-tiblatsend script.\n\n");

        // Allocate buffer for samples
        numSamples = messageCount;
        samples = calloc((numSamples+1), sizeof(tibint64_t));
        if (samples == NULL)
        {
            printf("Unable to malloc area for samples. Reduce your message count or\n"
                   "increase your sample interval.");
            exit(1);
        }

        // Make sure the buffer is fully swapped in
        memset(samples, 1, numSamples*sizeof(tibint64_t));
    }

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

    payload = calloc(payloadSize, 1);
    if (payload == NULL)
    {
        printf("Unable to malloc.");
        exit(1);
    }

    memset(payload, 119, payloadSize);

    sub = tibDirectSubscriber_Create(ex, realm, receiveEndpointName, NULL);
    CHECK(ex);

    pub = tibDirectPublisher_Create(ex, realm, sendEndpointName, NULL);
    CHECK(ex);

    if (csvFile != NULL)
        printf("Sampling latency every message.\n\n");

    if (warmupCount != 0)
    {
        currentLimit = warmupCount;

        start = tibAux_GetTime();

        buf = tibDirectPublisher_Reserve(ex, pub, 1, payloadSize, NULL);
        if (buf != NULL)
        {
            memcpy(buf, payload, payloadSize);
            tibDirectPublisher_SendReserved(ex, pub);

            while (tibEx_GetErrorCode(ex) == TIB_OK && running)
                tibDirectSubscriber_Dispatch(ex, sub,
                                             TIB_TIMEOUT_WAIT_FOREVER,
                                             onDirectBuf, NULL);
        }

        memset(&latencyStats, 0, sizeof(latencyStats));
    }
    
    printf("Sending %d messages with payload size %d\n", messageCount, payloadSize);

    messageProgress = 0;
    running         = tibtrue;
    currentLimit    = messageCount;

    buf = tibDirectPublisher_Reserve(ex, pub, 1, payloadSize, NULL);
    if (buf != NULL)
    {
        memcpy(buf, payload, payloadSize);

        start = tibAux_GetTime();

        tibDirectPublisher_SendReserved(ex, pub);

        if (csvFile == NULL)
            while (tibEx_GetErrorCode(ex) == TIB_OK && running)
                tibDirectSubscriber_Dispatch(ex, sub,
                                             TIB_TIMEOUT_WAIT_FOREVER,
                                             onDirectBuf, NULL);
        else
            while (tibEx_GetErrorCode(ex) == TIB_OK && running)
                tibDirectSubscriber_Dispatch(ex, sub,
                                             TIB_TIMEOUT_WAIT_FOREVER,
                                             onSampledDirectBuf, NULL);

    }

    if (csvFile != NULL)
        for (i = 0;  i < numSamples;  i++)
        {
            rttTime  = samples[i] * tibAux_GetTimerScale();
            latency  = 0.5 * rttTime;
            offset  += rttTime;
        
            tibAux_StatUpdate(&latencyStats, samples[i]);
        
            fprintf(csvFile, "%d,%.12e,%.12e,1,%.12e\n", (i + 1), latency,
                    rttTime, offset);
        }

    if (latencyStats.N > 1)
    {
        printf("\n%d round trip samples taken.\n", latencyStats.N);
        printf("Aggregate Latency: ");
        tibAux_PrintStats(&latencyStats, 0.5*tibAux_GetTimerScale());
               
        printf("\n\n");
    }

    if (csvFile != NULL)
        fclose(csvFile);

    tibDirectSubscriber_Close(ex, sub);
    tibDirectPublisher_Close(ex, pub);
    
    free(payload);

    tibRealm_Close(ex, realm);
    tib_Close(ex);
    CHECK(ex);
    
    tibEx_Destroy(ex);
    
    free(samples);
    
    return 0;
}
