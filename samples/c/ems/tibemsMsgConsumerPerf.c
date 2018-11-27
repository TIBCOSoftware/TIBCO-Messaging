/* 
 * Copyright (c) 2006-$Date: 2016-12-13 17:00:37 -0600 (Tue, 13 Dec 2016) $ TIBCO Software Inc. 
 * Licensed under a BSD-style license. Refer to [LICENSE]
 * For more information, please contact:
 * TIBCO Software Inc., Palo Alto, California, USA
 * 
 * $Id: tibemsMsgConsumerPerf.c 90180 2016-12-13 23:00:37Z olivierh $
 * 
 */

/*
 * Usage:  tibemsMsgConsumerPerf [options]
 *
 *  where options are:
 *
 *   -server       <url>         EMS server URL. Default is
 *                               "tcp://localhost:7222".
 *   -user         <username>    User name. Default is null.
 *   -password     <password>    User password. Default is null.
 *   -topic        <topic-name>  Topic name. Default is "topic.sample".
 *   -queue        <queue-name>  Queue name. No default.
 *   -count        <num msgs>    Number of messages to consume. Default is 10k.
 *   -time         <seconds>     Number of seconds to run. Default is 0.
 *   -threads      <num threads> Number of consumer threads. Default is 1.
 *   -connections  <num conns>   Number of consumer connections. Default is 1.
 *   -txnsize      <num msgs>    Number of messages per transaction. Default 0.
 *   -durable      <name>        Durable subscription name.
 *   -selector     <selector>    Message selector for consumer threads. 
 *   -ackmode      <mode>        Message acknowledge mode. Default is AUTO.
 *                               Other values: DUPS_OK, CLIENT EXPLICIT_CLIENT,
 *                               EXPLICIT_CLIENT_DUPS_OK and NO.
 *   -uniquedests                Each consumer thread uses a unique destination.
 */

#include "tibemsUtilities.h"

typedef struct _runArgs
{
    tibemsConnection            connection;
    tibems_int                  number;
    tibems_int                  count;
    tibems_long                 start;
    tibems_long                 stop;
} runArgs;

/*-----------------------------------------------------------------------
 * Parameters
 *----------------------------------------------------------------------*/
char*                           serverUrl      = "tcp://localhost:7222";
char*                           username       = NULL;
char*                           password       = NULL;
char*                           destination    = "topic.sample";
char*                           durable        = NULL;
char*                           selector       = NULL;
tibems_int                      txnSize        = 0;
tibems_int                      count          = 0;
tibems_long                     runTime        = 0;
tibems_int                      threads        = 1;
tibems_int                      connections    = 1;
tibemsAcknowledgeMode           ackMode        = TIBEMS_AUTO_ACKNOWLEDGE;
tibems_bool                     useUniqueDests = TIBEMS_FALSE;
tibems_bool                     useTopic       = TIBEMS_TRUE;

tibemsErrorContext              gErrorContext  = NULL;    

/*---------------------------------------------------------------------
 * fail
 *---------------------------------------------------------------------*/
void fail(
    const char*                 message, 
    tibemsErrorContext          errorContext)
{
    tibems_status               status = TIBEMS_OK;
    const char*                 str    = NULL;

    printf("ERROR: %s\n",message);

    status = tibemsErrorContext_GetLastErrorString(errorContext, &str);
    printf("\nLast error message =\n%s\n", str);
    status = tibemsErrorContext_GetLastErrorStackTrace(errorContext, &str);
    printf("\nStack trace = \n%s\n", str);

    exit(1);
}

/*-----------------------------------------------------------------------
 * usage
 *----------------------------------------------------------------------*/
