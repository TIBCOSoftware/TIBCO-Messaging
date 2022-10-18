/* 
 * Copyright (c) 2001-$Date: 2016-12-13 17:00:37 -0600 (Tue, 13 Dec 2016) $ TIBCO Software Inc. 
 * Licensed under a BSD-style license. Refer to [LICENSE]
 * For more information, please contact:
 * TIBCO Software Inc., Palo Alto, California, USA
 * 
 * $Id: tibemsXAMsgConsumer.c 90180 2016-12-13 23:00:37Z olivierh $
 * 
 */

/*
 * This sample demonstrates how to use the tibemsXAResource object
 * for consuming messages.
 *
 * Note that the tibemsXAResource calls are normally made by the
 * Transaction Manager, not directly by the application being managed
 * by the TM.
 *
 * This sample subscribes to specified destination and
 * receives and prints all received messages.
 *
 * Notice that the specified destination should exist in your configuration
 * or your topics/queues configuration file should allow
 * creation of the specified destination. 
 *
 * If this sample is used to receive messages published by 
 * tibemsMsgProducer sample, it must be started prior
 * to running the tibemsMsgProducer sample.
 *
 * Usage:  tibemsXAMsgConsumer [options]
 *
 *    where options are:
 *
 *      -server     Server URL.
 *                  If not specified this sample assumes a
 *                  serverUrl of "tcp://localhost:7222"
 *
 *      -user       User name. Default is null.
 *      -password   User password. Default is null.
 *      -topic      Topic name. Default is "topic.sample"
 *      -queue      Queue name. No default
 * 
 *
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

/*-----------------------------------------------------------------------
 * User must supply the xa.h file.
 *----------------------------------------------------------------------*/
#include "xa.h"

#include "tibemsUtilities.h"

/*-----------------------------------------------------------------------
 * Parameters
 *----------------------------------------------------------------------*/

char*           serverUrl   = NULL;
char*           userName    = NULL;
char*           password    = NULL;
char*           pk_password = NULL;
char*           name        = "topic.sample";
tibems_bool     useTopic    = TIBEMS_TRUE;

/*-----------------------------------------------------------------------
 * Variables
 *----------------------------------------------------------------------*/
tibemsConnectionFactory  factory     = NULL;
tibemsConnection         connection  = NULL;
tibemsSession            session     = NULL;
tibemsMsgConsumer        msgConsumer = NULL;
tibemsDestination        destination = NULL;

tibemsSSLParams          sslParams   = NULL;
int                      receive     = 1;

tibemsErrorContext       gErrorContext = NULL;    

/*-----------------------------------------------------------------------
 * usage
 *----------------------------------------------------------------------*/
void usage() 
{
    printf("\nUsage: tibemsXAMsgConsumer [options] [ssl options]\n");
    printf("\n");
    printf("   where options are:\n");
    printf("\n");
    printf(" -server   <server URL> - EMS server URL, default is local server\n");
    printf(" -user     <user name>  - user name, default is null\n");
    printf(" -password <password>   - password, default is null\n");
    printf(" -topic    <topic-name> - topic name, default is \"topic.sample\"\n");
    printf(" -queue    <queue-name> - queue name, no default\n");
    printf(" -help-ssl              - help on ssl parameters\n");
    exit(0);
}

/*---------------------------------------------------------------------
 * failNoExit
 *---------------------------------------------------------------------*/
void failNoExit(
    const char* message, 
    tibems_status s)
{
    const char* msg = tibemsStatus_GetText(s);
    const char* str = NULL;

    printf("ERROR: %s\n",message);
    printf("\tSTATUS: %d %s\n",s,msg?msg:"(Undefined Error)");

    tibemsErrorContext_GetLastErrorString(gErrorContext, &str);
    printf("\nLast error message =\n%s\n", str);
    tibemsErrorContext_GetLastErrorStackTrace(gErrorContext, &str);
    printf("\nStack trace = \n%s\n", str);
}

/*---------------------------------------------------------------------
 * fail
 *---------------------------------------------------------------------*/
void fail(
    const char* message, 
    tibems_status s)
{
    failNoExit(message, s);
    exit(1);
}

/*-----------------------------------------------------------------------
 * parseArgs
 *----------------------------------------------------------------------*/
