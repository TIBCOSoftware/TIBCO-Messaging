/* 
 * Copyright (c) 2001-$Date: 2016-12-13 17:00:37 -0600 (Tue, 13 Dec 2016) $ TIBCO Software Inc. 
 * Licensed under a BSD-style license. Refer to [LICENSE]
 * For more information, please contact:
 * TIBCO Software Inc., Palo Alto, California, USA
 * 
 * $Id: tibemsBrowser.c 90180 2016-12-13 23:00:37Z olivierh $
 * 
 */

/*
 * This sample demonstrates the use of tibemsQueueBrowser with
 * TIBCO Enterprise for EMS.
 *
 * Notice that TIBCO Enterprise for EMS implements dynamic
 * queue browsers. This means that the tibemsQueueBrowser can
 * dynamically receive new messages added to the queue.
 * If tibemsQueueBrowser_GetNext(...) method QueueBrowser class 
 * returns TIBEMS_NOT_FOUND, the EMS application
 * can wait and try to call it later. If the queue being browsed
 * has received new messages, the tibemsQueueBrowser_GetNext(...)
 * method will return TIBEMS_OK and the application can browse 
 * new messages. If tibemsQueueBrowser_GetNext(...) returns 
 * TIBEMS_NOT_FOUND, the application can choose to quit browsing 
 * or can wait for more messages to be delivered into the queue.
 *
 * After all queue messages have been delivered to the queue
 * browser, TIBCO Enterprise for EMS waits for some time and then
 * tries to query if any new messages. This happens behind the scene,
 * user application can try to call tibemsQueueBrowser_GetNext(...) 
 * at any time, the internal engine will only actually query the 
 * queue every fixed interval. TIBCO Enterprise for EMS queries
 * the queue not more often than every 5 seconds but the length
 * of that interval is a subject to change without notice.
 *
 * Usage:  tibemsQueueBrowser [options]
 *
 *    where options are:
 *
 *      -server     Server URL.
 *                  If not specified this sample assumes a
 *                  serverUrl of "tcp://localhost:7222"
 *
 *      -user       User name. Default is null.
 *      -password   User password. Default is null.
 *      -queue      Queue name. Default is "queue.sample.browser"
 * 
 *
 */
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

#include "tibemsUtilities.h"

/*-----------------------------------------------------------------------
 * Parameters
 *----------------------------------------------------------------------*/

char*                           serverUrl   = NULL;
char*                           userName    = NULL;
char*                           password    = NULL;
char*                           pk_password = NULL;
char*                           queueName   = "queue.sample.browser";


/*-----------------------------------------------------------------------
 * Variables
 *----------------------------------------------------------------------*/
tibemsConnectionFactory         factory      = NULL;
tibemsConnection                connection   = NULL;
tibemsSession                   session      = NULL;
tibemsMsgConsumer               msgConsumer  = NULL;
tibemsMsgProducer               msgProducer  = NULL;
tibemsQueueBrowser              queueBrowser = NULL;
tibemsDestination               destination  = NULL;           

tibemsSSLParams                 sslParams    = NULL;

tibemsErrorContext              errorContext = NULL;

/*-----------------------------------------------------------------------
 * usage
 *----------------------------------------------------------------------*/
void usage() 
{
    printf("\nUsage: tibemsQueueBrowser [options] [ssl options]\n");
    printf("\n");
    printf("   where options are:\n");
    printf("\n");
    printf(" -server   <server URL> - EMS server URL, default is local server\n");
    printf(" -user     <user name>  - user name, default is null\n");
    printf(" -password <password>   - password, default is null\n");
    printf(" -queue    <queue-name> - queue name, default is \"queue.sample.browser\"\n");
    printf(" -help-ssl              - help on ssl parameters\n");
    exit(0);
}

/*-----------------------------------------------------------------------
 * parseArgs
 *----------------------------------------------------------------------*/
