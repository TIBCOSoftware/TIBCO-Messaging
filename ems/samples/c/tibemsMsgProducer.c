/* 
 * Copyright (c) 2001-$Date: 2016-12-13 17:00:37 -0600 (Tue, 13 Dec 2016) $ TIBCO Software Inc. 
 * Licensed under a BSD-style license. Refer to [LICENSE]
 * For more information, please contact:
 * TIBCO Software Inc., Palo Alto, California, USA
 * 
 * $Id: tibemsMsgProducer.c 90180 2016-12-13 23:00:37Z olivierh $
 * 
 */

/*
 * This is a simple sample of a basic tibemsMsgProducer.
 *
 * This samples publishes specified message(s) on a specified
 * destination and quits.
 *
 * Notice that the specified destination should exist in your configuration
 * or your topics/queues configuration file should allow
 * creation of the specified topic. Sample configuration supplied with
 * the TIBCO Enterprise for EMS distribution allows creation of any
 * destination.
 *
 * If this sample is used to publish messages into 
 * tibemsMsgConsumer sample, the tibemsMsgConsumer
 * sample must be started first.
 *
 * If -topic is not specified this sample will use a topic named
 * "topic.sample".
 *
 * Usage:  tibemsMsgProducer  [options]
 *                               <message-text1>
 *                               ...
 *                               <message-textN>
 *
 *  where options are:
 *
 *   -server    <server-url>  Server URL.
 *                            If not specified this sample assumes a
 *                            serverUrl of "tcp://localhost:7222"
 *   -user      <user-name>   User name. Default is null.
 *   -password  <password>    User password. Default is null.
 *   -topic     <topic-name>  Topic name. Default value is "topic.sample"
 *   -queue     <queue-name>  Queue name. No default
 *   -async                   Send asynchronously, default is false.
 *
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "tibemsUtilities.h"

/*-----------------------------------------------------------------------
 * Parameters
 *----------------------------------------------------------------------*/

char*                           serverUrl    = NULL;
char*                           userName     = NULL;
char*                           password     = NULL;
char*                           pk_password  = NULL;
char*                           name         = "topic.sample";
char**                          data         = NULL;
tibems_int                      datac        = 0;    
tibems_bool                     useTopic     = TIBEMS_TRUE;
tibems_bool                     useAsync      = TIBEMS_FALSE;

/*-----------------------------------------------------------------------
 * Variables
 *----------------------------------------------------------------------*/
tibemsConnectionFactory         factory      = NULL;
tibemsConnection                connection   = NULL;
tibemsSession                   session      = NULL;
tibemsMsgProducer               msgProducer  = NULL;
tibemsDestination               destination  = NULL;

tibemsSSLParams                 sslParams    = NULL;

tibemsErrorContext              errorContext = NULL;

/*
 * Note:  Use caution when modifying a message in a completion
 * callback to avoid concurrent message use.
 */
static void
onCompletion(tibemsMsg msg, tibems_status status, void* closure)
{
    const char *text;

    if (tibemsTextMsg_GetText((tibemsTextMsg)msg, &text) != TIBEMS_OK)
    {
        printf("Error retrieving message!\n");
        return;
    }

    if (status == TIBEMS_OK)
    {
        printf("Successfully sent message %s.\n", text);
    }
    else
    {
        printf("Error sending message %s.\n", text);
        printf("Error:  %s.\n", tibemsStatus_GetText(status));
    }
}

/*-----------------------------------------------------------------------
 * usage
 *----------------------------------------------------------------------*/
void usage() 
{
    printf("\nUsage: tibemsMsgProducer [options] [ssl options]\n");
    printf("                            <message-text-1>\n");
    printf("                           [<message-text-2>] ...\n");
    printf("\n");
    printf("   where options are:\n");
    printf("\n");
    printf("   -server   <server URL>  - EMS server URL, default is local server\n");
    printf("   -user     <user name>   - user name, default is null\n");
    printf("   -password <password>    - password, default is null\n");
    printf("   -topic    <topic-name>  - topic name, default is \"topic.sample\"\n");
    printf("   -queue    <queue-name>  - queue name, no default\n");
    printf("   -async                  - send asynchronously, default is false\n");
    printf("   -help-ssl               - help on ssl parameters\n");
    exit(0);
}
    
/*-----------------------------------------------------------------------
 * parseArgs
 *----------------------------------------------------------------------*/
