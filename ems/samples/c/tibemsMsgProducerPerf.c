/* 
 * Copyright (c) 2006-$Date: 2016-12-13 17:00:37 -0600 (Tue, 13 Dec 2016) $ TIBCO Software Inc. 
 * Licensed under a BSD-style license. Refer to [LICENSE]
 * For more information, please contact:
 * TIBCO Software Inc., Palo Alto, California, USA
 * 
 * $Id: tibemsMsgProducerPerf.c 90180 2016-12-13 23:00:37Z olivierh $
 * 
 */

/*
 * Usage:  tibemsMsgProducerPerf [options]
 *
 *  where options are:
 *
 *   -server       <url>         EMS server URL. Default is
 *                               "tcp://localhost:7222".
 *   -user         <username>    User name. Default is null.
 *   -password     <password>    User password. Default is null.
 *   -topic        <topic-name>  Topic name. Default is "topic.sample".
 *   -queue        <queue-name>  Queue name. No default.
 *   -size         <num bytes>   Message payload size in bytes. Default is 100.
 *   -count        <num msgs>    Number of messages to send. Default is 10k.
 *   -time         <seconds>     Number of seconds to run. Default is 0.
 *   -delivery     <mode>        Delivery mode. Default is NON_PERSISTENT.
 *                               Other values: PERSISTENT and RELIABLE.
 *   -threads      <num threads> Number of producer threads. Default is 1.
 *   -connections  <num conns>   Number of producer connections. Default is 1.
 *   -txnsize      <num msgs>    Number of nessages per transaction. Default 0.
 *   -rate         <msg/sec>     Message rate for producer threads.
 *   -payload      <file name>   File containing message payload.
 *   -uniquedests                Each producer thread uses a unique destination.
 *   -compression                Enable message compression.
 */

#include "tibemsUtilities.h"

typedef struct _runArgs
{
    tibemsConnection connection;
    tibems_int                  number;
    tibems_int                  count;
    tibems_long                 start;
    tibems_long                 stop;
    tibems_long                 completionTime;
} runArgs;

typedef struct __completionListenerFields
{
    tibems_int                  completionCount;
    tibems_bool                 finished;
    volatile tibems_int         finishCount;
    runArgs*                    runArgs;

} _completionListenerFields, *completionListenerFields;

completionListenerFields
completionListenerFields_create(
    runArgs* args)
{
    completionListenerFields newCl = NULL;

    newCl = (completionListenerFields)calloc(1, sizeof(_completionListenerFields));
    newCl->runArgs = args;

    return newCl;
}

void
completionListenerFields_Destroy(
    completionListenerFields cl)
{
    if (cl)
        free(cl);
}

void
completionListenerFields_SetFinishCount(
    completionListenerFields  clFields,
    int                       count)
{
    clFields->finishCount = count;
}

void
onCompletion(
    tibemsMsg     msg,
    tibems_status status,
    void*         args)
{
    completionListenerFields cl = (completionListenerFields)args;

    if (status != TIBEMS_OK)
    {
        printf("Error sending message: %s.\n",
            tibemsStatus_GetText(status));

        return;
    }

    cl->completionCount++; 
 
    if (cl->finished) 
        return; 
 
    if (cl->finishCount > 0 && cl->completionCount >= cl->finishCount) 
    { 
        cl->finished = TIBEMS_TRUE; 
        cl->runArgs->completionTime = currentMillis();
    } 
}

/*-----------------------------------------------------------------------
 * Parameters
 *----------------------------------------------------------------------*/
char*                           serverUrl      = "tcp://localhost:7222";
char*                           username       = NULL;
char*                           password       = NULL;
char*                           destination    = "topic.sample";
char*                           payloadFile    = NULL;
tibems_int                      txnSize        = 0;
tibems_int                      count          = 0;
tibems_long                     runTime        = 0;
tibems_int                      threads        = 1;
tibems_int                      connections    = 1;
tibems_int                      deliveryMode   = TIBEMS_NON_PERSISTENT;
tibems_bool                     async          = TIBEMS_FALSE;
tibems_int                      msgSize        = 0;
tibems_int                      msgRate        = 0;
tibems_bool                     useUniqueDests = TIBEMS_FALSE;
tibems_bool                     useTopic       = TIBEMS_TRUE;
tibems_bool                     useCompression = TIBEMS_FALSE;

