/*
 * Copyright (c) 2010-2017 TIBCO Software Inc.
 * Licensed under a BSD-style license. Refer to [LICENSE]
 * For more information, please contact:
 * TIBCO Software Inc., Palo Alto, California, USA
 */

/*
 * This is a sample of a basic C FTL publisher program which sends the
 * requested number of messages at the desired delay between the sends.
 * It uses a timer for the delays.
 */

#include <stdlib.h>
#include <stdio.h>
#include <stddef.h>
#include <string.h>

#include "tib/ftl.h"

#include "tibaux.h"

#define DEFAULT_URL "http://localhost:8080"
#define DEFAULT_SECURE_URL "https://localhost:8080"

tibint32_t      count           = 1;
tibint32_t      delay           = 1000;
const char      *appName        = "tibsend";
const char      *realmServer    = NULL;
const char      *pubEndpoint    = "tibsend-endpoint";
const char      *formatName     = "Format-1";
const char      *trcLevel       = NULL;
const char      *user           = "guest";
const char      *password       = "guest-pw";
const char      *identifier     = NULL;
const char      *secondary      = NULL;
tibbool_t       keyedOpaque     = tibfalse;
tibbool_t       trustAll        = tibfalse;
const char      *trustFile      = NULL;
const char      *clientLabel    = NULL;

tibbool_t       finished        = tibfalse;

typedef struct {

    tibint32_t          count;

    tibRealm            realm;
    tibPublisher        pub;

    tibMessage          msg;

    tibFieldRef         refLong;
    tibFieldRef         refString;
    tibFieldRef         refOpaque;
    tibFieldRef         refKey;
    tibFieldRef         refData;

} PubStruct;

void
usage(
    int         argc,
    char        **argv)
{
    printf("Usage: %s [options] url\n", argv[0]);
    printf("Default url is "DEFAULT_URL"\n");
    printf("    -a, --application <name>\n"
           "        --client-label <label>\n"
           "    -c, --count <int>\n"
           "    -d, --delay <int> (in millis)\n"
           "    -e, --endpoint <name>\n"
           "    -f, --format <name>\n"
           "    -h, --help\n"
           "    -id, --identifier\n"
           "    -ko, --keyedopaque\n"
           "    -p, --password <string>\n"
           "    -s, --secondary <string>\n"
           "    -t, --trace <level>\n"
           "           where <level> can be:\n"
           "                 off, severe, warn, info, debug, verbose\n"
           "        --trustall\n"
           "        --trustfile <file>\n"
           "    -u, --user <string>\n");

    exit(1);
}

