/*
 * Copyright (c) 2010-2017 TIBCO Software Inc.
 * Licensed under a BSD-style license. Refer to [LICENSE]
 * For more information, please contact:
 * TIBCO Software Inc., Palo Alto, California, USA
 */

/*
 * This is a sample of a basic C FTL monitoring subscriber program.
 */

#include <stdlib.h>
#include <stdio.h>
#include <stddef.h>
#include <string.h>
#include <ctype.h>

#include "tib/ftl.h"

#include "tibaux.h"

#define DEFAULT_URL "http://localhost:8080"
#define DEFAULT_SECURE_URL "https://localhost:8080"

const char      *appName        = NULL;
const char      *realmServer    = NULL;
const char      *match          = NULL;
const char      *trcLevel       = NULL;
const char      *user           = "guest";
const char      *password       = "guest-pw";
const char      *secondary      = NULL;
tibbool_t       trustAll        = tibfalse;
const char      *trustFile      = NULL;
const char      *clientLabel    = NULL;

tibFieldRef     refMsgType             = NULL;
tibFieldRef     refMetrics             = NULL;

void
usage(
    int         argc,
    char        **argv)
{
    printf("Usage: %s [options] url\n", argv[0]);
    printf("Default url is "DEFAULT_URL"\n");
    printf("    -a,  --application <name>\n"
           "    -d,  --durablename <name>\n"
           "         --client-label <label>\n"
           "    -h,  --help\n"
           "    -m,  --match <content match>\n"
           "            where <content match> is of the form:\n"
           "                 '{\"fieldname\":<value>}'\n"
           "            where <value> is:\n"
           "                 \"string\" for a string field\n"
           "                 a number such as 2147483647 for an integer field\n"
           "    -p,  --password <string>\n"
           "    -u,  --user <string>\n"
           "    -s,  --secondary <string>\n"
           "    -t,  --trace <level>\n"
           "            where <level> can be:\n"
           "                  off, severe, warn, info, debug, verbose\n"
           "         --trustall\n"
           "         --trustfile <file>\n"
           );

    exit(1);
}