void parseArgs(int argc, char** argv) 
{
    int i=1;
    
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
            printf("Unrecognized parameter: %s\n",argv[i]);
            usage();
        }
    }

    if(!serverUrl || strncmp(serverUrl,"ssl",3))
    {
        tibemsSSLParams_Destroy(sslParams);
        sslParams = NULL;
    }
}

/*---------------------------------------------------------------------
 * initialize XID with not-guaranteed-unique value
 *---------------------------------------------------------------------*/
void initXID(
    XID* xid)
{
    static time_t       seed    = 0;
    long                randNum = 0;

    xid->formatID = 1;
    xid->gtrid_length = 8;
    xid->bqual_length = 0;

    /* one time seed for random number generator */
    if (seed == 0)
    {
        seed = time(0);
        srand((int)seed);
    }

    randNum = rand();
    sprintf(xid->data, "%08lX\n", randNum);
}

/*---------------------------------------------------------------------
 * onException
 *---------------------------------------------------------------------*/
void onException(
    tibemsConnection    conn,
    tibems_status       reason,
    void*               closure)
{
    /* print the connection exception status */

    if (reason == TIBEMS_SERVER_NOT_CONNECTED)
    {
        printf("CONNECTION EXCEPTION: Server Disconnected\n");
        receive = 0;
    }
}

/*-----------------------------------------------------------------------
 * run
 *----------------------------------------------------------------------*/
