/*
 * Copyright (c) 2001-$Date: 2017-03-08 09:49:09 -0600 (Wed, 08 Mar 2017) $ TIBCO Software Inc.
 * Licensed under a BSD-style license. Refer to [LICENSE]
 * For more information, please contact:
 * TIBCO Software Inc., Palo Alto, California, USA
 *
 * $Id: tibemsXAMsgProducer.c 92163 2017-03-08 15:49:09Z krailton $
 *
 */

/*
 * This sample demonstrates how to use the x/Open DTP XA interface
 * for sending messages.
 *
 * Note that the XA calls are normally made by the Transaction
 * Manager, not directly by the application being managed by the TM.
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
 * Usage:  tibemsXAMsgProducer  [options]
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
 * On most platforms, declaring the switch vector is sufficent.
 * On windows the __declspec is required.
 *----------------------------------------------------------------------*/

#ifndef _WINDOWS
extern struct xa_switch_t tibemsXASwitch;
#else
__declspec(dllimport) struct xa_switch_t tibemsXASwitch;
#endif

/*-----------------------------------------------------------------------
 * Parameters
 *----------------------------------------------------------------------*/

char*           serverUrl    = NULL;
char*           userName     = NULL;
char*           name         = "topic.sample";
char**          data         = NULL;
int             datac        = 0;
tibems_bool     useTopic     = TIBEMS_TRUE;
char*           xaOpenStr    = NULL;

/*-----------------------------------------------------------------------
 * Variables
 *----------------------------------------------------------------------*/
tibemsConnection   connection   = NULL;
tibemsSession      session      = NULL;
tibemsMsgProducer  msgProducer  = NULL;
tibemsDestination  destination  = NULL;

tibemsErrorContext gErrorContext  = NULL;

/*-----------------------------------------------------------------------
 * usage
 *----------------------------------------------------------------------*/
void usage()
{
    printf("\nUsage: tibemsXAMsgProducer [options] [ssl options]\n");
    printf("                             <message-text-1>\n");
    printf("                             [<message-text-2>] ...\n");
    printf("\n");
    printf("   where options are:\n");
    printf("\n");
    printf("   -server   <server URL>  - EMS server URL, default is local server\n");
    printf("   -user     <user name>   - user name, default is null\n");
    printf("   -password <password>    - password, default is null\n");
    printf("   -topic    <topic-name>  - topic name, default is \"topic.sample\"\n");
    printf("   -queue    <queue-name>  - queue name, no default\n");
    printf("   -help-ssl               - help on ssl parameters\n");
    exit(0);
}

/*---------------------------------------------------------------------
 * fail
 *---------------------------------------------------------------------*/
void fail(
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

    exit(1);
}

/*---------------------------------------------------------------------
 * xafail
 *---------------------------------------------------------------------*/
void xafail(
    const char* message,
    int rc)
{
    const char* str = NULL;

    printf("XA ERROR: %s\n",message);
    printf("\tRETURN VALUE: %d\n", rc);

    tibemsErrorContext_GetLastErrorString(gErrorContext, &str);
    printf("\nLast error message =\n%s\n", str);
    tibemsErrorContext_GetLastErrorStackTrace(gErrorContext, &str);
    printf("\nStack trace = \n%s\n", str);
}

/*-----------------------------------------------------------------------
 * parseArgs
 *----------------------------------------------------------------------*/
void parseArgs(int argc, char** argv)
{
    int i;
    int max=2;
    int total=0;
    int len;
    tibems_bool append;
    tibems_bool hasArg;
    char** assignParam;

    /*
     * determine the worst-case length for the xa open string.
     */
    for (i=1; i<argc; i++)
        max += (int)strlen(argv[i]) + 1;

    /*
     * allocate the xa open string.
     */
    xaOpenStr = (char*)malloc(max);
    if (xaOpenStr == NULL)
        fail("Error allocating xa open string", TIBEMS_NO_MEMORY);

    /*
     * ensure string is not empty so that xa_open_entry will always
     * create the XA connection and session.
     */
    *xaOpenStr = ' ';
    total = 1;

    /*
     * handle the help, serverUrl, queue, topic, and userName arguments.
     * fill in the xa open string, skipping the args for the destination
     * as well as the strings that are sent as messages.
     * determine where the strings to be sent are located.
     */
    i = 1;
    while(i < argc)
    {
        append = TIBEMS_FALSE;
        hasArg = TIBEMS_FALSE;
        assignParam = NULL;

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
            append = TIBEMS_TRUE;
            hasArg = TIBEMS_TRUE;
            assignParam = &serverUrl;
        }
        else
        if (strcmp(argv[i],"-topic")==0)
        {
            useTopic = TIBEMS_TRUE;
            hasArg = TIBEMS_TRUE;
            assignParam = &name;
        }
        else
        if (strcmp(argv[i],"-queue")==0)
        {
            useTopic = TIBEMS_FALSE;
            hasArg = TIBEMS_TRUE;
            assignParam = &name;
        }
        else
        if (strcmp(argv[i],"-user")==0)
        {
            append = TIBEMS_TRUE;
            hasArg = TIBEMS_TRUE;
            assignParam = &userName;
        }
        else
        if (strncmp(argv[i],"-",1)==0)
        {
            append = TIBEMS_TRUE;
            hasArg = TIBEMS_TRUE;
        }
        else
        {
            data = &(argv[i]);
            datac = argc - i;
            break;
        }

        if (append == TIBEMS_TRUE)
        {
            len = (int)strlen(argv[i]);
            memcpy(xaOpenStr + total, argv[i], len);
            total += len;
            xaOpenStr[total] = ' ';
            total += 1;
        }

        if (hasArg == TIBEMS_FALSE)
            continue;

        if ((i+1) >= argc)
            usage();
        ++i;

        if (assignParam != NULL)
            *assignParam = argv[i];

        if (append == TIBEMS_TRUE)
        {
            len = (int)strlen(argv[i]);
            memcpy(xaOpenStr + total, argv[i], len);
            total += len;
            xaOpenStr[total] = ' ';
            total += 1;
        }

        ++i;
    }

    xaOpenStr[total] = '\0';
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