void usage() 
{
    printf("\nUsage: tibemsMsgConsumerPerf [options]\n");
    printf("\n");
    printf("   where options are:\n");
    printf("\n");
    printf(" -server       <url>         Server URL. Default is \"tcp://localhost:7222\".\n");
    printf(" -user         <username>    User name. Default is null.\n");
    printf(" -password     <password>    User password. Default is null.\n");
    printf(" -topic        <name>        Topic name. Default is \"topic.sample\".\n");
    printf(" -queue        <name>        Queue name. No default.\n");
    printf(" -durable      <name>        Durable subscription name.\n");
    printf(" -selector     <selector>    Message selector for consumer threads.\n");
    printf(" -ackmode      <mode>        Acknowledge mode: NO, AUTO, DUPS_OK, CLIENT,\n");
    printf("                             EXPLICIT_CLIENT, EXPLICIT_CLIENT_DUPS_OK.\n");
    printf("                             Default is AUTO.\n");
    printf(" -txnsize      <num msgs>    Number of messages per transaction.\n");
    printf(" -count        <num msgs>    Number of messages to receive.\n");
    printf(" -time         <seconds>     Number of seconds to run.\n");
    printf(" -threads      <num threads> Number of consumer threads.\n");
    printf(" -connections  <num conns>   Number of connections.\n");
    printf(" -uniquedests                Each consumer uses a unique destination.\n");
    exit(0);
}

/*-----------------------------------------------------------------------
 * parseArgs
 *----------------------------------------------------------------------*/
void parseArgs(int argc, char** argv) 
{
    tibems_int                  i = 1;

    while (i < argc)
    {
        if (!argv[i])
        {
            i += 1;
        }
        else if (strcmp(argv[i],"-help")==0)
        {
            usage();
        }
        else if (strcmp(argv[i],"-server")==0)
        {
            if ((i+1) >= argc) usage();
            serverUrl = argv[i+1];
            i += 2;
        }
        else if (strcmp(argv[i],"-user")==0)
        {
            if ((i+1) >= argc) usage();
            username = argv[i+1];
            i += 2;
        }
        else if (strcmp(argv[i],"-password")==0)
        {
            if ((i+1) >= argc) usage();
            password = argv[i+1];
            i += 2;
        }
        else if (strcmp(argv[i],"-topic")==0)
        {
            if ((i+1) >= argc) usage();
            destination = argv[i+1];
            useTopic = TIBEMS_TRUE;
            i += 2;
        }
        else if (strcmp(argv[i],"-queue")==0)
        {
            if ((i+1) >= argc) usage();
            destination = argv[i+1];
            useTopic = TIBEMS_FALSE;
            i += 2;
        }
        else if (strcmp(argv[i],"-durable")==0)
        {
            if ((i+1) >= argc) usage();
            durable = argv[i+1];
            i += 2;
        }
        else if (strcmp(argv[i],"-selector")==0)
        {
            if ((i+1) >= argc) usage();
            selector = argv[i+1];
            i += 2;
        }
        else if (strcmp(argv[i],"-ackmode")==0)
        {
            if ((i+1) >= argc) usage();
            if (strcmp(argv[i+1], "NO") == 0)
            {
                ackMode = TIBEMS_NO_ACKNOWLEDGE;
            }
            else if (strcmp(argv[i+1], "AUTO") == 0)
            {
                ackMode = TIBEMS_AUTO_ACKNOWLEDGE;
            }
            else if (strcmp(argv[i+1], "CLIENT") == 0)
            {
                ackMode = TIBEMS_CLIENT_ACKNOWLEDGE;
            }
            else if (strcmp(argv[i+1], "DUPS_OK") == 0)
            {
                ackMode = TIBEMS_DUPS_OK_ACKNOWLEDGE;
            }
            else if (strcmp(argv[i+1], "EXPLICIT_CLIENT") == 0)
            {
                ackMode = TIBEMS_EXPLICIT_CLIENT_ACKNOWLEDGE;
            }
            else if (strcmp(argv[i+1], "EXPLICIT_CLIENT_DUPS_OK") == 0)
            {
                ackMode = TIBEMS_EXPLICIT_CLIENT_DUPS_OK_ACKNOWLEDGE;
            }
            else
            {
                printf("Invalid value for -ackmode: %s\n", argv[i+1]);
                usage();
            }
            i += 2;
        }
        else if (strcmp(argv[i],"-txnsize")==0)
        {
            if ((i+1) >= argc) usage();
            txnSize = atoi(argv[i+1]);
            i += 2;
        }
        else if (strcmp(argv[i],"-count")==0)
        {
            if ((i+1) >= argc) usage();
            count = atoi(argv[i+1]);
            i += 2;
        }
        else if (strcmp(argv[i],"-time")==0)
        {
            if ((i+1) >= argc) usage();
            runTime = (tibems_long)(1000 * atoi(argv[i+1]));
            i += 2;
        }
        else if (strcmp(argv[i],"-threads")==0)
        {
            if ((i+1) >= argc) usage();
            threads = atoi(argv[i+1]);
            if (threads < 1)
            {
                printf("Invalid value for -threads: %s\n", argv[i+1]);
                usage();
            }
            i += 2;
        }
        else if (strcmp(argv[i],"-connections")==0)
        {
            if ((i+1) >= argc) usage();
            connections = atoi(argv[i+1]);
            if (connections < 1)
            {
                printf("Invalid value for -connections: %s\n", argv[i+1]);
                usage();
            }
            i += 2;
        }
        else if (strcmp(argv[i],"-uniquedests")==0)
        {
            useUniqueDests = TIBEMS_TRUE;
            i += 1;
        }
        else
        {
            printf("Unrecognized parameter: %s\n",argv[i]);
            usage();
        }
    }

    if (runTime == 0 && count == 0)
    {
        printf("please specify either -count or -time\n");
        usage();
    }
}