void parseArgs(int argc, char** argv) 
{
    tibems_int                  i = 1;

    sslParams = tibemsSSLParams_Create();
    
    setSSLParams(sslParams,argc,argv,&pk_password);
    
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
        if (strcmp(argv[i],"-server")==0) 
        {
            if ((i+1) >= argc) usage();
            serverUrl = argv[i+1];
            i += 2;
        }
        else
        if (strcmp(argv[i],"-topic")==0) 
        {
            if ((i+1) >= argc) usage();
            name = argv[i+1];
            i += 2;
        }
        else
        if (strcmp(argv[i],"-queue")==0) 
        {
            if ((i+1) >= argc) usage();
            name = argv[i+1];
            useTopic = TIBEMS_FALSE;
            i += 2;
        }
        else
        if (strcmp(argv[i],"-async")==0) 
        {
            useAsync = TIBEMS_TRUE;
            i += 1;
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
        {
            data = &(argv[i]);
            datac = argc - i;
            break;
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

/*-----------------------------------------------------------------------
 * run
 *----------------------------------------------------------------------*/
void run() 
{
    tibems_status               status       = TIBEMS_OK;
    tibemsTextMsg               msg          = NULL;
    tibems_int                  i;

    if (!name) 
    {
        printf("***Error: must specify destination name\n");
        usage();
    }
    
    if (datac == 0) 
    {
        printf("***Error: must specify at least one message text\n");
        usage();
    }
    
    printf("Publishing to destination '%s'\n",name);
    

    status = tibemsErrorContext_Create(&errorContext);

    if (status != TIBEMS_OK)
    {
        printf("ErrorContext create failed: %s\n", tibemsStatus_GetText(status));
        exit(1);
    }

    factory = tibemsConnectionFactory_Create();
    if (!factory)
    {
        fail("Error creating tibemsConnectionFactory", errorContext);
    }

    status = tibemsConnectionFactory_SetServerURL(factory,serverUrl);
    if (status != TIBEMS_OK) 
    {
        fail("Error setting server url", errorContext);
    }

    /* create the connection, use ssl if specified */
    if(sslParams)
    {
        status = tibemsConnectionFactory_SetSSLParams(factory,sslParams);
        if (status != TIBEMS_OK) 
        {
            fail("Error setting ssl params", errorContext);
        }
        status = tibemsConnectionFactory_SetPkPassword(factory,pk_password);
        if (status != TIBEMS_OK) 
        {
            fail("Error setting pk password", errorContext);
        }
    }

    status = tibemsConnectionFactory_CreateConnection(factory,&connection,
                                                      userName,password);
    if (status != TIBEMS_OK)
    {
        fail("Error creating tibemsConnection", errorContext);
    }

    /* create the destination */
    if (useTopic)
        status = tibemsTopic_Create(&destination,name);
    else
        status = tibemsQueue_Create(&destination,name);
    if (status != TIBEMS_OK)
    {
        fail("Error creating tibemsDestination", errorContext);
    }

    /* create the session */
    status = tibemsConnection_CreateSession(connection,
            &session,TIBEMS_FALSE,TIBEMS_AUTO_ACKNOWLEDGE);
    if (status != TIBEMS_OK)
    {
        fail("Error creating tibemsSession", errorContext);
    }


    /* create the producer */
    status = tibemsSession_CreateProducer(session,
            &msgProducer,destination);
    if (status != TIBEMS_OK)
    {
        fail("Error creating tibemsMsgProducer", errorContext);
    }
            
    /* publish messages */
    for (i = 0; i < datac; i++) 
    {
        /* create the text message */
        status = tibemsTextMsg_Create(&msg);
        if (status != TIBEMS_OK)
        {
            fail("Error creating tibemsTextMsg", errorContext);
        }
        
        /* set the message text */
        status = tibemsTextMsg_SetText(msg,data[i]);
        if (status != TIBEMS_OK)
        {
            fail("Error setting tibemsTextMsg text", errorContext);
        }

        /* publish the message */
        if (useAsync)
            status = tibemsMsgProducer_AsyncSend(msgProducer, msg, onCompletion, NULL);
        else
            status = tibemsMsgProducer_Send(msgProducer,msg);

        if (status != TIBEMS_OK)
        {
            fail("Error publishing tibemsTextMsg", errorContext);
        }
        printf("Published message: %s\n",data[i]);
        
        /* destroy the message */
        status = tibemsMsg_Destroy(msg);
        if (status != TIBEMS_OK)
        {
            fail("Error destroying tibemsTextMsg", errorContext);
        }
    }
    
    /* destroy the destination */
    status = tibemsDestination_Destroy(destination);
    if (status != TIBEMS_OK)
    {
        fail("Error destroying tibemsDestination", errorContext);
    }

    /* close the connection */
    status = tibemsConnection_Close(connection);
    if (status != TIBEMS_OK)
    {
        fail("Error closing tibemsConnection", errorContext);
    }

    /* destroy the ssl params */
    if (sslParams)
    {
        tibemsSSLParams_Destroy(sslParams);
    }

    tibemsErrorContext_Close(errorContext);
}

/*-----------------------------------------------------------------------
 * main
 *----------------------------------------------------------------------*/
int main(int argc, char** argv)
{
    tibems_int                  i;

    parseArgs(argc,argv);

    /* print parameters */
    printf("------------------------------------------------------------------------\n");
    printf("tibemsMsgProducer SAMPLE\n");
    printf("------------------------------------------------------------------------\n");
    printf("Server....................... %s\n",serverUrl?serverUrl:"localhost");
    printf("User......................... %s\n",userName?userName:"(null)");
    printf("Destination.................. %s\n",name);
    printf("Send Asynchronously.......... %s\n",useAsync?"true":"false");
    printf("Message Text................. \n");
    for(i = 0; i < datac; i++) 
    {
        printf("\t%s \n", data[i]);
    }
    printf("------------------------------------------------------------------------\n");

    run();

    return 1;
}
