/*
 * Copyright (c) 2010-2018 TIBCO Software Inc.
 * Licensed under a BSD-style license. Refer to [LICENSE]
 * For more information, please contact:
 * TIBCO Software Inc., Palo Alto, California, USA
 */

/*
 * This is a sample of a basic C FTL subscriber program which receives the
 * requested number of messages and then cleans up and exits.
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

tibint32_t      count           = 1;
tibint32_t      recved          = 0;
const char      *appName        = "tibrecv";
const char      *realmServer    = NULL;
const char      *match          = NULL;
const char      *subEndpoint    = "tibrecv-endpoint";
const char      *trcLevel       = NULL;
const char      *user           = "guest";
const char      *password       = "guest-pw";
const char      *identifier     = NULL;
const char      *secondary      = NULL;
const char      *durableName    = NULL;
tibbool_t       explicitAck     = tibfalse;
tibbool_t       unsubscribe     = tibfalse;
tibbool_t       trustAll        = tibfalse;
const char      *trustFile      = NULL;
const char      *clientLabel    = NULL;

tibFieldRef     refLong         = NULL;
tibFieldRef     refString       = NULL;
tibFieldRef     refOpaque       = NULL;

typedef struct {
    tibint32_t      n;
    tibMessage      msgs[250];
} ackListRec, *ackList;

void
usage(
    int         argc,
    char        **argv)
{
    printf("Usage: %s [options] url\n", argv[0]);
    printf("Default url is "DEFAULT_URL"\n");
    printf("    -a,  --application <name>\n"
           "    -d,  --durablename <name>\n"
           "    -c,  --count <int>\n"
           "         --client-label <label>\n"
           "    -e,  --endpoint <name>\n"
           "    -h,  --help\n"
           "    -id, --identifier <name>\n"
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
           "    -us  --unsubscribe \n"
           "            the durable name needs to be specified as well\n"
           "    -x,  --explicitAck \n");

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
        if (tibAux_GetString(&i, "--durablename", "-d", &durableName))
            continue;
        if (tibAux_GetInt(&i ,"--count", "-c", &count))
            continue;
        if (tibAux_GetString(&i, "--endpoint", "-e", &subEndpoint))
            continue;
        if (tibAux_GetFlag(&i, "--help", "-h"))
            usage(argc, argv);
        if (tibAux_GetString(&i, "--identifier", "-id", &identifier))
            continue;
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
        if (tibAux_GetFlag(&i, "--explicitAck", "-x"))
        {
            explicitAck = tibtrue;
            continue;
        }
        if (tibAux_GetFlag(&i, "--unsubscribe", "-us"))
        {
            unsubscribe = tibtrue;
            continue;
        }
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
    refLong         = tibFieldRef_Create(ex, "My-Long");
    refString       = tibFieldRef_Create(ex, "My-String");
    refOpaque       = tibFieldRef_Create(ex, "My-Opaque");
}

void
cleanupMsgFieldRefs(
    tibEx               ex)
{
    tibFieldRef_Destroy(ex, refLong);
    tibFieldRef_Destroy(ex, refString);
    tibFieldRef_Destroy(ex, refOpaque);
}

void
printOpaque(
    tibEx               ex,
    const void          *buffer,
    tibint32_t          size)
{
    const char          *val = (const char*)buffer;
    tibint32_t          i;
    tibint32_t          unprintable = 0;

    if (tibEx_GetErrorCode(ex) != TIB_OK)
        return;

    printf("  type opaque: size %d data: '", size);
    for (i = 0; i < size; i++)
        if (isprint(val[i]))
            printf("%c", val[i]);
        else
            unprintable++;
    printf("'");
    if (unprintable > 0)
        printf(" (unprintable chars %d)", unprintable);
    printf("\n");
}

void
printMessage(
    tibEx               ex,
    tibMessage          msg)
{
    tibint64_t          lvalue = 0;
    const char          *svalue = NULL;
    const void          *ovalue = NULL;
    tibint32_t          size = 0;
    tibint32_t          len = 0;
    char                *buffer = 0;
    tibint32_t          fieldCount = 0;

    printf("message:\n");

    // we assume this has the fields set by tibsend.c, if not just
    // print the message and be done with it.
    if (tibMessage_IsFieldSetByRef(ex, msg, refLong))
    {
        lvalue = tibMessage_GetLongByRef(ex, msg, refLong);
        printf("  type long:  %"auxPRId64"\n", lvalue);
        fieldCount++;
    }
    if (tibMessage_IsFieldSetByRef(ex, msg, refString))
    {
        svalue = tibMessage_GetStringByRef(ex, msg, refString);
        printf("  type string: '%s'\n", svalue ? svalue : "<null>");
        fieldCount++;
    }
    if (tibMessage_IsFieldSetByRef(ex, msg, refOpaque))
    {
        ovalue = tibMessage_GetOpaqueByRef(ex, msg, refOpaque, &size);
        printOpaque(ex, ovalue, size);
        fieldCount++;
    }

    if (fieldCount != 3)
    {
        // get the size of output string
        len = tibMessage_ToString(ex, msg, NULL, 0);
        buffer = (char*)malloc(len);
        if (buffer)
        {
            (void)tibMessage_ToString(ex, msg, buffer, len);
            printf("  %s\n", buffer);
            free(buffer);
        }
    }

    if (tibEx_GetErrorCode(ex) != TIB_OK)
        tibAux_PrintException(ex);
    fflush(stdout);
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
        //
        // Do not destroy message as it is owned by the library.  If you need
        // to pass the messages to something outside this callback, use
        // tibMessage_MutableCopy and pass the copies.
        //
        recved++;
        printf("received message %d\n", recved);
        printMessage(ex, msgs[i]);

        if (explicitAck)
        {
            ackList     waitingAcks = (ackList) closures[i];

            waitingAcks->msgs[waitingAcks->n++] = tibMessage_MutableCopy(ex, msgs[i]);
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
    ackListRec          ackableMsgs;

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
    if (identifier)
        tibProperties_SetString(ex, props,
                                TIB_REALM_PROPERTY_STRING_APPINSTANCE_IDENTIFIER, identifier);
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
    props = tibProperties_Create(ex);

    if (explicitAck)
        tibProperties_SetBoolean(ex, props,
                                 TIB_SUBSCRIBER_PROPERTY_BOOL_EXPLICIT_ACK, explicitAck);

    if (durableName)
        tibProperties_SetString(ex, props,
                                TIB_SUBSCRIBER_PROPERTY_STRING_DURABLE_NAME, durableName);

    sub = tibSubscriber_Create(ex, realm, subEndpoint, cm, props);
    CHECK(ex);
    tibProperties_Destroy(ex, props);

    // Create a data loss advisory matcher 
    tibAux_snprintf(str, sizeof(str), "{\"%s\":\"%s\"}", 
                  TIB_ADVISORY_FIELD_NAME, TIB_ADVISORY_NAME_DATALOSS);
    am = tibContentMatcher_Create(ex, realm, str);

    // Create an advisory subscriber
    adv = tibSubscriber_Create(ex, realm, TIB_ADVISORY_ENDPOINT_NAME, am, NULL);
    CHECK(ex);

    // Create a queue and add subscribers to it
    props = tibProperties_Create(ex);
    tibProperties_SetInt(ex, props, TIB_EVENTQUEUE_PROPERTY_INT_DISCARD_POLICY, TIB_EVENTQUEUE_DISCARD_NEW);
    tibProperties_SetInt(ex, props, TIB_EVENTQUEUE_PROPERTY_INT_DISCARD_POLICY_MAX_EVENTS, 1000);
    queue = tibEventQueue_Create(ex, realm, props);
    tibProperties_Destroy(ex, props);
    tibEventQueue_AddSubscriber(ex, queue, sub, onMessages, &ackableMsgs);
    tibEventQueue_AddSubscriber(ex, queue, adv, onAdvisory, NULL);
    CHECK(ex);

    // set up references to the expected message format for fast access
    createMsgFieldRefs(ex);

    // Begin dispatching messages
    printf("waiting for message(s)\n");
    while (recved < count)
    {
        tibint32_t          i;

        ackableMsgs.n = 0;
        tibEventQueue_Dispatch(ex, queue, TIB_TIMEOUT_WAIT_FOREVER);
        if (tibEx_GetErrorCode(ex) != TIB_OK)
            break;

        for (i = 0;  i < ackableMsgs.n;  i++)
        {
            tibMessage_Acknowledge(ex, ackableMsgs.msgs[i]);
            if (tibEx_GetErrorCode(ex) != TIB_OK)
            {
                tibAux_PrintException(ex);
                tibEx_Clear(ex);
            }

            tibMessage_Destroy(ex, ackableMsgs.msgs[i]);
        }
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

    if (durableName && unsubscribe)
    {
        tibRealm_Unsubscribe(ex, realm, subEndpoint, durableName);
    }

    tibRealm_Close(ex, realm);
    tib_Close(ex);

    // Check if it worked before destroying the exception
    CHECK(ex);
    tibEx_Destroy(ex);

    return 0;
}