/*---------------------------------------------------------------------
 * acknowledge mode to string
 *---------------------------------------------------------------------*/
char* ackModeToString(tibems_int ackmode)
{
    switch (ackmode)
    {
    case TIBEMS_NO_ACKNOWLEDGE:
        return "NO";
    case TIBEMS_AUTO_ACKNOWLEDGE:
        return "AUTO";
    case TIBEMS_CLIENT_ACKNOWLEDGE:
        return "CLIENT";
    case TIBEMS_DUPS_OK_ACKNOWLEDGE:
        return "DUPS_OK";
    case TIBEMS_EXPLICIT_CLIENT_ACKNOWLEDGE:
        return "EXPLICIT_CLIENT";
    case TIBEMS_EXPLICIT_CLIENT_DUPS_OK_ACKNOWLEDGE:
        return "EXPLICIT_CLIENT_DUPS_OK";
    default:
        return "(unknown)";
    }
}

/*---------------------------------------------------------------------
 * create destination
 *---------------------------------------------------------------------*/
void createDestination(tibems_int num, tibemsDestination* dest)
{
    tibems_status               status;
    char*                       tmpStr;

    if (useUniqueDests == TIBEMS_TRUE)
    {
        tmpStr = (char*)malloc(strlen(destination)+17);
        sprintf(tmpStr, "%s.%d", destination, num);
    }
    else
    {
        tmpStr = stringdup(destination);
    }

    if (useTopic == TIBEMS_TRUE)
    {
        status = tibemsTopic_Create(dest, tmpStr);
    }
    else
    {
        status = tibemsQueue_Create(dest, tmpStr);
    }

    if (status != TIBEMS_OK)
    {
        fail("Error creating tibemsDestination", NULL);
    }

    free(tmpStr);
}

/*---------------------------------------------------------------------
 * multicast exception callback
 *---------------------------------------------------------------------*/
void multicastExceptionCb(tibemsConnection connection, tibemsSession session,
                          tibemsMsgConsumer consumer, tibems_status error,
                          const char* description, void* closure)
{
    tibems_status               status;

    printf("EMS Multicast Exception: %s\n", description);

    status = tibemsSession_Close(session);
    if (status != TIBEMS_OK)
    {
        printf("Error closing tibemsSession: %s\n", tibemsStatus_GetText(status));
    }
}

/*---------------------------------------------------------------------
 * run test
 *---------------------------------------------------------------------*/