void run() 
{
    tibems_status       status      = TIBEMS_OK;
    tibemsMsg           msg         = NULL;
    const char*         txt         = NULL;
    tibemsMsgType       msgType     = TIBEMS_MESSAGE_UNKNOWN;
    char*               msgTypeName = "UNKNOWN";
    tibemsXAResource    xaResource  = NULL;
    XID                 xid;
    
    if (!name) {
        printf("***Error: must specify destination name\n");
        usage();
    }

    status = tibemsErrorContext_Create(&gErrorContext);
    if (status != TIBEMS_OK)
    {
        printf("ErrorContext create failed: %s\n", tibemsStatus_GetText(status));
        exit(1);
    }
    
    printf("Subscribing to destination: '%s'\n\n",name);
    

    factory = tibemsConnectionFactory_Create();
    if (!factory)
    {
        fail("Error creating tibemsConnectionFactory", status);
    }

    status = tibemsConnectionFactory_SetServerURL(factory,serverUrl);
    if (status != TIBEMS_OK) 
    {
        fail("Error setting server url", status);
    }

    /* create the connection, use ssl if specified */
    if(sslParams)
    {
        status = tibemsConnectionFactory_SetSSLParams(factory,sslParams);
        if (status != TIBEMS_OK) 
        {
            fail("Error setting ssl params", status);
        }
        status = tibemsConnectionFactory_SetPkPassword(factory,pk_password);
        if (status != TIBEMS_OK) 
        {
            fail("Error setting pk password", status);
        }
    }

    status = tibemsConnectionFactory_CreateXAConnection(factory,&connection,
                                                      userName,password);
    if (status != TIBEMS_OK)
    {
        fail("Error creating tibemsConnection", status);
    }


    /* set the exception listener */
    status = tibemsConnection_SetExceptionListener(connection,
            onException, NULL);
    if (status != TIBEMS_OK)
    {
        fail("Error setting exception listener", status);
    }

    /* create the destination */
    if (useTopic)
        status = tibemsTopic_Create(&destination,name);
    else
        status = tibemsQueue_Create(&destination,name);
    if (status != TIBEMS_OK)
    {
        fail("Error creating tibemsDestination", status);
    }
        
    /* create the session */
    status = tibemsXAConnection_CreateXASession(connection,&session);
    if (status != TIBEMS_OK)
    {
        fail("Error creating tibemsSession", status);
    }
        
    /* create the consumer */
    status = tibemsSession_CreateConsumer(session,
            &msgConsumer,destination,NULL,TIBEMS_FALSE);
    if (status != TIBEMS_OK)
    {
        fail("Error creating tibemsMsgConsumer", status);
    }

    /* get the XA resource */
    status = tibemsXASession_GetXAResource(session, &xaResource);
    if (status != TIBEMS_OK)
    {
        fail("Error getting tibemsXAResource", status);
    }

    /* start the connection */
    status = tibemsConnection_Start(connection);
    if (status != TIBEMS_OK)
    {
        fail("Error starting tibemsConnection", status);
    }
    
    /* read messages */
    while(receive) 
    {
        /* start the transaction */
        initXID(&xid);
        status = tibemsXAResource_Start(xaResource,&xid, TMNOFLAGS);
        if (status != TIBEMS_OK)
        {
            fail("Error starting tibemsXAResource", status);
        }

        /* receive the message */
        status = tibemsMsgConsumer_Receive(msgConsumer,&msg);
        if (status != TIBEMS_OK)
        {
            fail("Error receiving message", status);
        }
        if (!msg)
            break;

        /* check message type */
        status = tibemsMsg_GetBodyType(msg,&msgType);
        if (status != TIBEMS_OK)
        {
            fail("Error getting message type", status);
        }

        switch(msgType)
        {
            case TIBEMS_MESSAGE:
                msgTypeName = "MESSAGE";
                break;

            case TIBEMS_BYTES_MESSAGE:
                msgTypeName = "BYTES";
                break;

            case TIBEMS_OBJECT_MESSAGE:
                msgTypeName = "OBJECT";
                break;

            case TIBEMS_STREAM_MESSAGE:
                msgTypeName = "STREAM";
                break;

            case TIBEMS_MAP_MESSAGE:
                msgTypeName = "MAP";
                break;

            case TIBEMS_TEXT_MESSAGE:
                msgTypeName = "TEXT";
                break;

            default:
                msgTypeName = "UNKNOWN";
                break;
        }

        /* publish sample sends TEXT message, if received other
         * just print entire message
         */
        if (msgType != TIBEMS_TEXT_MESSAGE)
        {
            printf("Received %s message:\n",msgTypeName);
            tibemsMsg_Print(msg);
        }
        else
        {
            /* get the message text */
            status = tibemsTextMsg_GetText(msg,&txt);
            if (status != TIBEMS_OK)
            {
                fail("Error getting tibemsTextMsg text", status);
            }

            printf("Received TEXT message: %s\n",
                txt ? txt : "<text is set to NULL>");
        }

        /* destroy the message */
        status = tibemsMsg_Destroy(msg);
        if (status != TIBEMS_OK)
        {
            fail("Error destroying tibemsMsg", status);
        }

        /* end the transaction */
        status = tibemsXAResource_End(xaResource,&xid, TMSUCCESS);
        if (status != TIBEMS_OK)
        {
            failNoExit("Error ending tibemsXAResource", status);
            tibemsXAResource_Rollback(xaResource,&xid);
            exit(1);
        }
    
        /* prepare the transaction */
        status = tibemsXAResource_Prepare(xaResource,&xid);
        if (status != TIBEMS_OK)
        {
            failNoExit("Error preparing tibemsXAResource", status);
            tibemsXAResource_Rollback(xaResource,&xid);
            exit(1);
        }
    
        /* commit the transaction */
        status = tibemsXAResource_Commit(xaResource,&xid, TMNOFLAGS);
        if (status != TIBEMS_OK)
        {
            fail("Error committing tibemsXAResource", status);
        }
    }
    
    /* destroy the destination */
    status = tibemsDestination_Destroy(destination);
    if (status != TIBEMS_OK)
    {
        fail("Error destroying tibemsDestination", status);
    }

    /* close the connection */
    status = tibemsConnection_Close(connection);
    if (status != TIBEMS_OK)
    {
        fail("Error closing tibemsConnection", status);
    }

    /* destroy the ssl params */
    if (sslParams) 
    {
        tibemsSSLParams_Destroy(sslParams);
    }

    tibemsErrorContext_Close(gErrorContext);
}
    
/*-----------------------------------------------------------------------
 * main
 *----------------------------------------------------------------------*/
int main(int argc, char** argv)
{
    parseArgs(argc,argv);

    /* print parameters */
    printf("\n------------------------------------------------------------------------\n");
    printf("tibemsXAMsgConsumer SAMPLE\n");
    printf("------------------------------------------------------------------------\n");
    printf("Server....................... %s\n",serverUrl?serverUrl:"localhost");
    printf("User......................... %s\n",userName?userName:"(null)");
    printf("Destination.................. %s\n",name);
    printf("------------------------------------------------------------------------\n\n");

    run();

    return 1;
}
