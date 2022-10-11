/*
 * Copyright (c) 2011-2017 TIBCO Software Inc.
 * Licensed under a BSD-style license. Refer to [LICENSE]
 * For more information, please contact:
 * TIBCO Software Inc., Palo Alto, California, USA
 */

/*
 * This is a sample of a basic FTL GROUP membership program which joins a group
 * and displays changes to the group membership status for a given amount of
 * time. After that, it leaves the group, cleans up and exits.
 */

#include <stdlib.h>
#include <stdio.h>
#include <stddef.h>
#include <string.h>
#include <ctype.h>
#include <time.h>

#include "tib/ftl.h"
#include "tibgroup/group.h"
#include "tibaux.h"

#define DEFAULT_URL "http://localhost:8080"
#define DEFAULT_SECURE_URL "https://localhost:8080"

const char      *appName        = "tibrecv";
const char      *realmServer    = DEFAULT_URL;
const char      *trcLevel       = NULL;
const char      *user           = "guest";
const char      *password       = "guest-pw";
const char      *identifier     = NULL;
const char      *secondary      = NULL;
const char      *groupName      = "group";
tibdouble_t     duration        = 100;
tibdouble_t     activation      = 0;
tibbool_t       hasDescriptor   = tibfalse;
tibbool_t       observerOnly    = tibfalse;
const char      *memberDescriptor = NULL;
tibbool_t       trustAll        = tibfalse;
const char      *trustFile      = NULL;
const char      *clientLabel    = NULL;

void
usage(
    int         argc,
    char        **argv)
{
    printf("Usage: %s [options] url\n", argv[0]);
    printf("Default url is "DEFAULT_URL"\n");
    printf("  where options can be:\n");
    printf("    -a, --application <name>\n"
           "        --client-label <label>\n"
           "    -h, --help\n"
           "    -id,--identifier <string>\n"
           "    -s, --secondary <string>\n"
           "    -t, --trace <level>\n"
           "        --trustall\n"
           "        --trustfile <file>\n"
           "    -u, --user <string>\n"
           "    -g, --group <string>\n"
           "    -d, --duration <seconds>\n"
           "    -i, --interval <seconds>\n"
           "    -md, --memberDescriptor <string> - the string that identifies this member to other members,\n"
           "    *                    If the string value is not provided then this member is anonymous\n"
           "    -o,  --observerOnly - this member is joining as observer, thus it doesn't receive an ordinal and doesn't generate up/down events\n"
           "  where --trace can be:\n"
           "    off, severe, warn, info, debug, verbose\n");

    exit(1);
}