THREAD_RETVAL run(void* a)
{
    runArgs*                    args         = (runArgs*) a;
    tibems_long                 currentTime;
    tibems_long                 firstTime;
    tibemsDestination           dest;
    tibemsSession               session;
    tibemsMsgConsumer           consumer;
    tibemsMsg                   msg;
    tibems_status               status;
    tibems_long                 remaining    = 0;
    char*                       durableName  = NULL;
    tibems_int                  msgCount     = 0;
    tibemsErrorContext          errorContext = NULL;
    tibems_bool                 isMulticast  = TIBEMS_FALSE;

    /* Each thread has it's own error context. */
    status = tibemsErrorContext_Create(&errorContext);

    if (status != TIBEMS_OK)
    {
        printf("ErrorContext create failed: %s\n", tibemsStatus_GetText(status));
        exit(1);
    }

    tibems_Sleep(250);

    /* create the destination */
    createDestination(args->number, &dest);

    /* create the session */
    status = tibemsConnection_CreateSession(args->connection, 
                                            &session,
                                            txnSize > 0 ? TIBEMS_TRUE : TIBEMS_FALSE,
                                            ackMode);
    if (status != TIBEMS_OK)
    {
        fail("Error creating tibemsSession", errorContext);
    }

    /* create the consumer */
    if (durable != NULL)
    {
        durableName = (char*)malloc(strlen(durable)+17);
        sprintf(durableName, "%s.%d", durable, args->number);
        status = tibemsSession_CreateDurableSubscriber(session,
                                                       &consumer,
                                                       dest,
                                                       durableName,
                                                       selector,
                                                       TIBEMS_FALSE);
    }
    else
    {
        status = tibemsSession_CreateConsumer(session,
                                              &consumer,
                                              dest,
                                              selector,
                                              TIBEMS_FALSE);
    }
    if (status != TIBEMS_OK)
    {
        fail("Error creating tibemsMsgConsumer", errorContext);
    }

    status = tibems_IsConsumerMulticast(consumer, &isMulticast);
    if (status != TIBEMS_OK)
    {
        fail("Error getting tibemsMsgConsumer multicast status", errorContext);
    }

    if (isMulticast == TIBEMS_TRUE)
    {
        status = tibems_SetMulticastExceptionListener(multicastExceptionCb, NULL);
        if (status != TIBEMS_OK)
        {
            fail("Error setting multicast exception listener", errorContext);
        }
    }

    if (runTime > 0)
    {
        firstTime = currentMillis();
        remaining = runTime;
    }

    /* process messages */
    while (count == 0 || msgCount < (count/threads))
    {
        /* receive next message */
        if (runTime > 0)
        {
            if (remaining <= 0)
            {
                break;
            }
            status = tibemsMsgConsumer_ReceiveTimeout(consumer, &msg, remaining);
            currentTime = currentMillis();
            remaining = runTime - (currentTime - firstTime);
        }
        else
        {
            status = tibemsMsgConsumer_Receive(consumer, &msg);
        }

        if (status == TIBEMS_TIMEOUT)
        {
            break;
        }
        else if (status != TIBEMS_OK)
        {
            fail("Error receiving message", errorContext);
        }

        /* start timing */
        if (msgCount == 0)
        {
            args->start = currentMillis();
        }

        msgCount++;

        /* acknowledge message if necessary */
        if (ackMode == TIBEMS_CLIENT_ACKNOWLEDGE ||
            ackMode == TIBEMS_EXPLICIT_CLIENT_ACKNOWLEDGE ||
            ackMode == TIBEMS_EXPLICIT_CLIENT_DUPS_OK_ACKNOWLEDGE)
        {
            status = tibemsMsg_Acknowledge(msg);
            if (status != TIBEMS_OK)
            {
                fail("Error acknowledging message", errorContext);
            }
        }

        /* commit transactions as necessary */
        if (txnSize > 0 && (msgCount % txnSize == 0))
        {
            status = tibemsSession_Commit(session);
            if (status != TIBEMS_OK)
            {
                fail("Error committing transaction", errorContext);
            }
        }

        /* destroy the message */
        status = tibemsMsg_Destroy(msg);
        if (status != TIBEMS_OK)
        {
            fail("Error destroying tibemsMsg", errorContext);
        }
    }

    /* commit any outstanding messages */
    if (txnSize > 0)
    {
        status = tibemsSession_Commit(session);
        if (status != TIBEMS_OK)
        {
            fail("Error committing transaction", errorContext);
        }
    }

    /* stop timing */
    args->stop = currentMillis();
    args->count = msgCount;

    /* close the consumer */
    status = tibemsMsgConsumer_Close(consumer);
    if (status != TIBEMS_OK)
    {
        fail("Error closing tibemsMsgConsumer", errorContext);
    }

    /* unsubscribe durable subscription */
    if (durableName != NULL)
    {
        status = tibemsSession_Unsubscribe(session, durableName);
        if (status != TIBEMS_OK)
        {
            fail("Error unsubscribing", errorContext);
        }

        free(durableName);
    }

    /* close the session */
    status = tibemsSession_Close(session);
    if (status != TIBEMS_OK)
    {
        fail("Error closing tibemsMsgConsumer", errorContext);
    }

    /* destroy destination */
    status = tibemsDestination_Destroy(dest);
    if (status != TIBEMS_OK)
    {
        fail("Error destroying tibemsDestination", errorContext);
    }

    tibemsErrorContext_Close(errorContext);
    return 0;
}

