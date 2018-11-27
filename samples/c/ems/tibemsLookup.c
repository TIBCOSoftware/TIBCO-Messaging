/* 
 * Copyright (c) 2001-$Date: 2016-12-13 17:00:37 -0600 (Tue, 13 Dec 2016) $ TIBCO Software Inc. 
 * Licensed under a BSD-style license. Refer to [LICENSE]
 * For more information, please contact:
 * TIBCO Software Inc., Palo Alto, California, USA
 * 
 * $Id: tibemsLookup.c 90180 2016-12-13 23:00:37Z olivierh $
 * 
 */

/*
 * The sample allows you to query the TIBCO EMS Server or
 * external LDAP server for factories and destinations.
 *
 * Notice that the specified destination should exist in your configuration
 * or your topics/queues configuration file should allow
 * creation of the specified topic. Sample configuration supplied with
 * the TIBCO Enterprise for EMS distribution allows creation of any
 * destination.
 *
 * When using Factory or Destination lookup from the EMS Server, the
 * factory or destination must already exist in the server or the lookup
 * will fail.  See the tibemsadmin help for more help on creating factories
 * and destination objects in the TIBCO EMS Server.
 *
 * Usage:  tibemsLookup  [options]
 *
 *  where options are:
 *
 *   -server    <server-url>  Server URL.
 *                            If not specified this sample assumes a
 *                            serverUrl of "tcp://localhost:7222"
 *   -user      <user-name>   User name. Default is null.
 *   -password  <password>    User password. Default is null.
 *   ...
 *
 * Example usage:
 *
 * lookup from EMS sever:
 *
 * tibemsLookup -server "tcp://localhost:7222" -lookupdest testQueue
 * -lookupfact testFact 
 *
 * lookup from directory service: 
 *
 * tibemsLookup -ldap_server "ldap://myserver:389" -ldap_basedn "o=JNDITutorial"
 * -ldap_principal "cn=Directory Manager" -ldap_credential passwd 
 * -ldap_sscope subtree -lookupdest cn=testQueue -lookupfact cn=testFact
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "tibemsUtilities.h"
#include "tibems/confact.h"

/*-----------------------------------------------------------------------
 * Parameters
 *----------------------------------------------------------------------*/

char*                           serverUrl      = NULL;
char*                           userName       = NULL;
char*                           password       = NULL;
char*                           pk_password    = NULL;
char                            nameBuf[1024];
char*                           lookupdest     = NULL;
char*                           lookupfact     = NULL;

/*-----------------------------------------------------------------------
 * Variables
 *----------------------------------------------------------------------*/
tibemsDestination               destination    = NULL;
tibemsConnectionFactory         cf             = NULL;

tibemsSSLParams                 sslParams      = NULL;

tibemsLookupParams              lookupParams   = NULL;

tibemsErrorContext              errorContext   = NULL;

/*-----------------------------------------------------------------------
 * usage
 *----------------------------------------------------------------------*/
void usage() 
{
    printf("\nUsage: tibemsLookup [options] [ssl options]\n");
    printf("   where options are:\n");
    printf("\n");
    printf("   -server   <server URL>  - EMS server URL, default is local server\n");
    printf("   -user     <user name>   - user name, default is null\n");
    printf("   -password <password>    - password, default is null\n");
    printf("   -lookupdest <name>      - lookup the named destination in the EMS server\n");
    printf("   -lookupfact <name>      - lookup the named factory in the EMS server\n");
    printf("   -help-ssl               - help on ssl parameters\n");
    printf("   -help-ldap              - help on ldap parameters\n");
    exit(0);
}
    
void
ldapUsage()
{
    printf("\n");
    printf("   where ldap options are:\n");
    printf("   -ldap_server <url>       - ldap server url\n");
    printf("   -ldap_basedn <dn>        - ldap base dn\n");
    printf("   -ldap_principal <pdn>    - ldap administrator\n");
    printf("   -ldap_credential <pwd>   - binding credentials for ldap principal\n");
    printf("   -ldap_sscope <scope>     - ldap search scope, 'onelevel' or 'subtree'\n");
    printf("   -ldap_conntype <type>    - connection type to ldap server\n");
    printf("   -ldap_cafile <filename>  - trusted CA for server authentication\n");
    printf("\n");
    exit(0);
}

