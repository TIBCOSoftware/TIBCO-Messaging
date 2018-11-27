/*
 * Copyright (c) 2010-2017 TIBCO Software Inc.
 * Licensed under a BSD-style license. Refer to [LICENSE]
 * For more information, please contact:
 * TIBCO Software Inc., Palo Alto, California, USA
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

const char      *appName        = "tibmapget";
const char      *realmServer    = NULL;
const char      *mapEndpoint    = "tibmapget-endpoint";
const char      *trcLevel       = NULL;
const char      *user           = "guest";
const char      *password       = "guest-pw";
const char      *identifier     = NULL;
const char      *secondary      = NULL;
const char      *mapName        = "tibmap";
char            *keys           = "";
char            *keysStart      = NULL;
const char      *keyList        = NULL;
tibbool_t       removeMap       = tibfalse;
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
    printf("    -a,  --application <name>\n"
           "         --client-label <label>\n"
           "    -e,  --endpoint <name>\n"
           "    -h,  --help\n"
           "    -id, --identifier <name>\n"
           "    -l,  --keyList <string>[,<string>]\n"
           "            if not provided the entire key space will be iterated\n"
           "    -n,  --mapname <name>\n"
           "    -p,  --password <string>\n"
           "    -u,  --user <string>\n"
           "    -r   --removemap \n"
           "            the mapname command line arg needs to be specified as well\n"
           "    -s,  --secondary <string>\n"
           "    -t,  --trace <level>\n"
           "            where <level> can be:\n"
           "                  off, severe, warn, info, debug, verbose\n"
           "         --trustall\n"
           "         --trustfile <file>\n");

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
        if (tibAux_GetString(&i, "--endpoint", "-e", &mapEndpoint))
            continue;
        if (tibAux_GetFlag(&i, "--help", "-h"))
            usage(argc, argv);
        if (tibAux_GetString(&i, "--identifier", "-id", &identifier))
            continue;
        if (tibAux_GetString(&i, "--keyList", "-l", &keyList))
            continue;
        if (tibAux_GetString(&i, "--mapname", "-n", &mapName))
            continue;
        if (tibAux_GetString(&i, "--password", "-p", &password))
            continue;
        if (tibAux_GetString(&i, "--secondary", "-s", &secondary))
            continue;
        if (tibAux_GetString(&i, "--trace", "-t", &trcLevel))
            continue;
        if (tibAux_GetString(&i, "--user", "-u", &user))
            continue;
        if (tibAux_GetFlag(&i, "--removeMap", "-r"))
        {
            removeMap = tibtrue;
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

    if (keyList)
    {
        keysStart = tibAux_strdup(keyList);
        keys  = keysStart;
    }
}

void
memFail(
    const char          *obj)
{
    printf("memory allocation failed for %s\n", obj);
    exit(-1);
}

char*
msgToString(
    tibEx               ex,
    tibMessage          msg)
{
    tibint32_t          len = 0;
    char                *buffer = 0;

    // get the size of output string
    len = tibMessage_ToString(ex, msg, NULL, 0);
    buffer = (char*)malloc(len);
    if (buffer)
        (void)tibMessage_ToString(ex, msg, buffer, len);

    if (tibEx_GetErrorCode(ex) != TIB_OK)
        tibAux_PrintException(ex);
    fflush(stdout);

    return buffer;
}

int
main(int argc, char** argv)
{
    tibEx               ex;
    tibProperties       props;
    tibRealm            realm;
    tibMap              tibmap;
    tibMapIterator      iter;
    tibMessage          msg;

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

    // create a dynamic map in tibstore if it doesn't exist.
    tibmap = tibRealm_CreateMap(ex, realm, mapEndpoint, mapName, NULL);
    CHECK(ex);

    if (keyList)
    {
        char* key = tibAux_strtok_r(keys, ",", &keys);
    
        do
        {
            printf("getting value for key %s\n", key); fflush(stdout);
    
            msg = tibMap_Get(ex, tibmap, key);
            if (msg != NULL)
            {    
                char *msgString = msgToString(ex, msg);
                printf("key: %s, value = %s\n", key, msgString);
                free(msgString);
            }
            else
            {
                printf("currently no key '%s' in the map\n", key);
            }
        }
        while ((key = tibAux_strtok_r(keys, ",", &keys)));
    }
    else 
    {
        tibMessage value;
        char       *msgString;
        const char *key;

        printf("\niterating keys in map: [%s]\n\n", mapName); fflush(stdout);

        iter = tibMap_CreateIterator(ex, tibmap, NULL);
        while(tibMapIterator_Next(ex, iter))
        {
            key   = tibMapIterator_CurrentKey(ex, iter);
            value = tibMapIterator_CurrentValue(ex, iter);
            
            msgString = msgToString(ex, value);
            printf("key: %s, value = %s\n", key, msgString);
            free(msgString);
        }

        // destroy the iterator.
        tibMapIterator_Destroy(ex, iter);
    }

    // close the map
    tibMap_Close(ex, tibmap);

    // if removeMap is true and mapName is specified then remove the map.
    if (mapName && removeMap)
    {
        printf("Removing map '%s'\n", mapName);    
        tibRealm_RemoveMap(ex, realm, mapEndpoint, mapName, NULL);
    }

    tibRealm_Close(ex, realm);
    tib_Close(ex);

    // Check if it worked before destroying the exception
    CHECK(ex);
    tibEx_Destroy(ex);

    return 0;
}