void
parseArgs(int argc, char** argv)
{
    int    i = 1;
    int    tmp;

    tibAux_InitGetArgs(argc, argv, usage);

    for (i = 1;  i < argc;  i++)
    {
        if (tibAux_GetString(&i, "--application", "-a", &appName))
            continue;
        if (tibAux_GetString(&i, "--client-label", "--client-label", &clientLabel))
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
        if (tibAux_GetString(&i, "--group", "-g", &groupName))
            continue;
        if (tibAux_GetInt(&i, "--duration", "-d", &tmp))
        {
            duration = tmp;
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
        if (tibAux_GetFlag(&i, "--memberDescriptor", "-md"))
        {
            if (!observerOnly)
            {
                hasDescriptor = tibtrue;
            }
            else
            {
                printf("invalid option: %s conflicting with observerOnly", argv[i]);
                usage(argc, argv);
            }
            tibAux_GetOptionalString(&i, "--memberDescriptor", "-md", &memberDescriptor);
            continue;
        }
        if (tibAux_GetFlag(&i, "--observerOnly", "-o"))
        {
            observerOnly = tibtrue;
            continue;                
        }
        if (tibAux_GetInt(&i, "--interval", "-i", &tmp))
        {
            activation = tmp;
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
printException(
    tibEx       e)
{
    tibint32_t  length                  = tibEx_ToString(e, NULL, 0);
    char        *exceptionString        = malloc(length);

    if (exceptionString != NULL)
    {
        tibEx_ToString(e, exceptionString, length);

        printf("%s\n", exceptionString);

        free(exceptionString);
    }

    tibEx_Clear(e);
}

void
printMessage(
    tibEx               e,
    tibMessage          message)
{
    tibint32_t          length  = 0;
    char                *buffer = 0;

    length = tibMessage_ToString(e, message, NULL, 0);

    buffer = (char*)malloc(length);
    if (buffer != NULL)
    {
        (void)tibMessage_ToString(e, message, buffer, length);

        printf("%s\n", buffer);

        free(buffer);
    }

    if (tibEx_GetErrorCode(e) != TIB_OK)
        printException(e);
}

/*
 * Group Member Status is passed from group server to members as a long field
 * of the group members status message. This function does the conversion 
 * from long to tibMemberEvent 
 */

tibbool_t memberEventValueOf(tibint64_t memberStatus, tibMemberEvent *value )
{
    tibbool_t      validValue = tibtrue;
    switch((int)memberStatus)
    {        
        case 0:
            *value = GROUP_MEMBER_JOINED;
            break;
        case 1:
            *value = GROUP_MEMBER_LEFT;
            break;
        case 2:
            *value = GROUP_MEMBER_DISCONNECTED;
            break;
        default:
            validValue = tibfalse;
            break;
    }

    return validValue;
}

void printValueOf(tibMemberEvent memberStatus)
{
    switch(memberStatus)
    {
        case GROUP_MEMBER_JOINED:
            printf("GROUP_MEMBER_JOINED");
            break;
        case GROUP_MEMBER_LEFT:
            printf("GROUP_MEMBER_LEFT");
            break;
        case GROUP_MEMBER_DISCONNECTED:
            printf("GROUP_MEMBER_DISCONNECTED");
            break;
        default:
            printf("INVALID STATE");
            break;
    }
}

/*
 * This call returns the tibMemberEvent
 */
tibbool_t getMemberStatusEvent(tibEx e, tibMessage statusMsg, tibMemberEvent *value)
{
    if (tibMessage_IsFieldSet(e, statusMsg, TIB_GROUP_FIELD_GROUP_MEMBER_EVENT))
    {
       if (memberEventValueOf(tibMessage_GetLong(e, statusMsg, TIB_GROUP_FIELD_GROUP_MEMBER_EVENT), value))
       {
           return tibtrue;
       }
    }
    return tibfalse;
}

/*
 * This call returns list of status messages from a status advisory message
 */
tibMessage* getMembersStatusList(tibEx e, tibMessage statusAdvMsg, tibint32_t* arraySize)
{
    tibMessage* result = NULL;
    tibint32_t     retArraySize = 0;
    if (tibMessage_IsFieldSet(e, statusAdvMsg, TIB_GROUP_FIELD_GROUP_MEMBER_STATUS_LIST))
    {
        result = (tibMessage*)tibMessage_GetArray(e, statusAdvMsg,
                                                  TIB_FIELD_TYPE_MESSAGE_ARRAY,
                                                  TIB_GROUP_FIELD_GROUP_MEMBER_STATUS_LIST,
                                                  &retArraySize);
    }
    *arraySize = retArraySize;

    return result;
}

/*
 * This call returns the descriptor message for the member that generated the event
 * The descriptor is null if the member did not provided one at the time of joining 
 */
tibMessage getMembersDescriptor(tibEx e, tibMessage statusMsg)
{
    tibMessage result = NULL;
    if (tibMessage_IsFieldSet(e, statusMsg, TIB_GROUP_FIELD_GROUP_MEMBER_DESCRIPTOR))
    {
        result = tibMessage_GetMessage(e, statusMsg, TIB_GROUP_FIELD_GROUP_MEMBER_DESCRIPTOR);
    }
    return result;
}


/*
 * Group Member Server connection status is passed from group library
 * as a long field of status advisory message. This function does the conversion 
 * from long to tibMemberServerConnection 
 */

tibbool_t
serverConnectionValueOf(tibint64_t serverConnStatus, tibMemberServerConnection *value)
{
    tibbool_t      validValue = tibtrue;
    switch((int)serverConnStatus)
    {        
        case 0:
            *value = GROUP_SERVER_UNAVAILABLE;
            break;
        case 1:
            *value = GROUP_SERVER_AVAILABLE;
            break;
        default:
            validValue = tibfalse;
            break;
    }

    return validValue;
}

tibbool_t
serverConnectionValid(tibEx e, tibMessage statusMsg)
{
    tibMemberServerConnection value;
    if (serverConnectionValueOf(tibMessage_GetLong(e, statusMsg, TIB_GROUP_FIELD_GROUP_SERVER_AVAILABLE), &value))
    {
        if (value == GROUP_SERVER_AVAILABLE)
            return tibtrue;
        else
            return tibfalse;
    }

    return tibfalse;
}

void
onAdvStatus(
    tibEx               ex,
    tibEventQueue       queue,
    tibint32_t          msgNum,
    tibMessage          *msgs,
    void                **closures)
{
    int i;
    tibMessage  msg;
    tibMessage  memberDescMsg;
    tibMessage* membersStatusList = NULL;
    tibint32_t  statusSize = 0;
    int         members    = 0;
    const char  *svalue = NULL;

    for (i = 0; i < msgNum; i++)
    {
        msg = msgs[i];
        printf("status advisory:");

        svalue = tibMessage_GetString(ex, msg, TIB_ADVISORY_FIELD_SEVERITY);
        printf("  %s: %s\n", TIB_ADVISORY_FIELD_SEVERITY, svalue);

        svalue = tibMessage_GetString(ex, msg, TIB_ADVISORY_FIELD_MODULE);
        printf("  %s: %s\n", TIB_ADVISORY_FIELD_MODULE, svalue);

        svalue = tibMessage_GetString(ex, msg, TIB_ADVISORY_FIELD_NAME);
        printf("  %s: %s\n", TIB_ADVISORY_FIELD_NAME, svalue);

        svalue = tibMessage_GetString(ex, msg, TIB_GROUP_ADVISORY_FIELD_GROUP);
        printf("  %s: %s\n", TIB_GROUP_ADVISORY_FIELD_GROUP, svalue);

        if (serverConnectionValid(ex, msg))
        {
            membersStatusList = NULL;
            membersStatusList = getMembersStatusList(ex, msg, &statusSize);
            if (membersStatusList == NULL)
            {
                printf("No other members have joined the group yet\n");
            }
            else
            {
                for(members = 0; members < statusSize; members++)
                {
                    tibMemberEvent statusEvent;
                    memberDescMsg = NULL;
                    if (getMemberStatusEvent(ex, membersStatusList[members], &statusEvent))
                    {
                       printf("Members status: ");
                       printValueOf(statusEvent);
                       printf("\n");
                    }
                    memberDescMsg = getMembersDescriptor(ex, membersStatusList[members]);
                    if (memberDescMsg != NULL)
                    {
                        printMessage(ex, memberDescMsg);
                    }
                    else
                    {
                        printf("This member is anonymous\n");
                    }
                }
            }
        }
        else
        {
           printf("Connection to server LOST -- reset MEMBER LIST!!!!!\n"); 
        }
    }

}

void
onAdvisory(
    tibEx               ex,
    tibEventQueue       queue,
    tibint32_t          msgNum,
    tibMessage          *msgs,
    void                **closures)
{
    tibint32_t          i;
    tibMessage          msg;
    const char          *svalue = NULL;
    tibint64_t          lvalue = 0;
    
    for (i = 0; i < msgNum; i++)
    {
        msg = msgs[i];
        printf("advisory:");

        svalue = tibMessage_GetString(ex, msg, TIB_ADVISORY_FIELD_SEVERITY);
        printf("  %s: %s\n", TIB_ADVISORY_FIELD_SEVERITY, svalue);

        svalue = tibMessage_GetString(ex, msg, TIB_ADVISORY_FIELD_MODULE);
        printf("  %s: %s\n", TIB_ADVISORY_FIELD_MODULE, svalue);

        svalue = tibMessage_GetString(ex, msg, TIB_ADVISORY_FIELD_NAME);
        printf("  %s: %s\n", TIB_ADVISORY_FIELD_NAME, svalue);

        svalue = tibMessage_GetString(ex, msg, TIB_GROUP_ADVISORY_FIELD_GROUP);
        printf("  %s: %s\n", TIB_GROUP_ADVISORY_FIELD_GROUP, svalue);

        lvalue = tibMessage_GetLong(ex, msg, TIB_GROUP_ADVISORY_FIELD_ORDINAL);
        printf("  %s: %"auxPRId64"\n", TIB_GROUP_ADVISORY_FIELD_ORDINAL, lvalue);
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
    tibProperties       realmProps;
    tibProperties       groupProps;
    tibRealm            realm;
    tibContentMatcher   am = NULL;
    tibContentMatcher   groupStatusMatcher = NULL;
    tibSubscriber       groupStatusSubscriber = NULL;
    tibSubscriber       adv = NULL;
    tibEventQueue       queue;
    tibGroup            group;
    tibint64_t          start;
    char*               name = NULL;
    int                 nameLen;
    char                str[256];
    tibMessage          descriptorMsg = NULL;

    printf("#\n# %s\n#\n# (FTL) %s\n# (FTL GROUP) %s\n#\n", argv[0],
        tib_Version(), tibGroup_Version());

    parseArgs(argc, argv);

    ex = tibEx_Create();
    if (ex == NULL)
        memFail("ex");

    tib_Open(ex, TIB_COMPATIBILITY_VERSION);

    // Set global trace to specified level
    if (trcLevel != NULL)
        tib_SetLogLevel(ex, trcLevel);

    // Get a single realm per process
    realmProps = tibProperties_Create(ex);
    tibProperties_SetString(ex, realmProps,
                            TIB_REALM_PROPERTY_STRING_USERNAME, user);
    tibProperties_SetString(ex, realmProps,
                            TIB_REALM_PROPERTY_STRING_USERPASSWORD, password);
    if (identifier)
        tibProperties_SetString(ex, realmProps,
                                TIB_REALM_PROPERTY_STRING_APPINSTANCE_IDENTIFIER, identifier);
    if (secondary)
        tibProperties_SetString(ex, realmProps,
                                TIB_REALM_PROPERTY_STRING_SECONDARY_SERVER, secondary);

    if (trustAll || trustFile)
    {
        if (!realmServer)
            realmServer = DEFAULT_SECURE_URL;

        if (trustAll)
        {
            tibProperties_SetLong(ex, realmProps, TIB_REALM_PROPERTY_LONG_TRUST_TYPE,
                                        TIB_REALM_HTTPS_CONNECTION_TRUST_EVERYONE);
        }
        else
        {
            tibProperties_SetLong(ex, realmProps, TIB_REALM_PROPERTY_LONG_TRUST_TYPE,
                    TIB_REALM_HTTPS_CONNECTION_USE_SPECIFIED_TRUST_FILE);
            tibProperties_SetString(ex, realmProps, TIB_REALM_PROPERTY_STRING_TRUST_FILE, trustFile);
        }
    }
    else
    {
        if (!realmServer)
            realmServer = DEFAULT_URL;
    }

    // set the client label
    if (clientLabel != NULL)
        tibProperties_SetString(ex, realmProps, TIB_REALM_PROPERTY_STRING_CLIENT_LABEL, clientLabel);
    else
        tibProperties_SetString(ex, realmProps, TIB_REALM_PROPERTY_STRING_CLIENT_LABEL, argv[0]);

    realm = tibRealm_Connect(ex, realmServer, appName, realmProps);
    tibProperties_Destroy(ex, realmProps);

    // Create a group advisory matcher
    tibAux_snprintf(str, sizeof(str), "{\"%s\":\"%s\"}", 
                  TIB_ADVISORY_FIELD_NAME, TIB_GROUP_ADVISORY_NAME_ORDINAL_UPDATE);

    queue = tibEventQueue_Create(ex, realm, NULL);

    groupProps = tibProperties_Create(ex);

    if (observerOnly)
    {
        tibProperties_SetBoolean(ex, groupProps,
                                 TIB_GROUP_PROPERTY_BOOLEAN_OBSERVER,
                                 tibtrue);
    }
    else
    {
        am = tibContentMatcher_Create(ex, realm, str);

        // Create an advisory subscriber
        adv = tibSubscriber_Create(ex, realm, TIB_ADVISORY_ENDPOINT_NAME, am, NULL);
        tibEventQueue_AddSubscriber(ex, queue, adv, onAdvisory, NULL);

        if (hasDescriptor)
        {
            if (memberDescriptor != NULL)
            {
                descriptorMsg = tibMessage_Create(ex, realm, "identifier_format");
                tibMessage_SetString(ex, descriptorMsg, "my_descriptor_string",
                                     memberDescriptor);          
            }
            tibProperties_SetMessage(ex, groupProps, 
                                 TIB_GROUP_PROPERTY_MESSAGE_MEMBER_DESCRIPTOR,
                                 descriptorMsg);
        }
    }

    if (observerOnly || hasDescriptor)
    {
        tibAux_snprintf(str, sizeof(str), "{\"%s\":\"%s\"}",
                TIB_ADVISORY_FIELD_NAME, TIB_GROUP_ADVISORY_NAME_GROUP_STATUS);
        groupStatusMatcher = tibContentMatcher_Create(ex, realm, str);
        groupStatusSubscriber = tibSubscriber_Create(ex, realm,
                                                      TIB_ADVISORY_ENDPOINT_NAME,
                                                      groupStatusMatcher,
                                                      NULL);
        tibEventQueue_AddSubscriber(ex, queue, groupStatusSubscriber,
                                    onAdvStatus, NULL);     

    }

    // Join the specified group
    if (activation > 0)
        tibProperties_SetDouble(ex, groupProps, 
                                TIB_GROUP_PROPERTY_DOUBLE_ACTIVATION_INTERVAL, 
                                activation);

    group = tibGroup_Join(ex, realm, groupName, groupProps);
    tibProperties_Destroy(ex, groupProps);

    nameLen = tibGroup_GetName(ex, group, NULL, 0);
    name = calloc(nameLen, sizeof(char));
    tibGroup_GetName(ex, group, name, nameLen);
    printf("joined group: '%.*s'\n", nameLen, name);
    free(name);

    // Begin dispatching messages
    printf("waiting for %f seconds\n", duration);
    start = tibAux_GetTime();
    while (tibEx_GetErrorCode(ex) == TIB_OK && duration > 0)
    {
        tibEventQueue_Dispatch(ex, queue, duration);
        duration -= (tibAux_GetTime() - start) * tibAux_GetTimerScale();
        start = tibAux_GetTime();
    }

    printf("done waiting\n");

    // Leave the Group
    tibGroup_Leave(ex, group);

    // Clean up
    if (queue && adv)
    {
        tibEventQueue_RemoveSubscriber(ex, queue, adv, NULL);
        tibSubscriber_Close(ex, adv);
        tibContentMatcher_Destroy(ex, am);
    }

    tibEventQueue_Destroy(ex, queue, NULL);

    if (groupStatusMatcher != NULL)
    {
        tibContentMatcher_Destroy(ex, groupStatusMatcher);
    }
    if (groupStatusSubscriber != NULL)
    {
        tibSubscriber_Close(ex, groupStatusSubscriber);
    }
    tibRealm_Close(ex, realm);
    tib_Close(ex);

    // Check if it worked before destroying the exception
    if (tibEx_GetErrorCode(ex) != TIB_OK)
        tibAux_PrintException(ex);
    tibEx_Destroy(ex);

    return 0;
}