tibems_status
setLookupParams(
    tibemsLookupParams          lookup_params,
    int                         argc, 
    char*                       argv[]) 
{
    tibems_status               status = TIBEMS_OK;
    tibems_int                  i      = 1;
    
    while(i < argc && status == TIBEMS_OK)
    {
        if (!argv[i]) 
        {
            i += 1;
        }        
        else
        if (strcmp(argv[i],"-ldap_server")==0) 
        {
            if ((i+1) >= argc) ldapUsage();

            status = tibemsLookupParams_SetLdapServerUrl(lookup_params,argv[i+1]);
            argv[i] = NULL;
            argv[i+1] = NULL;
            i += 2;
        }
        else
        if (strcmp(argv[i],"-ldap_basedn")==0) 
        {
            if ((i+1) >= argc) ldapUsage();

            status = tibemsLookupParams_SetLdapBaseDN(lookup_params,argv[i+1]);
            argv[i] = NULL;
            argv[i+1] = NULL;
            i += 2;
        }
        else
        if (strcmp(argv[i],"-ldap_principal")==0) 
        {
            if ((i+1) >= argc) ldapUsage();

            status = tibemsLookupParams_SetLdapPrincipal(lookup_params,argv[i+1]);
            argv[i] = NULL;
            argv[i+1] = NULL;
            i += 2;
        }
        else
        if (strcmp(argv[i],"-ldap_credential")==0) 
        {
            if ((i+1) >= argc) ldapUsage();

            status = tibemsLookupParams_SetLdapCredential(lookup_params,argv[i+1]);
            argv[i] = NULL;
            argv[i+1] = NULL;
            i += 2;
        }
        else
        if (strcmp(argv[i],"-ldap_sscope")==0) 
        {
            if ((i+1) >= argc) ldapUsage();

            status = tibemsLookupParams_SetLdapSearchScope(lookup_params,argv[i+1]);
            argv[i] = NULL;
            argv[i+1] = NULL;
            i += 2;
        }
        else
        if (strcmp(argv[i],"-ldap_conntype")==0) 
        {
            if ((i+1) >= argc) ldapUsage();

            status = tibemsLookupParams_SetLdapConnType(lookup_params,argv[i+1]);
            argv[i] = NULL;
            argv[i+1] = NULL;
            i += 2;
        }
        else
        if (strcmp(argv[i],"-ldap_cafile")==0) 
        {
            if ((i+1) >= argc) ldapUsage();

            status = tibemsLookupParams_SetLdapCAFile(lookup_params,argv[i+1]);
            argv[i] = NULL;
            argv[i+1] = NULL;
            i += 2;
        }
        else 
        {
            i += 1;
        }
    }

    return status;
}

/*-----------------------------------------------------------------------
 * parseArgs
 *----------------------------------------------------------------------*/
void parseArgs(int argc, char** argv) 
{
    tibems_int                  i = 1;

    sslParams = tibemsSSLParams_Create();
    
    setSSLParams(sslParams,argc,argv,&pk_password);

    lookupParams = tibemsLookupParams_Create();

    setLookupParams(lookupParams, argc, argv);

    while(i < argc)
    {
        if (!argv[i]) 
        {
            i += 1;
        }
        else
        if (strcmp(argv[i],"-help")==0) 
        {
            usage();
        }
        else
        if (strcmp(argv[i],"-help-ssl")==0) 
        {
            sslUsage();
        }
        else
        if (strcmp(argv[i],"-help-ldap")==0) 
        {
            ldapUsage();
        }
        else
        if (strcmp(argv[i],"-server")==0) 
        {
            if ((i+1) >= argc) usage();
            serverUrl = argv[i+1];
            i += 2;
        }
        else
        if (strcmp(argv[i],"-user")==0) 
        {
            if ((i+1) >= argc) usage();
            userName = argv[i+1];
            i += 2;
        }
        else
        if (strcmp(argv[i],"-password")==0) 
        {
            if ((i+1) >= argc) usage();
            password = argv[i+1];
            i += 2;
        }
        else 
        if (strcmp(argv[i],"-lookupdest")==0) 
        {
            if ((i+1) >= argc) usage();
            lookupdest = argv[i+1];
            i += 2;
        }
        else
        if (strcmp(argv[i],"-lookupfact")==0) 
        {
            if ((i+1) >= argc) usage();
            lookupfact = argv[i+1];
            i += 2;
        }
        else
        {
            printf("unknown parameter <%s> ignoring\n", argv[i]);
            i++;
        }
    }

    if(!serverUrl || strncmp(serverUrl,"ssl",3))
    {
        tibemsSSLParams_Destroy(sslParams);
        sslParams = NULL;
    }

}

/*---------------------------------------------------------------------
 * fail
 *---------------------------------------------------------------------*/
void fail(
    const char*                 message, 
    tibemsErrorContext          errContext)
{
    tibems_status               status = TIBEMS_OK;
    const char*                 str    = NULL;

    printf("ERROR: %s\n",message);

    status = tibemsErrorContext_GetLastErrorString(errContext, &str);
    printf("\nLast error message =\n%s\n", str);
    status = tibemsErrorContext_GetLastErrorStackTrace(errContext, &str);
    printf("\nStack trace = \n%s\n", str);

    exit(1);
}