void parseArgs(int argc, char** argv) 
{
    tibems_int                  i = 1;
    
    sslParams = tibemsSSLParams_Create();
    
    setSSLParams(sslParams,argc,argv, &pk_password);

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
        if (strcmp(argv[i],"-queue")==0) 
        {
            if ((i+1) >= argc) usage();
            queueName = argv[i+1];
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

/*---------------------------------------------------------------------
 * onException
 *---------------------------------------------------------------------*/
void onException(
    tibemsConnection            conn,
    tibems_status               reason,
    void*                       closure)
{
    /* print the connection exception status */

    if (reason == TIBEMS_SERVER_NOT_CONNECTED)
    {
        printf("CONNECTION EXCEPTION: Server Disconnected\n");
    }
}

/*-----------------------------------------------------------------------
 * closeAll
 *----------------------------------------------------------------------*/
void closeAll() 
{
    tibems_status               status = TIBEMS_OK;

    /* close the browser */
    status = tibemsQueueBrowser_Close(queueBrowser);
    if (status != TIBEMS_OK)
    {
        fail("Error closing tibemsQueueBrowser", errorContext);
    }
    
    /* destroy the queue */
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
 * run
 *----------------------------------------------------------------------*/
void run() 
{
    tibems_status               status   = TIBEMS_OK;
    tibemsMsg                   msg      = NULL;
    tibems_int                  attemptc = 0; 
    tibems_int                  browsec  = 0;
    tibems_int                  drainc   = 0;  
    tibems_int                  messagec = 0;
    tibems_int                  msg_num  = 0;
    tibems_int                  i        = 0;

    if (!queueName) {
        printf("***Error: must specify queue name\n");
        usage();
    }
    
    printf("Browser sample.\n");
    printf("Using server:   %s\n", serverUrl ? serverUrl : "(null)");
    printf("Browsing queue: %s\n",queueName);
    
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

    /* set the exception listener */
    status = tibemsConnection_SetExceptionListener(connection,
            onException, NULL);
    if (status != TIBEMS_OK)
    {
        fail("Error setting exception listener", errorContext);
    }

    /* create the queue */
    status = tibemsQueue_Create(&destination,queueName);
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

    /* start the connection */
    status = tibemsConnection_Start(connection);
    if (status != TIBEMS_OK)
    {
        fail("Error starting tibemsConnection", errorContext);
    }

    /* 
     * drain the queue
     */

    /* create the consumer */
    status = tibemsSession_CreateConsumer(session,
            &msgConsumer,destination,NULL,TIBEMS_FALSE);
    if (status != TIBEMS_OK)
    {
        fail("Error creating tibemsMsgConsumer", errorContext);
    }

    printf("Draining the queue %s\n",queueName);

    /* read queue messages until empty */
    while(1) 
    {
        /* receive the message */
        status = tibemsMsgConsumer_ReceiveTimeout(msgConsumer,&msg,1000);
        if (status == TIBEMS_TIMEOUT)
            break;
        
        if (status != TIBEMS_OK)
        {
            fail("Error receiving message", errorContext);
        }
        
        if (!msg)
            break;

        drainc++;

        /* destroy the message */
        status = tibemsMsg_Destroy(msg);
        if (status != TIBEMS_OK)
        {
            fail("Error destroying tibemsMsg", errorContext);
        }
    }

    printf("Drained %d messages from the queue.\n",drainc);
            
    /* 
     * close receiver to prevent any 
     * queue messages to be delivered
     */
    status = tibemsMsgConsumer_Close(msgConsumer);
    if (status != TIBEMS_OK)
    {
        fail("Error closing tibemsMsgConsumer", errorContext);
    }


    /*
     * send 5 messages into queue
     */
    printf("Sending 5 messages into queue.\n");
    for (i=0; i<5; i++) 
    {
        messagec++;

        /* create the message */
        status = tibemsSession_CreateMessage(session,&msg);
        if (status != TIBEMS_OK)
        {
            fail("Error creating tibemsMsg", errorContext);
        }

        /* set the msg_num Integer Property */
        status = tibemsMsg_SetIntProperty(msg,"msg_num",messagec);
        if (status != TIBEMS_OK)
        {
            fail("Error setting tibemsMsg Integer Property", errorContext);
        }

        /* send the message */
        status = tibemsMsgProducer_Send(msgProducer,msg);
        if (status != TIBEMS_OK)
        {
            fail("Error sending tibemsMsg", errorContext);
        }

        /* destroy the message */
        status = tibemsMsg_Destroy(msg);
        if (status != TIBEMS_OK)
        {
            fail("Error destroying tibemsMsg", errorContext);
        }
    }
    
    /*
     * create browser and browse what is there in the queue
     */
    printf("--- Browsing the queue.\n");

    status = tibemsSession_CreateBrowser(session,
            &queueBrowser,destination,NULL);
    if (status != TIBEMS_OK)
    {
        fail("Error creating tibemsQueueBrowser", errorContext);
    }    
    
    while(1) 
    {
        /* get the next message */
        status = tibemsQueueBrowser_GetNext(queueBrowser,&msg);

        if (status == TIBEMS_NOT_FOUND)
            break;

        if (status != TIBEMS_OK)
        {
            fail("Error getting next message from tibemsQueueBrowser", errorContext);
        }

        if(!msg)
            break;

        /* get the msg_num Integer Property */
        status = tibemsMsg_GetIntProperty(msg,"msg_num",&msg_num);
        if (status != TIBEMS_OK)
        {
           fail("Error getting Integer Property from tibemsMsg", errorContext);
        }

        printf("Browsed message: number=%d\n",msg_num);

        /* destroy the message */
        status = tibemsMsg_Destroy(msg);
        if (status != TIBEMS_OK)
        {
            fail("Error destroying tibemsMsg", errorContext);
        }

        browsec++;
    }
            
    printf("--- No more messages in the queue.\n");
            
    /*
     * send 5 more messages into queue
     */
    printf("Sending 5 more messages into queue.\n");
    for (i=0; i<5; i++) 
    {
        messagec++;

        /* create the message */
        status = tibemsSession_CreateMessage(session,&msg);
        if (status != TIBEMS_OK)
        {
            fail("Error creating tibemsMsg", errorContext);
        }

        /* set the msg_num Integer Property */
        status = tibemsMsg_SetIntProperty(msg,"msg_num",messagec);
        if (status != TIBEMS_OK)
        {
            fail("Error setting tibemsMsg Integer Property", errorContext);
        }

        /* send the message */
        status = tibemsMsgProducer_Send(msgProducer,msg);
        if (status != TIBEMS_OK)
        {
            fail("Error sending tibemsMsg", errorContext);
        }

        /* destroy the message */
        status = tibemsMsg_Destroy(msg);
        if (status != TIBEMS_OK)
        {
            fail("Error destroying tibemsMsg", errorContext);
        }
    }

    /* 
     * try to browse again, if no success for some time
     * then quit
     */
     
    /* notice that we will *not* receive new messages
     * instantly. It happens because tibemsQueueBrowser limits
     * the frequency of query requests sent into the queue
     * after the queue was empty. Internal engine only queries
     * the queue every so many seconds, so we'll likely have
     * to wait here for some time.
     */
           
    while(1) 
    {
        /* get the next message */
        status = tibemsQueueBrowser_GetNext(queueBrowser,&msg);
        if (status != TIBEMS_NOT_FOUND && status != TIBEMS_OK)
        {
            fail("Error getting next tibemsMsg from tibemsQueueBrowser", errorContext);
        }

        if(msg)
            break;

        attemptc++;

        printf("Waiting for messages to arrive, count=%d\n",attemptc);

        tibems_Sleep(1000);

        if (attemptc > 30)
        {
            printf("Still no messages in the queue after %d seconds.\n",attemptc);
            closeAll();
            exit(0);
        }
    }
                           
    /* 
     * got more messages, continue browsing 
     */
    printf("Found more messages. Continue browsing.\n");
    while(msg) 
    {
        /* get the msg_num Integer Property */
        status = tibemsMsg_GetIntProperty(msg,"msg_num",&msg_num);
        if (status != TIBEMS_OK)
        {
            fail("Error getting Integer Property from tibemsMsg", errorContext);
        }

        printf("Browsed message: number=%d\n",msg_num);

        /* destroy the message */
        status = tibemsMsg_Destroy(msg);
        if (status != TIBEMS_OK)
        {
            fail("Error destroying tibemsMsg", errorContext);
        }

        /* get the next message */
        status = tibemsQueueBrowser_GetNext(queueBrowser,&msg);
        if (status != TIBEMS_NOT_FOUND && status != TIBEMS_OK)
        {
            fail("Error getting next tibemsMsg from tibemsQueueBrowser", errorContext);
        }
    }

    /*
     * close all and quit
     */
    closeAll();
}
    
/*-----------------------------------------------------------------------
 * main
 *----------------------------------------------------------------------*/
int main(int argc, char** argv)
{
    parseArgs(argc,argv);

    /* print parameters */
    printf("------------------------------------------------------------------------\n");
    printf("tibemsQueueBrowser SAMPLE\n");
    printf("------------------------------------------------------------------------\n");
    printf("Server....................... %s\n",serverUrl?serverUrl:"localhost");
    printf("User......................... %s\n",userName?userName:"(null)");
    printf("Queue........................ %s\n",queueName);
    printf("------------------------------------------------------------------------\n");

    run();

    return 1;
}