void
parseArgs(int argc, char** argv)
{
    int    i = 1;

    tibAux_InitGetArgs(argc, argv, usage);

    for (i = 1;  i < argc;  i++)
    {
        if (tibAux_GetString(&i, "--application", "-a", &appName))
            continue;
        if (tibAux_GetString(&i, "--client-label", "--client-label", &clientLabel))
            continue;
        if (tibAux_GetFlag(&i, "--help", "-h"))
            usage(argc, argv);
        if (tibAux_GetString(&i, "--match", "-m", &match))
            continue;
        if (tibAux_GetString(&i, "--password", "-p", &password))
            continue;
        if (tibAux_GetString(&i, "--secondary", "-s", &secondary))
            continue;
        if (tibAux_GetString(&i, "--trace", "-t", &trcLevel))
            continue;
        if (tibAux_GetString(&i, "--user", "-u", &user))
            continue;
        if (tibAux_GetFlag(&i, "--trustall", "--trustall")) {
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
memFail(
    const char          *obj)
{
    printf("memory allocation failed for %s\n", obj);
    exit(-1);
}

void
createMsgFieldRefs(
    tibEx               ex)
{
    refMsgType                 = tibFieldRef_Create(ex, TIB_MONITORING_FIELD_MSG_TYPE);
    refMetrics                 = tibFieldRef_Create(ex, TIB_MONITORING_FIELD_METRICS);
}

void
cleanupMsgFieldRefs(
    tibEx               ex)
{
    tibFieldRef_Destroy(ex, refMsgType);
    tibFieldRef_Destroy(ex, refMetrics);
}

void
processMessageFields(
    tibEx               ex,
    tibMessage          msg,
    unsigned int        indent)
{
    tibbool_t           addSeparator = tibfalse;
    tibMessageIterator  it = NULL;

    while (indent-- > 0)
    {
        fputs("    ", stdout);
    }
    it = tibMessageIterator_Create(ex, msg);
    while (tibMessageIterator_HasNext(ex, it))
    {
        tibFieldRef ref = NULL;
        const char * fieldName = NULL;
        tibFieldType fieldType;
        const char * stringVal;
        tibint64_t intVal;
        char valBuf[32];

        ref = tibMessageIterator_GetNext(ex, it);
        fieldType = tibMessage_GetFieldTypeByRef(ex, msg, ref);
        switch (fieldType)
        {
            case TIB_FIELD_TYPE_LONG_ARRAY:
            case TIB_FIELD_TYPE_DOUBLE_ARRAY:
            case TIB_FIELD_TYPE_STRING_ARRAY:
            case TIB_FIELD_TYPE_MESSAGE_ARRAY:
            case TIB_FIELD_TYPE_DATETIME_ARRAY:
                /* Skip array fields */
                continue;
            default:
                break;
        }
        fieldName = tibFieldRef_GetFieldName(ex, ref);
        if (addSeparator)
        {
            fputs(", ", stdout);
        }
        addSeparator = tibtrue;
        fputs(fieldName, stdout);
        fputs("=", stdout);
        switch (fieldType)
        {
            case TIB_FIELD_TYPE_LONG:
                intVal = tibMessage_GetLongByRef(ex, msg, ref);
                tibAux_snprintf(valBuf, sizeof(valBuf), "%" auxPRId64, intVal);
                fputs(valBuf, stdout);
                break;
            case TIB_FIELD_TYPE_STRING:
                stringVal = tibMessage_GetStringByRef(ex, msg, ref);
                fputs(stringVal, stdout);
                break;
            default:
                fputs("Unsupported field type", stdout);
                break;
        }
    }
    tibMessageIterator_Destroy(ex, it);
    fputs("\n", stdout);
}

void
processFTLMetricMessage(
    tibEx               ex,
    tibMessage          msg)
{
    tibMessage*         metrics = NULL;
    tibint32_t          metricsCount;
    tibint32_t          idx;

    if (!tibMessage_IsFieldSetByRef(ex, msg, refMetrics))
    {
        return;
    }
    processMessageFields(ex, msg, 0);
    metrics = (tibMessage *) tibMessage_GetArrayByRef(ex, msg, TIB_FIELD_TYPE_MESSAGE_ARRAY, refMetrics, &metricsCount);
    for (idx = 0; idx < metricsCount; ++idx)
    {
        processMessageFields(ex, metrics[idx], 1);
    }
}

void
processFTLEventMessage(
    tibEx               ex,
    tibMessage          msg)
{
    processMessageFields(ex, msg, 0);
}

void
onMessages(
    tibEx               ex,
    tibEventQueue       msgQueue,
    tibint32_t          msgNum,
    tibMessage          *msgs,
    void                **closures)
{
    tibint32_t          i;

    for (i = 0; i < msgNum; i++)
    {
        tibMessage msg = msgs[i];
        if (tibMessage_IsFieldSetByRef(ex, msg, refMsgType))
        {
            tibint64_t msgType = tibMessage_GetLongByRef(ex, msg, refMsgType);
            switch (msgType)
            {
                case TIB_MONITORING_MSG_TYPE_METRICS:
                    fputs("Metrics: ", stdout);
                    processFTLMetricMessage(ex, msg);
                    break;
                case TIB_MONITORING_MSG_TYPE_SERVER_METRICS:
                    fputs("Server metrics: ", stdout);
                    processFTLMetricMessage(ex, msg);
                    break;
                case TIB_MONITORING_MSG_TYPE_EVENT:
                    fputs("Event: ", stdout);
                    processFTLEventMessage(ex, msg);
                    break;
                default:
                    break;
            }
        }
    }

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
    tibEx               ex;
    tibProperties       props;
    tibRealm            realm;
    tibEventQueue       queue = NULL;
    tibContentMatcher   cm = NULL;
    tibContentMatcher   am;
    tibSubscriber       sub;
    tibSubscriber       adv;
    char                str[256];

    printf("#\n# %s\n#\n# %s\n#\n", argv[0], tib_Version());

    parseArgs(argc, argv);

    ex = tibEx_Create();
    if (ex == NULL)
        memFail("ex");

    tib_Open(ex, TIB_COMPATIBILITY_VERSION);
    CHECK(ex);

    // Set global trace to specified level
    if (trcLevel != NULL)
        tib_SetLogLevel(ex, trcLevel);

    // Get a single realm per process
    props = tibProperties_Create(ex);
    tibProperties_SetString(ex, props,
                            TIB_REALM_PROPERTY_STRING_USERNAME, user);
    tibProperties_SetString(ex, props,
                            TIB_REALM_PROPERTY_STRING_USERPASSWORD, password);
    if (secondary)
        tibProperties_SetString(ex, props,
                                TIB_REALM_PROPERTY_STRING_SECONDARY_SERVER, secondary);

    if (trustAll || trustFile)
    {
        if (!realmServer)
            realmServer = DEFAULT_SECURE_URL;

        if (trustAll)
        {
            tibProperties_SetLong(ex, props, TIB_REALM_PROPERTY_LONG_TRUST_TYPE,
                                        TIB_REALM_HTTPS_CONNECTION_TRUST_EVERYONE);
        }
        else
        {
            tibProperties_SetLong(ex, props, TIB_REALM_PROPERTY_LONG_TRUST_TYPE,
                    TIB_REALM_HTTPS_CONNECTION_USE_SPECIFIED_TRUST_FILE);
            tibProperties_SetString(ex, props, TIB_REALM_PROPERTY_STRING_TRUST_FILE, trustFile);
        }
    }
    else
    {
        if (!realmServer)
            realmServer = DEFAULT_URL;
    }

    // set the client label
    if (clientLabel != NULL)
        tibProperties_SetString(ex, props, TIB_REALM_PROPERTY_STRING_CLIENT_LABEL, clientLabel);
    else
        tibProperties_SetString(ex, props, TIB_REALM_PROPERTY_STRING_CLIENT_LABEL, argv[0]);

    realm = tibRealm_Connect(ex, realmServer, appName, props);
    CHECK(ex);
    tibProperties_Destroy(ex, props);

    // Create a content matcher, if specified
    if (match != NULL)
        cm = tibContentMatcher_Create(ex, realm, match);

    // Create a subscriber
    sub = tibSubscriber_Create(ex, realm, TIB_MONITORING_ENDPOINT_NAME, cm, NULL);
    CHECK(ex);

    // Create a data loss advisory matcher 
    tibAux_snprintf(str, sizeof(str), "{\"%s\":\"%s\"}", 
                  TIB_ADVISORY_FIELD_NAME, TIB_ADVISORY_NAME_DATALOSS);
    am = tibContentMatcher_Create(ex, realm, str);

    // Create an advisory subscriber
    adv = tibSubscriber_Create(ex, realm, TIB_ADVISORY_ENDPOINT_NAME, am, NULL);
    CHECK(ex);

    // Create a queue and add subscribers to it
    queue = tibEventQueue_Create(ex, realm, NULL);
    tibEventQueue_AddSubscriber(ex, queue, sub, onMessages, NULL);
    tibEventQueue_AddSubscriber(ex, queue, adv, onAdvisory, NULL);
    CHECK(ex);

    // set up references to the expected message format for fast access
    createMsgFieldRefs(ex);

    // Begin dispatching messages
    printf("waiting for message(s)\n");
    while (1)
    {
        tibEventQueue_Dispatch(ex, queue, TIB_TIMEOUT_WAIT_FOREVER);
        if (tibEx_GetErrorCode(ex) != TIB_OK)
            break;
    }

    // Clean up
    cleanupMsgFieldRefs(ex);
    if (queue && sub)
        tibEventQueue_RemoveSubscriber(ex, queue, sub, NULL);
    if (queue && adv)
        tibEventQueue_RemoveSubscriber(ex, queue, adv, NULL);
    tibEventQueue_Destroy(ex, queue, NULL);
    tibSubscriber_Close(ex, sub);
    tibSubscriber_Close(ex, adv);
    tibContentMatcher_Destroy(ex, cm);
    tibContentMatcher_Destroy(ex, am);

    tibRealm_Close(ex, realm);
    tib_Close(ex);

    // Check if it worked before destroying the exception
    CHECK(ex);
    tibEx_Destroy(ex);

    return 0;
}
