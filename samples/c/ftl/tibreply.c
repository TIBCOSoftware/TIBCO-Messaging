/*
 * Copyright (c) 2010-2017 TIBCO Software Inc.
 * Licensed under a BSD-style license. Refer to [LICENSE]
 * For more information, please contact:
 * TIBCO Software Inc., Palo Alto, California, USA
 */

/*
 * This is a sample C replier program which is used with the tibrequest program
 * to demonstrate the basic use of inboxes.
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "tib/ftl.h"

#include "tibaux.h"

#define DEFAULT_URL "http://localhost:8080"

const char*     realmServer             = DEFAULT_URL;
const char*     appName                 = "tibreply";
const char*     endpointName            = "tibreply-endpoint";
const char*     user                    = "guest";
const char*     password                = "guest-pw";
const char      *identifier             = NULL;
const char      *secondary              = NULL;
const char      *clientLabel            = NULL;

const char      *trcLevel               = NULL;
tibint32_t      count                   = 1;
tibint32_t      received                = 0;
tibbool_t       running                 = tibtrue;

tibPublisher    pub                     = NULL;
tibMessage      replyMsg                = NULL;

void
usage(int argc, char** argv)
{
    printf("Usage: %s [options] url\n", argv[0]);
    printf("Default url is "DEFAULT_URL"\n");
    printf("   -a, --application <name>\n"
           "       --client-label <label>\n"
           "   -c, --count <int>\n"
           "   -e, --endpoint <name>\n"
           "   -h, --help\n"
           "   -id, --identifier\n"
           "   -p, --password <string>\n"
           "   -s, --secondary <string>\n"
           "   -t, --trace <level>\n"
           "   -u, --user <string>\n"
           "  where --trace can be:\n"
           "    off, severe, warn, info, debug, verbose\n");

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
        if (tibAux_GetInt(&i ,"--count", "-c", &count))
            continue;
        if (tibAux_GetFlag(&i, "--help", "-h"))
           usage(argc, argv);
        if (tibAux_GetString(&i, "--identifier", "-id", &identifier))
            continue;
        if (tibAux_GetString(&i, "--password", "-p", &password))
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
onMsg(
    tibEx                       ex,
    tibEventQueue               msgQueue,
    tibint32_t                  msgNum,
    tibMessage                  *msgs,
    void                        **closures)
{
    tibInbox                    inbox = NULL;
    tibMessage                  msg;
    tibMessageIterator          msgIter;
    tibFieldRef                 fieldRef;
    tibFieldType                fieldType;
    const char*                 fieldTypeName;
    const char*                 fieldName;
    tibint32_t                  opaqueSize;

    if (msgNum != 1)
    {
        printf("Received %d messages instead of one.\n", msgNum);
        running = tibfalse;
    }
    received++;
    printf("Received request # %d\n", received);

    msg = msgs[0];

    // use a message iterator to print fields that have been set
    // in the message
    msgIter = tibMessageIterator_Create(ex, msg);

    while (tibMessageIterator_HasNext(ex, msgIter))
    {
        // retrieve the field reference from the iterator to access
        // fields.  Note that the field reference does not
        // have to be destroyed.
        fieldRef  = tibMessageIterator_GetNext(ex, msgIter);

        // extract the information we need to print the fields
        fieldName = tibFieldRef_GetFieldName(ex, fieldRef);
        fieldType = tibMessage_GetFieldType(ex, msg, fieldName);
        fieldTypeName = tibFieldType_GetAsString(ex, fieldType);
                
        // do not continue if we have encountered an error
        if (tibEx_GetErrorCode(ex) != TIB_OK)
            break;

        printf("Name: %s\n", fieldName);
        printf("    Type: %s\n", fieldTypeName);
        printf("    Value: ");
        
        switch (fieldType)
        {
            case TIB_FIELD_TYPE_LONG:
                printf("%"auxPRId64"\n", tibMessage_GetLongByRef(ex, msg, fieldRef));
                break;
            case TIB_FIELD_TYPE_STRING:
                printf("%s\n", tibMessage_GetStringByRef(ex, msg, fieldRef));
                break;
            case TIB_FIELD_TYPE_OPAQUE:
                (void)tibMessage_GetOpaqueByRef(ex, msg, fieldRef, &opaqueSize);
                printf("opaque, %d bytes\n", opaqueSize);
                break;
            case TIB_FIELD_TYPE_INBOX:
                inbox = tibMessage_GetInboxByRef(ex, msg, fieldRef);
                printf("(inbox)\n");
                break;
            default:
                printf("Unhandled type\n");
                break;
        }
    }
    tibMessageIterator_Destroy(ex, msgIter);

    // set by name since performance is not demonstrated here
    tibMessage_SetString(ex, replyMsg, "My-String", "reply");

    printf("Sending reply # %d\n", received);  
    tibPublisher_SendToInbox(ex, pub, inbox, replyMsg);

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
    tibSubscriber           sub     = NULL;
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

    sub = tibSubscriber_Create(ex, realm, endpointName, NULL, NULL);
    tibEventQueue_AddSubscriber(ex, queue, sub, onMsg, NULL);

    pub = tibPublisher_Create(ex, realm, endpointName, NULL);
    
    replyMsg = tibMessage_Create(ex, realm, "Format-1");
    
    while (tibEx_GetErrorCode(ex) == TIB_OK && received < count && running)
        tibEventQueue_Dispatch(ex, queue, TIB_TIMEOUT_WAIT_FOREVER);

    // Clean up
    if (queue && sub)
        tibEventQueue_RemoveSubscriber(ex, queue, sub, NULL);
    tibEventQueue_Destroy(ex, queue, NULL);
    tibSubscriber_Close(ex, sub);
    tibPublisher_Close(ex, pub);
    tibMessage_Destroy(ex, replyMsg);
    tibRealm_Close(ex, realm);
    tib_Close(ex);

    // Check if it worked before destroying the exception
    if (tibEx_GetErrorCode(ex) != TIB_OK)
        tibAux_PrintException(ex);
    tibEx_Destroy(ex);
    
    return 0;
}