tibems_long                     completionTime = 0;

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
    printf("\nUsage: tibemsMsgProducerPerf [options]\n");
    printf("\n");
    printf("   where options are:\n");
    printf("\n");
    printf(" -server       <url>         Server URL. Default is \"tcp://localhost:7222\".\n");
    printf(" -user         <username>    User name. Default is null.\n");
    printf(" -password     <password>    User password. Default is null.\n");
    printf(" -topic        <name>        Topic name. Default is \"topic.sample\".\n");
    printf(" -queue        <name>        Queue name. No default.\n");
    printf(" -size         <bytes>       Message payload size in bytes. Default is 0.\n");
    printf(" -delivery     <mode>        Delivery mode: RELIABLE, NON_PERSISTENT, PERSISTENT\n");
    printf("                             Default is NON_PERSISTENT.\n");
    printf(" -txnsize      <num msgs>    Number of messages per transaction.\n");
    printf(" -count        <num msgs>    Number of messages to receive.\n");
    printf(" -time         <seconds>     Number of seconds to run.\n");
    printf(" -threads      <num threads> Number of producer threads.\n");
    printf(" -connections  <num conns>   Number of connections.\n");
    printf(" -rate         <msg/sec>     Message rate for producer threads.\n");
    printf(" -payload      <filename>    File containing message payload.\n");
    printf(" -uniquedests                Each producer uses a unique destination.\n");
    printf(" -compression                Enable message compression.\n");
    printf(" -async                      Send messages asynchronously.\n");
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
        else if (strcmp(argv[i],"-size")==0)
        {
            if ((i+1) >= argc) usage();
            msgSize = atoi(argv[i+1]);
            if (msgSize < 0)
            {
                printf("Invalid value for -size: %s\n", argv[i+1]);
                usage();
            }
            i += 2;
        }
        else if (strcmp(argv[i],"-delivery")==0)
        {
            if ((i+1) >= argc) usage();
            if (strcmp(argv[i+1], "RELIABLE") == 0)
            {
                deliveryMode = TIBEMS_RELIABLE;
            }
            else if (strcmp(argv[i+1], "NON_PERSISTENT") == 0)
            {
                deliveryMode = TIBEMS_NON_PERSISTENT;
            }
            else if (strcmp(argv[i+1], "PERSISTENT") == 0)
            {
                deliveryMode = TIBEMS_PERSISTENT;
            }
            else
            {
                printf("Invalid value for -delivery: %s\n", argv[i+1]);
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
        else if (strcmp(argv[i],"-rate")==0)
        {
            if ((i+1) >= argc) usage();
            msgRate = atoi(argv[i+1]);
            i += 2;
        }
        else if (strcmp(argv[i],"-payload")==0)
        {
            if ((i+1) >= argc) usage();
            payloadFile = argv[i+1];
            i += 2;
        }
        else if (strcmp(argv[i],"-uniquedests")==0)
        {
            useUniqueDests = TIBEMS_TRUE;
            i += 1;
        }
        else if (strcmp(argv[i],"-compression")==0)
        {
            useCompression = TIBEMS_TRUE;
            i += 1;
        }
        else if (strcmp(argv[i],"-async")==0)
        {
            async = TIBEMS_TRUE;
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
 * delivery mode to string
 *---------------------------------------------------------------------*/
char* deliveryModeToString(tibems_int delmode)
{
    switch (delmode)
    {
    case TIBEMS_RELIABLE:
        return "RELIABLE";
    case TIBEMS_NON_PERSISTENT:
        return "NON_PERSISTENT";
    case TIBEMS_PERSISTENT:
        return "PERSISTENT";
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

/*-----------------------------------------------------------------------
 * create the message
 *----------------------------------------------------------------------*/
void createMessage(tibemsMsg* msg)
{
    tibems_status               status;
    char*                       bytes   = 0;

    status = tibemsBytesMsg_Create(msg);
    if (status != TIBEMS_OK)
    {
        fail("Error creating tibemsMsg", NULL);
    }

    if (payloadFile)
    {
        FILE* file;

        file = fopen(payloadFile, "rb");
        if (file == NULL)
        {
            printf("Error opening %s: %s\n", payloadFile, strerror(errno));
        }
        else
        {
            fseek(file, 0L, SEEK_END);
            msgSize = ftell(file);
            rewind(file);
            bytes = malloc(msgSize);
            fread(bytes, msgSize, 1, file);
            fclose(file);
        }

    } 
    else if (msgSize > 0)
    {
        tibems_int i;
        char       c = 'A';
        bytes = malloc(msgSize);
        for (i = 0; i < msgSize; i++)
        {
            bytes[i] = c++;
            if (c > 'z')
                c = 'A';
        }
    }

    if (bytes)
    {
        status = tibemsBytesMsg_WriteBytes(*msg, bytes, msgSize);
        if (status != TIBEMS_OK)
        {
            fail("Error writing bytes to message", NULL);
        }
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
    tibemsMsgProducer           producer;
    tibemsMsg                   msg;
    tibems_status               status;
    tibems_int                  msgCount     = 0;
    /* for message rate checking */
    tibems_long                 sampleStart  = 0;
    tibems_int                  sampleTime   = 10;
    tibems_int                  sampleCount  = 0;

    tibemsErrorContext          errorContext = NULL;

    completionListenerFields    clArgs = NULL;

    /* Each thread has it's own error context. */
    status = tibemsErrorContext_Create(&errorContext);

    if (status != TIBEMS_OK)
    {
        printf("ErrorContext create failed: %s\n", tibemsStatus_GetText(status));
        exit(1);
    }

    if (async)
    {
        clArgs = completionListenerFields_create(args);
        if (!clArgs)
        {
            printf("Unable to create completion listener arguments.\n");
            exit(1);
        }
    }

    tibems_Sleep(500);

    /* create the destination */
    createDestination(args->number, &dest);

    /* create the session */
    status = tibemsConnection_CreateSession(args->connection, 
                                            &session,
                                            txnSize > 0 ? TIBEMS_TRUE : TIBEMS_FALSE,
                                            TIBEMS_AUTO_ACKNOWLEDGE);
    if (status != TIBEMS_OK)
    {
        fail("Error creating tibemsSession", errorContext);
    }

    /* create the producer */
    status = tibemsSession_CreateProducer(session,
                                          &producer,
                                          dest);
    if (status != TIBEMS_OK)
    {
        fail("Error creating tibemsMsgProducer", errorContext);
    }

    /* set the delivery mode */
    status = tibemsMsgProducer_SetDeliveryMode(producer,
                                               deliveryMode);
    if (status != TIBEMS_OK)
    {
        fail("Error setting delivery mode", errorContext);
    }

    /* performance settings */
    status = tibemsMsgProducer_SetDisableMessageID(producer, 
                                                   TIBEMS_TRUE) ||
             tibemsMsgProducer_SetDisableMessageTimestamp(producer,
                                                          TIBEMS_TRUE);
    if (status != TIBEMS_OK)
    {
        fail("Error configuring tibemsMsgProducer", errorContext);
    }

    /* create the message */
    createMessage(&msg);

    if (useCompression == TIBEMS_TRUE)
    {
        status = tibemsMsg_SetBooleanProperty(msg, 
                                              "JMS_TIBCO_COMPRESS", 
                                              TIBEMS_TRUE);
        if (status != TIBEMS_OK)
        {
            fail("Error setting compression property", errorContext);
        }
    }

    /* start timing */
    args->start = currentMillis();

    if (runTime > 0)
        firstTime = args->start;

    /* process messages */
    while (count == 0 || msgCount < (count/threads))
    {
        /* send next message */
        if (runTime > 0)
        {
            if (async)
                status = tibemsMsgProducer_AsyncSend(producer, msg, onCompletion, clArgs);
            else
                status = tibemsMsgProducer_Send(producer, msg);

            currentTime = currentMillis();
            if (runTime < (currentTime - firstTime))
            {
                break;
            }
        }
        else
        {
            if (async)
                status = tibemsMsgProducer_AsyncSend(producer, msg, onCompletion, clArgs);
            else
                status = tibemsMsgProducer_Send(producer, msg);
        }

        if (status != TIBEMS_OK)
        {
            fail("Error sending message", errorContext);
        }

        msgCount++;

        /* commit transactions as necessary */
        if (txnSize > 0 && (msgCount % txnSize == 0))
        {
            status = tibemsSession_Commit(session);
            if (status != TIBEMS_OK)
            {
                fail("Error committing transaction", errorContext);
            }
        }

        /* check message rate */
        if (msgRate > 0)
        {
            if (msgRate < 100)
            {
                if (msgCount % 10 == 0)
                {
                    tibems_long sleepval = (tibems_long)((10.0/msgRate)*1000);
                    tibems_Sleep(sleepval);
                }
            }
            else
            {
                tibems_long elapsed = currentMillis() - sampleStart;
                if (elapsed >= sampleTime)
                {
                    tibems_int actualMsgs = msgCount - sampleCount;
                    tibems_int expectedMsgs = (tibems_int)(elapsed*(msgRate/1000.0));
                    if (actualMsgs > expectedMsgs)
                    {
                        tibems_long sleepval = (tibems_long)
                            ((actualMsgs-expectedMsgs)/(msgRate/1000.0));
                        tibems_Sleep(sleepval);
                        if (sampleTime > 20)
                            sampleTime -= 10;
                    }
                    else
                    {
                        if (sampleTime < 300)
                            sampleTime += 10;
                    }
                    sampleStart = currentMillis();
                    sampleCount = msgCount;
                }
            }
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

    if (async)
    {
        completionListenerFields_SetFinishCount(clArgs, msgCount);
        /* the completion listener will finish and set the completion
         * field upon the last listener.  Closing the producer will
         * block until all the send completion events have been
         * processed.
         */
    }

    /* close the producer */
    status = tibemsMsgProducer_Close(producer);
    if (status != TIBEMS_OK)
    {
        fail("Error closing tibemsMsgProducer", errorContext);
    }

    /* destroy the message */
    status = tibemsMsg_Destroy(msg);
    if (status != TIBEMS_OK)
    {
        fail("Error destroying tibemsMsg", errorContext);
    }

    /* destroy destination */
    status = tibemsDestination_Destroy(dest);
    if (status != TIBEMS_OK)
    {
        fail("Error destroying tibemsDestination", errorContext);
    }

    if (async)
        completionListenerFields_Destroy(clArgs);

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
    tibems_status               status      = TIBEMS_OK;
    tibems_int                  i;
    tibems_int                  total       = 0;
    tibems_double               rate;
    tibems_long                 start       = 0;
    tibems_long                 stop        = 0;
    tibemsConnectionFactory     factory     = NULL;

    /* parse arguments */
    parseArgs(argc,argv);

    printf("------------------------------------------------------------------------\n");
    printf("tibemsMsgProducerPerf\n");
    printf("------------------------------------------------------------------------\n");
    printf("Server....................... %s\n", serverUrl != NULL ? serverUrl : "localhost");
    printf("User......................... %s\n", username != NULL ? username : "(NULL)");
    printf("Destination.................. %s\n", destination != NULL ? destination : "(NULL)");
    if (payloadFile)
        printf("Message Size................. %s\n", payloadFile);
    else
        printf("Message Size................. %d\n", msgSize);
    printf("Producer Threads............. %d\n", threads);
    printf("Producer Connections......... %d\n", connections);
    printf("Delivery Mode................ %s\n", deliveryModeToString(deliveryMode));
    printf("Compression.................. %s\n", useCompression == TIBEMS_TRUE ? "TRUE" : "FALSE");
    printf("Asynchronous Sending......... %s\n", async == TIBEMS_TRUE ? "TRUE" : "FALSE");
    if (txnSize > 0)
    {
        printf("Transaction Size............. %d\n", txnSize);
    }
    printf("------------------------------------------------------------------------\n\n");

    printf("Publishing to destination '%s'\n\n", destination);

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
        if (threadArgs[i].completionTime > completionTime)
        {
            completionTime = threadArgs[i].completionTime;
        }
    }

    if ( stop > start )
    {
        rate = (total*1000.0)/(double)(stop-start);
        
        printf("%d times took %d milliseconds, performance is %.0f messages/second\n",
               total, (tibems_int)(stop-start), rate);

        if (async) 
        { 
           if (completionTime > start) 
           { 
               rate = (total*1000.0)/(double)(completionTime-start);

               printf("%d completion listeners took %d milliseconds, performance is %.0f calls/second\n",
                   total, (tibems_int)(completionTime-start), rate);
           } 
           else 
           { 
               printf("interval too short to calculate a completion listener rate\n"); 
           } 
        }
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