/*-----------------------------------------------------------------------
 * main
 *----------------------------------------------------------------------*/
int main(int argc, char** argv)
{
    runArgs*                    threadArgs;
    THREAD_OBJ*                 threadArray;
    tibemsConnection*           connArray;
    tibems_status               status       = TIBEMS_OK;
    tibems_int                  i;
    tibems_int                  total        = 0;
    tibems_double               rate;
    tibems_long                 start        = 0;
    tibems_long                 stop         = 0;
    tibemsConnectionFactory     factory      = NULL;

    /* parse arguments */
    parseArgs(argc,argv);

    printf("------------------------------------------------------------------------\n");
    printf("tibemsMsgConsumerPerf\n");
    printf("------------------------------------------------------------------------\n");
    printf("Server....................... %s\n", serverUrl != NULL ? serverUrl : "localhost");
    printf("User......................... %s\n", username != NULL ? username : "(NULL)");
    printf("Destination.................. %s\n", destination != NULL ? destination : "(NULL)");
    printf("Consumer Threads............. %d\n", threads);
    printf("Consumer Connections......... %d\n", connections);
    printf("Acknowledge Mode............. %s\n", ackModeToString(ackMode));
    printf("Durable...................... %s\n", durable ? "true" : "false");
    printf("Selector..................... %s\n", selector ? selector : "null");
    if (txnSize > 0)
    {
        printf("Transaction Size............. %d\n", txnSize);
    }
    printf("------------------------------------------------------------------------\n\n");

    status = tibemsErrorContext_Create(&gErrorContext);

    if (status != TIBEMS_OK)
    {
        printf("ErrorContext create failed: %s\n", tibemsStatus_GetText(status));
        exit(1);
    }

    factory = tibemsConnectionFactory_Create();
    if (!factory)
    {
        printf("Error creating tibemsConnectionFactory\n");
    }

    status = tibemsConnectionFactory_SetServerURL(factory,serverUrl);
    if (status != TIBEMS_OK) 
    {
        fail("Error setting server url", gErrorContext);
    }

    /* create the connections */
    connArray = (tibemsConnection*)malloc(sizeof(tibemsConnection)*connections);
    for (i = 0; i < connections; i++)
    {
        status = tibemsConnectionFactory_CreateConnection(factory,&connArray[i],
                                                          username,password);
        if (status != TIBEMS_OK)
        {
            fail("Error creating tibemsConnection", gErrorContext);
        }

        status = tibemsConnection_Start(connArray[i]);
        if (status != TIBEMS_OK)
        {
            fail("Error starting tibemsConnection", gErrorContext);
        }
    }

    /* start the threads */
    threadArgs = (runArgs*)malloc(sizeof(runArgs)*threads);
    threadArray = (THREAD_OBJ*)malloc(sizeof(THREAD_OBJ)*threads);
    for (i = 0; i < threads; i++)
    {
        threadArgs[i].connection = connArray[i % connections];
        threadArgs[i].number = i+1;
        THREAD_CREATE(threadArray[i], &run, &threadArgs[i]);
    }

    /* wait for completion */
    ThreadJoin(threads, threadArray);

    /* gather rates */
    for (i = 0; i < threads; i++)
    {
        total += threadArgs[i].count;
        if (threadArgs[i].start < start || start == 0)
        {
            start = threadArgs[i].start;
        }
        if (threadArgs[i].stop > stop)
        {
            stop = threadArgs[i].stop;
        }
    }

    if ( stop > start )
    {
        rate = (total*1000.0)/(double)(stop-start);
        
        printf("%d times took %d milliseconds, performance is %.0f messages/second\n",
               total, (tibems_int)(stop-start), rate);
    }
    else
    {
        printf("interval too short to calculate a message rate\n");
    }

    /* cleanup */
    for (i = 0; i < connections; i++)
    {
        status = tibemsConnection_Close(connArray[i]);
        if (status != TIBEMS_OK)
        {
            fail("Error closing tibemsConnection", gErrorContext);
        }
    }

    free(connArray);
    free(threadArgs);
    free(threadArray);
    tibemsErrorContext_Close(gErrorContext);

    return 0;
}