void
parseArgs(int argc, char** argv)
{
    int         i = 1;

    tibAux_InitGetArgs(argc, argv, usage);

    for (i = 1;  i < argc;  i++)
    {
        if (tibAux_GetString(&i, "--application", "-a", &appName))
            continue;
        if (tibAux_GetString(&i, "--client-label", "--client-label", &clientLabel))
            continue;
        if (tibAux_GetInt(&i ,"--count", "-c", &count))
            continue;
        if (tibAux_GetInt(&i ,"--delay", "-d", &delay))
            continue;
        if (tibAux_GetString(&i, "--endpoint", "-e", &pubEndpoint))
            continue;
        if (tibAux_GetString(&i ,"--format", "-f", &formatName))
            continue;
        if (tibAux_GetFlag(&i, "--help", "-h"))
            usage(argc, argv);
        if (tibAux_GetString(&i, "--identifier", "-id", &identifier))
            continue;
        if (tibAux_GetFlag(&i, "--keyedopaque", "-ko")) {
            keyedOpaque = tibtrue;
            continue;
        }
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

    if (keyedOpaque)
        formatName = TIB_BUILTIN_MSG_FMT_KEYED_OPAQUE;

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
initPubStruct(
    tibEx               ex,
    PubStruct           *pubStruct,
    tibRealm            realm,
    tibPublisher        pub)
{
    memset(pubStruct, 0, sizeof(PubStruct));

    pubStruct->realm   = realm;
    pubStruct->pub     = pub;

    // we use field references for faster access during sets
    if (keyedOpaque)
    {
        pubStruct->refKey  = tibFieldRef_Create(ex, TIB_BUILTIN_MSG_FMT_KEY_FIELDNAME);
        pubStruct->refData = tibFieldRef_Create(ex, TIB_BUILTIN_MSG_FMT_OPAQUE_FIELDNAME);
    }
    else
    {
        pubStruct->refLong   = tibFieldRef_Create(ex, "My-Long");
        pubStruct->refString = tibFieldRef_Create(ex, "My-String");
        pubStruct->refOpaque = tibFieldRef_Create(ex, "My-Opaque");
    }
}

void
initMessage(
    tibEx               ex,
    PubStruct           *pubStruct)
{
    if (pubStruct->msg == NULL)
        pubStruct->msg = tibMessage_Create(ex, pubStruct->realm, formatName);
    else
        tibMessage_ClearAllFields(ex, pubStruct->msg);

    if (keyedOpaque)
    {
        char str[16];

        str[0] = 0;
        tibAux_StringCatLong(str, sizeof(str), (tibint64_t)pubStruct->count++ % 5);
        tibMessage_SetStringByRef(ex, pubStruct->msg, pubStruct->refKey, str);
        tibMessage_SetOpaqueByRef(ex, pubStruct->msg, pubStruct->refData, 
                                  "opaque", sizeof("opaque") - 1);
    }
    else
    {
        tibMessage_SetLongByRef(ex, pubStruct->msg, pubStruct->refLong,
                                (tibint64_t)++pubStruct->count);
        tibMessage_SetStringByRef(ex, pubStruct->msg, pubStruct->refString, pubEndpoint);
        tibMessage_SetOpaqueByRef(ex, pubStruct->msg, pubStruct->refOpaque, 
                                  "opaque", sizeof("opaque") - 1);
    }
}

void
cleanupPubStruct(
    tibEx               ex,
    PubStruct           *pubStruct)
{
    if (keyedOpaque)
    {
        tibFieldRef_Destroy(ex, pubStruct->refKey);
        tibFieldRef_Destroy(ex, pubStruct->refData);
    }
    else
    {
        tibFieldRef_Destroy(ex, pubStruct->refLong);
        tibFieldRef_Destroy(ex, pubStruct->refString);
        tibFieldRef_Destroy(ex, pubStruct->refOpaque);
    }

    tibMessage_Destroy(ex, pubStruct->msg);
}

void
onSend(
    tibEx               ex,
    tibEventQueue       msgQueue,
    tibTimer            timer,
    void*               closure)
{
    PubStruct           *pubStruct = (PubStruct*)closure;

    initMessage(ex, pubStruct);

    printf("sending message %d\n", pubStruct->count); 
    fflush(stdout);
    tibPublisher_Send(ex, pubStruct->pub, pubStruct->msg);

    // any exception that happens in the callback does not
    // propagate to the caller of the tibEventQueue_Dispatch
    if (tibEx_GetErrorCode(ex) != TIB_OK)
        tibAux_PrintException(ex);

    if (pubStruct->count == count)
        finished = tibtrue;
}

void
onTimerComplete(
    tibEx                       ex,
    tibTimer                    timer,
    void*                       closure)
{
    PubStruct           *pubStruct = (PubStruct*)closure;

    // no timer callback can be using this any more
    cleanupPubStruct(ex, pubStruct);
    free(pubStruct);
}

void
onNotification(
    tibEx                       ex,
    tibRealmNotificationType    type,
    const char                  *reason,
    void                        *closure)
{
    if (type == TIB_CLIENT_DISABLED)
    {
        printf("application administratively disabled: %s\n", reason);
        finished = tibtrue;
    }
    else
    {
        printf("notification type %d: %s\n", (int)type, reason);
    }
}

int
main(int argc, char** argv)
{
    tibEx               ex;
    tibProperties       props;
    tibRealm            realm = NULL;
    tibEventQueue       queue = NULL;
    tibPublisher        pub;
    PubStruct           *pubStruct;
    tibTimer            timer;

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

    // Set notification handler to log administrative actions
    tibRealm_SetNotificationHandler(ex, realm, onNotification, NULL);

    // Create sender object
    pub = tibPublisher_Create(ex, realm, pubEndpoint, NULL);
    CHECK(ex);

    // Initialize closure for send timer callback
    pubStruct = malloc(sizeof(PubStruct));
    if (pubStruct == NULL)
        memFail("pubStruct");
    initPubStruct(ex, pubStruct, realm, pub);
    CHECK(ex);

    // Start a timer to do the sends
    queue = tibEventQueue_Create(ex, realm, NULL);
    timer = tibEventQueue_CreateTimer(ex, queue, (1.0*delay)/1000.0, 
                                      onSend, pubStruct);
    CHECK(ex);

    while (tibEx_GetErrorCode(ex) == TIB_OK && !finished)
        tibEventQueue_Dispatch(ex, queue, TIB_TIMEOUT_WAIT_FOREVER);

    if (!finished)
    {
        // Something unusual happened. Give notification handler a chance to
        // run so that we can log if this is an administrative action.
        tibAux_SleepMillis(1000);
    }

    // Stop the timer
    tibEventQueue_DestroyTimer(ex, queue, timer, onTimerComplete);
    tibEventQueue_Destroy(ex, queue, NULL);

    // Clean up
    tibPublisher_Close(ex, pub);
    tibRealm_Close(ex, realm);
    tib_Close(ex);

    // Check if it worked before destroying the exception
    CHECK(ex);
    tibEx_Destroy(ex);

    return 0;
}
