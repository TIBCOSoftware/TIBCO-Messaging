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

const char      *appName        = "tibmapset";
const char      *realmServer    = NULL;
const char      *mapEndpoint    = "tibmapset-endpoint";
const char      *trcLevel       = NULL;
const char      *user           = "guest";
const char      *password       = "guest-pw";
const char      *identifier     = NULL;
const char      *formatName     = "Format-1";
const char      *secondary      = NULL;
const char      *mapName        = "tibmap";
char            *keys           = "";
char            *keysStart      = NULL;
const char      *keyList        = "key1,key2,key3";
tibbool_t       removeMap       = tibfalse;
tibbool_t       trustAll        = tibfalse;
const char      *trustFile      = NULL;
const char      *clientLabel    = NULL;
tibMessage      msg             = NULL;
tibRealm        realm;
tibint32_t      count = 0;

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
           "    -f,  --format <name>\n"
           "    -h,  --help\n"
           "    -id, --identifier <name>\n"
           "    -l,  --keyList <string>[,<string>]\n"
           "    -n,  --mapname <name>\n"
           "    -p,  --password <string>\n"
           "    -u,  --user <string>\n"
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
        if (tibAux_GetString(&i ,"--format", "-f", &formatName))
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

void
initMessage(
    tibEx               ex,
    char                *key)
{
    if (msg == NULL)
        msg = tibMessage_Create(ex, realm, formatName);
    else
        tibMessage_ClearAllFields(ex, msg);

    tibMessage_SetLong(ex, msg, "My-Long", ++count);
}

int
main(int argc, char** argv)
{
    tibEx               ex;
    tibProperties       props;
    tibMap              tibmap;

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

    // create a dynamic map in tibstore that will store the key-value 
    // pairs
    tibmap = tibRealm_CreateMap(ex, realm, mapEndpoint, mapName, NULL);
    CHECK(ex);

    if (keyList)
    {
        char* key = tibAux_strtok_r(keys, ",", &keys);
    
        do
        {
            printf("setting value for key %s\n", key); fflush(stdout);
            initMessage(ex, key);

            tibMap_Set(ex, tibmap, key, msg);
            CHECK(ex);
        }
        while ((key = tibAux_strtok_r(keys, ",", &keys)));
    }

    // close the map
    tibMap_Close(ex, tibmap);

    tibRealm_Close(ex, realm);
    tib_Close(ex);

    // Check if it worked before destroying the exception
    CHECK(ex);
    tibEx_Destroy(ex);

    return 0;
}
