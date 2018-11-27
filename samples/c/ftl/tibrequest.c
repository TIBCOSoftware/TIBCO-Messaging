/*
 * Copyright (c) 2010-2017 TIBCO Software Inc.
 * Licensed under a BSD-style license. Refer to [LICENSE]
 * For more information, please contact:
 * TIBCO Software Inc., Palo Alto, California, USA
 */

/*
 * This is a sample C request program which is used with the tibreply program
 * to demonstrate the basic use of inboxes.
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "tib/ftl.h"

#include "tibaux.h"

#define DEFAULT_URL "http://localhost:8080"

const char*     realmServer             = DEFAULT_URL;
const char*     appName                 = "tibrequest";
const char*     endpointName            = "tibrequest-endpoint";
const char      *trcLevel               = NULL;
const char      *user                   = "guest";
const char      *password               = "guest-pw";
const char      *identifier             = NULL;
const char      *secondary              = NULL;
const char      *clientLabel            = NULL;

tibbool_t       replyReceived           = tibfalse;
tibdouble_t     pingInterval            = 1.0;
tibPublisher    pub                     = NULL;

volatile tibTimer   pongTimer           = NULL;

void
usage(int argc, char** argv)
{
    printf("Usage: %s [options] url\n", argv[0]);
    printf("Default url is "DEFAULT_URL"\n");
    printf("   -a, --application <name>\n"
           "       --client-label <label>\n"
           "   -e, --endpoint <name>\n"
           "   -h, --help\n"
           "   -id,--identifier\n"
           "   -p, --password <string>\n"
           "       --ping <float> (in seconds)\n"
           "   -s, --secondary <string>\n"
           "   -t, --trace <level>\n"
           "          where <level> can be:\n"
           "                off, severe, warn, info, debug, verbose\n"
           "   -u, --user <string>\n");

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
        if (tibAux_GetString(&i, "--endpoint", "-e", &endpointName))
            continue;
        if (tibAux_GetFlag(&i, "--help", "-h"))
            usage(argc, argv);
        if (tibAux_GetString(&i, "--identifier", "-id", &identifier))
            continue;
        if (tibAux_GetString(&i, "--password", "-p", &password))
            continue;
        if (tibAux_GetDouble(&i, "--ping", "--ping", &pingInterval))
            continue;
        if (tibAux_GetString(&i, "--secondary", "-s", &secondary))
            continue;
        if (tibAux_GetString(&i, "--trace", "-t", &trcLevel))
            continue;
        if (tibAux_GetString(&i, "--user", "-u", &user))
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
}

void
printMessage(
    tibEx               ex,
    tibMessage          msg)
{
    tibint32_t          len = 0;
    char                *buffer = 0;

    if (tibEx_GetErrorCode(ex) != TIB_OK)
        return;

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

void
onMsg(
    tibEx                       ex,
    tibEventQueue               msgQueue,
    tibint32_t                  msgNum,
    tibMessage                  *msgs,
    void                        **closures)
{
    if (msgNum != 1)
    {
        printf("Received %d messages instead of one.\n", msgNum);
        exit(1);
    }

    if (pongTimer)
    {
        tibEventQueue_DestroyTimer(ex, msgQueue, pongTimer, NULL);
        pongTimer = NULL;
    }

    replyReceived = tibtrue;

    printf("received reply message:\n");
    printMessage(ex, msgs[0]);

    // any exception that happens in the callback does not
    // propagate to the caller of the tibEventQueue_Dispatch
    if (tibEx_GetErrorCode(ex) != TIB_OK)
        tibAux_PrintException(ex);
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
    tibEventQueue           queue   = NULL;
    tibProperties           props   = NULL;
    tibSubscriber           sub     = NULL;
    tibInbox                inbox   = NULL;
    tibMessage              msg     = NULL;
    tibEx                   ex      = tibEx_Create();

    printf("#\n# %s\n#\n# %s\n#\n", argv[0], tib_Version());

    parseArgs(argc, argv);

    tib_Open(ex, TIB_COMPATIBILITY_VERSION);

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

    // set the client label
    if (clientLabel != NULL)
        tibProperties_SetString(ex, props, TIB_REALM_PROPERTY_STRING_CLIENT_LABEL, clientLabel);
    else
        tibProperties_SetString(ex, props, TIB_REALM_PROPERTY_STRING_CLIENT_LABEL, argv[0]);

    realm = tibRealm_Connect(ex, realmServer, appName, props);
    tibProperties_Destroy(ex, props);

    queue = tibEventQueue_Create(ex, realm, NULL);

    sub = tibSubscriber_CreateOnInbox(ex, realm, endpointName, NULL);
    inbox = tibSubscriber_GetInbox(ex, sub);
    tibEventQueue_AddSubscriber(ex, queue, sub, onMsg, NULL);

    pub = tibPublisher_Create(ex, realm, endpointName, NULL);

    msg = tibMessage_Create(ex, realm, "Format-1");

    // set by name since performance is not demonstrated here
    tibMessage_SetString(ex, msg, "My-String", "request");
    tibMessage_SetLong(ex, msg, "My-Long", 10);

    tibMessage_SetOpaque(ex, msg, "My-Opaque", "payload", sizeof("payload"));

    // put our inbox in the request messge
    // set by name since performance is not demonstrated here
    tibMessage_SetInbox(ex, msg, "My-Inbox", inbox);

    if (pingInterval)
        pongTimer = tibEventQueue_CreateTimer(ex, queue, pingInterval,
                                              onPongTimeout, msg);

    printf("sending request message:\n");
    printMessage(ex, msg);
    
    // send the request message
    tibPublisher_Send(ex, pub, msg);

    // wait for the reply message. it will be sent to our inbox
    while (tibEx_GetErrorCode(ex) == TIB_OK && !replyReceived)
        tibEventQueue_Dispatch(ex, queue, TIB_TIMEOUT_WAIT_FOREVER);

    // Clean up
    if (queue && sub)
        tibEventQueue_RemoveSubscriber(ex, queue, sub, NULL);
    if (pongTimer)
        tibEventQueue_DestroyTimer(ex, queue, pongTimer, NULL);
    tibEventQueue_Destroy(ex, queue, NULL);
    tibSubscriber_Close(ex, sub);
    tibPublisher_Close(ex, pub);
    tibMessage_Destroy(ex, msg);
    tibRealm_Close(ex, realm);
    tib_Close(ex);

    // Check if it worked before destroying the exception
    if (tibEx_GetErrorCode(ex) != TIB_OK)
        tibAux_PrintException(ex);
    tibEx_Destroy(ex);

    return 0;
}