void
createContext(
    tibemsLookupContext*        context)
{
    tibems_status               status;

    if (!(*context))
    {
        if (tibemsLookupParams_GetLdapServerUrl(lookupParams))
            status = tibemsLookupContext_CreateExternal(context, lookupParams);
        else
            if (sslParams != NULL)
                status = tibemsLookupContext_CreateSSL(context, serverUrl, userName,password, 
                                                       sslParams, pk_password);
            else
                status = tibemsLookupContext_Create(context, serverUrl, userName,password);
            
        if (status != TIBEMS_OK)
        {
            fail("run: tibemsLookupContext_Create", errorContext);
        }
    }
}

/*-----------------------------------------------------------------------
 * run
 *----------------------------------------------------------------------*/
void run() 
{
    tibems_status               status            = TIBEMS_OK;

    tibemsLookupContext         context           = NULL;
    tibemsDestination           lookupDestination = NULL;
    char                        destName[512];
    
    status = tibemsErrorContext_Create(&errorContext);

    if (status != TIBEMS_OK)
    {
        printf("ErrorContext create failed: %s\n", tibemsStatus_GetText(status));
        exit(1);
    }

    if (lookupdest)
    {
        printf("\ndoing lookup for destination '%s'\n",lookupdest);
        
        if (!context)
            createContext(&context);
        
        /* Set the destination from the lookup destination object.
         * The lookup call will return TIBEMS_NOT_FOUND if the name is not created
         * in the server.
         */
        status = tibemsLookupContext_Lookup(context, lookupdest, (void**)&lookupDestination);
        switch(status){
        case TIBEMS_OK:
            destination = lookupDestination;
            status = tibemsDestination_GetName(destination, destName, sizeof(destName));
            if (status == TIBEMS_OK)
                printf("OK - successfully did lookup of dest  %s\n", lookupdest);
            break;
            
        case TIBEMS_NOT_FOUND:
            printf("INFO: The destination alias '%s' is not found in the EMS server.\n", lookupdest);
            printf("INFO: You can create the name using tibemsadmin commands:\n\n");
            printf("          tibemsadmin> create topic <yourtopic>\n");
            printf("          tibemsadmin> create jndiname %s topic <yourtopic>\n\n", lookupdest);
            destination = NULL;
            break;
        default:
            fail("run: tibemsLookupContext_Lookup", errorContext);
            break;
        }
    }
    
    
    if (lookupfact)
    {
        printf("\ndoing lookup for ConnectionFactory '%s'\n",lookupfact);
        
        if (!context)
            createContext(&context);
        
        /* lookup the factory object */
        status = tibemsLookupContext_Lookup(context, lookupfact, (void**)&cf);
        switch(status){
        case TIBEMS_OK:
            {
                printf("OK - successfully did lookup of connection factory, ");
                tibemsConnectionFactory_Print(cf);
            }
            break;
            
        case TIBEMS_NOT_FOUND:
            printf("INFO: The connection factory '%s' is not found in the EMS server.\n", lookupfact);
            printf("INFO: You can create the name using tibemsadmin commands:\n\n");
            printf("          tibemsadmin> create factory %s [queue|topic] <properties>\n", lookupfact);
            printf("INFO: This sample will then use factory %s to create connections.\n\n", lookupfact);
            
            destination = NULL;
            break;
        default:
            fail("tibemsLookupContext_Lookup(connectionFactory)", errorContext);
            break;
        }
    }
    
    /* finished lookups - destroy the context */
    if (context) {
        status = tibemsLookupContext_Destroy(context);
        if (status != TIBEMS_OK)
        {
            fail("tibemsLookupContext_Destroy", errorContext);
        }
    }
    
    if (cf)
    {
        status = tibemsConnectionFactory_Destroy(cf);
        if (status != TIBEMS_OK)
        {
            fail("Error destroying tibemsConnectionFactory", errorContext);
        }
    }
    if (destination)
    {
        status = tibemsDestination_Destroy(destination);
        if (status != TIBEMS_OK)
        {
            fail("Error destroying destination", errorContext);
        }
    }


    /* destroy the ssl params */
    tibemsSSLParams_Destroy(sslParams);
    tibemsLookupParams_Destroy(lookupParams);

    tibemsErrorContext_Close(errorContext);
}

/*-----------------------------------------------------------------------
 * main
 *----------------------------------------------------------------------*/
int main(int argc, char** argv)
{
    parseArgs(argc,argv);

    run();

    return 1;
}
