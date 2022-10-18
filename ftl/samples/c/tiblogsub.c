/*
 * Copyright (c) 2010-2017 TIBCO Software Inc.
 * Licensed under a BSD-style license. Refer to [LICENSE]
 * For more information, please contact:
 * TIBCO Software Inc., Palo Alto, California, USA
 */

/*
 * This is a sample of a basic C FTL logging subscriber program.
 */

#include <stdlib.h>
#include <stdio.h>
#include <stddef.h>
#include <string.h>
#include <ctype.h>

#if !defined (MAC_OS_X_VERSION_10_12)
#define MAC_OS_X_VERSION_10_12 101200
#endif

#if defined (__APPLE__) && (MAC_OS_X_VERSION_MIN_REQUIRED >= MAC_OS_X_VERSION_10_12)
  #include <os/log.h>
#elif !defined (_WIN32)
  #include <syslog.h>
#endif

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

#if defined (_WIN32)
tibbool_t writeToStderr = tibtrue;
#else 
tibbool_t writeToStderr = tibfalse;
#endif

#if defined (__APPLE__) && (MAC_OS_X_VERSION_MIN_REQUIRED >= MAC_OS_X_VERSION_10_12)
static os_log_t log = NULL;
#endif

void
usage(
    int         argc,
    char        **argv)
{
    printf("Usage: %s [options] url\n", argv[0]);
    printf("Default url is "DEFAULT_URL"\n");
    printf("    -a,  --application <name>\n"
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
           "         --stderr write to stderr for debugging purposes\n"
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
        if (tibAux_GetFlag(&i, "--stderr", "--stderr"))
        {
            writeToStderr = tibtrue;
            continue;
        }
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
    // TODO: create field refs
}

void
cleanupMsgFieldRefs(
    tibEx               ex)
{
    // TODO: destroy fields refs
}

void
processFTLLogMessage(
    tibEx               ex,
    const char          *host,
    const char          *clientId,
    const char          *label,
    tibMessage          msg)
{
    tibint64_t          millis = 0;
    char                dateStr[256];
    const char          *level, *component, *statement;

    millis = tibMessage_GetLong(ex, msg, TIB_LOGMSG_FIELD_TIMESTAMP);
    tibAux_MillisecondsToStr(millis, dateStr, 256);

    level     = tibMessage_GetString(ex, msg, TIB_LOGMSG_FIELD_LOG_LEVEL);
    component = tibMessage_GetString(ex, msg, TIB_LOGMSG_FIELD_COMPONENT);
    statement = tibMessage_GetString(ex, msg, TIB_LOGMSG_FIELD_STATEMENT);

#if defined (__APPLE__) && (MAC_OS_X_VERSION_MIN_REQUIRED >= MAC_OS_X_VERSION_10_12)
    if (!strcmp(level, "info"))
        os_log_info(log, "%{public}s ftl-client-id=%{public}s label=%{public}s host=%{public}s %{public}s %{public}s %{public}s", dateStr, clientId, label, host, level, component, statement);
    else if (!strcmp(level, "debug"))
        os_log_debug(log, "%{public}s ftl-client-id=%{public}s label=%{public}s host=%{public}s %{public}s %{public}s %{public}s", dateStr, clientId, label, host, level, component, statement);
    else if (!strcmp(level, "severe"))
        os_log_error(log, "%{public}s ftl-client-id=%{public}s label=%{public}s host=%{public}s %{public}s %{public}s %{public}s", dateStr, clientId, label, host, level, component, statement);
    else
        os_log(log, "%{public}s ftl-client-id=%{public}s label=%{public}s host=%{public}s %{public}s %{public}s %{public}s", dateStr, clientId, label, host, level, component, statement);
    
#elif defined (__linux)
    if (!strcmp(level, "warn"))
        syslog(LOG_WARNING, "%s ftl-client-id=%s label=%s host=%s %s %s %s", dateStr, clientId, label, host, level, component, statement);
    else if (!strcmp(level, "severe"))
        syslog(LOG_ERR, "%s ftl-client-id=%s label=%s host=%s %s %s %s", dateStr, clientId, label, host, level, component, statement);
    else if (!strcmp(level, "debug"))
        syslog(LOG_DEBUG, "%s ftl-client-id=%s label=%s host=%s %s %s %s", dateStr, clientId, label, host, level, component, statement);
    else
        syslog(LOG_INFO, "%s ftl-client-id=%s label=%s host=%s %s %s %s", dateStr, clientId, label, host, level, component, statement);
#endif
    if (writeToStderr)
        fprintf(stderr, "%s ftl-client-id=%s label=%s host=%s %s %s %s\n", dateStr, clientId, label, host, level, component, statement);
}

void
processFTLAdvisoryLogMessage(
    tibEx               ex,
    const char          *host,
    const char          *clientId,
    const char          *label,
    tibMessage          msg)
{
}

void
onMessages(
    tibEx               ex,
    tibEventQueue       msgQueue,
    tibint32_t          msgNum,
    tibMessage          *msgs,
    void                **closures)
{
    tibint32_t          i, j;
    tibMessage          *logs;
    int                 count = 1;
    const char          *clientId, *host, *label;

    for (i = 0; i < msgNum; i++)
    {
        tibMessage msg = msgs[i];
  
        tibMessageIterator iter = tibMessageIterator_Create(ex, msg);
        while (tibMessageIterator_HasNext(ex, iter))
        {
            tibFieldRef ref = tibMessageIterator_GetNext(ex, iter);
            const char *fieldName = tibFieldRef_GetFieldName(ex, ref);

            if (tibEx_GetErrorCode(ex) != TIB_OK)
            {
                tibAux_PrintException(ex);
                break;
            }

            // get the host where a client or FTL service emitted the log msg
            host     = tibMessage_GetString(ex, msg, TIB_LOGSTREAM_FIELD_HOST);
            // get the id of the client or FTL service that emitted the log msg
            clientId = tibMessage_GetString(ex, msg, TIB_LOGSTREAM_FIELD_CLIENT_ID);
            // get the client_label of the client or FTL service that emitted the log msg
            label = tibMessage_GetString(ex, msg, TIB_LOGSTREAM_FIELD_CLIENT_LABEL);

            // Get the log msgs
            if (!strncmp(fieldName, TIB_LOGSTREAM_FIELD_LOGS, strlen(TIB_LOGSTREAM_FIELD_LOGS)))
            {
                logs  = tibMessage_GetArray(ex, msg, TIB_FIELD_TYPE_MESSAGE_ARRAY, TIB_LOGSTREAM_FIELD_LOGS, &count);
                for (j=0; j<count; j++) 
                {
                    if (tibMessage_GetLong(ex, logs[j], TIB_LOGMSG_FIELD_IS_ADVISORY) == tibfalse)
                       processFTLLogMessage(ex, host, clientId, label, logs[j]);
                    else 
                       processFTLAdvisoryLogMessage(ex, host, clientId, label, logs[j]);
                }
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
    
#if defined (__linux)
    setlogmask(LOG_UPTO(LOG_DEBUG));
#endif

#if defined (__APPLE__) && (MAC_OS_X_VERSION_MIN_REQUIRED >= MAC_OS_X_VERSION_10_12)
    log = os_log_create("com.tibco.ftl", "ftl");
#elif defined(__linux)
    openlog("com.tibco.ftl", LOG_USER | LOG_NDELAY, LOG_LOCAL0);
#endif

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
    sub = tibSubscriber_Create(ex, realm, TIB_LOGGING_ENDPOINT_NAME, cm, NULL);
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

#if defined(__linux)
    closelog();
#endif

    return 0;
}