/*-----------------------------------------------------------------------
 * run
 *----------------------------------------------------------------------*/
void run()
{
    tibems_status       status          = TIBEMS_OK;
    tibemsTextMsg       msg             = NULL;
    struct xa_switch_t* xaSwitch        = &tibemsXASwitch;
    int                 xaReturn        = XA_OK;
    XID                 xid;
    int                 i;

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

    status = tibemsErrorContext_Create(&gErrorContext);
    if (status != TIBEMS_OK)
    {
        printf("ErrorContext create failed: %s\n", tibemsStatus_GetText(status));
        exit(1);
    }

    /*
     * Open classic XA interface.  This creates a connection for this
     * URL, if none already exists.  It also creates a session for this
     * thread and associates it with the RMID.
     */
    xaReturn = (xaSwitch->xa_open_entry)(xaOpenStr, 1, TMNOFLAGS);
    if (xaReturn != XA_OK)
    {
        xafail("Error opening xa", xaReturn);
        exit(1);
    }

    printf("Publishing to destination '%s'\n\n",name);

    /* get the connection created by the xa_open_entry */
    status = tibemsXAConnection_Get(&connection,serverUrl);
    if (status != TIBEMS_OK)
    {
        fail("Error getting tibemsConnection", status);
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

    /* get the session created by the xa_open_entry */
    status = tibemsXAConnection_GetXASession(connection,&session);
    if (status != TIBEMS_OK)
    {
        fail("Error creating tibemsSession", status);
    }

    /* create the producer */
    status = tibemsSession_CreateProducer(session,
            &msgProducer,destination);
    if (status != TIBEMS_OK)
    {
        fail("Error creating tibemsMsgProducer", status);
    }

    /* start transaction */
    initXID(&xid);
    xaReturn = (xaSwitch->xa_start_entry)(&xid, 1, TMNOFLAGS);
    if (xaReturn != XA_OK)
    {
        xafail("Error starting xa", xaReturn);
        exit(1);
    }

    /* publish messages */
    for (i = 0; i<datac; i++)
    {
        /* create the text message */
        status = tibemsTextMsg_Create(&msg);
        if (status != TIBEMS_OK)
        {
            fail("Error creating tibemsTextMsg", status);
        }

        /* set the message text */
        status = tibemsTextMsg_SetText(msg,data[i]);
        if (status != TIBEMS_OK)
        {
            fail("Error setting tibemsTextMsg text", status);
        }

        /* publish the message */
        status = tibemsMsgProducer_Send(msgProducer,msg);
        if (status != TIBEMS_OK)
        {
            fail("Error publishing tibemsTextMsg", status);
        }
        printf("Published message: %s\n",data[i]);

        /* destroy the message */
        status = tibemsMsg_Destroy(msg);
        if (status != TIBEMS_OK)
        {
            fail("Error destroying tibemsTextMsg", status);
        }
    }

    /* end transaction */
    xaReturn = (xaSwitch->xa_end_entry)(&xid, 1, TMSUCCESS);
    if (xaReturn != XA_OK)
    {
        xafail("Error ending xa", xaReturn);
        (xaSwitch->xa_rollback_entry)(&xid, 1, TMNOFLAGS);
        exit(1);
    }

    /* prepare transaction */
    xaReturn = (xaSwitch->xa_prepare_entry)(&xid, 1, TMNOFLAGS);
    if (xaReturn != XA_OK)
    {
        xafail("Error preparing xa", xaReturn);
        (xaSwitch->xa_rollback_entry)(&xid, 1, TMNOFLAGS);
        exit(1);
    }

    /* commit transaction */
    xaReturn = (xaSwitch->xa_commit_entry)(&xid, 1, TMNOFLAGS);
    if (xaReturn != XA_OK)
    {
        xafail("Error committing xa", xaReturn);
        exit(1);
    }

    /* destroy the destination */
    status = tibemsDestination_Destroy(destination);
    if (status != TIBEMS_OK)
    {
        fail("Error destroying tibemsDestination", status);
    }

    /* close classic XA interface */
    xaReturn = (xaSwitch->xa_close_entry)("", 1, TMNOFLAGS);
    if (xaReturn != XA_OK)
    {
        xafail("Error closing xa", xaReturn);
        exit(1);
    }

    tibemsErrorContext_Close(gErrorContext);
}

/*-----------------------------------------------------------------------
 * main
 *----------------------------------------------------------------------*/
int main(int argc, char** argv)
{
    int i;

    parseArgs(argc,argv);

    /* print parameters */
    printf("\n------------------------------------------------------------------------\n");
    printf("tibemsXAMsgProducer SAMPLE\n");
    printf("------------------------------------------------------------------------\n");
    printf("Server....................... %s\n",serverUrl?serverUrl:"localhost");
    printf("User......................... %s\n",userName?userName:"(null)");
    printf("Destination.................. %s\n",name);
    printf("Message Text................. \n");
    for(i=0;i<datac;i++)
    {
        printf("\t%s \n", data[i]);
    }
    printf("------------------------------------------------------------------------\n\n");

    run();

    free(xaOpenStr);

    return 1;
}
