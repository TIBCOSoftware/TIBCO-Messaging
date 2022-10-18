/* 
 * Copyright (c) 2001-$Date: 2016-12-13 17:00:37 -0600 (Tue, 13 Dec 2016) $ TIBCO Software Inc. 
 * Licensed under a BSD-style license. Refer to [LICENSE]
 * For more information, please contact:
 * TIBCO Software Inc., Palo Alto, California, USA
 * 
 * $Id: tibemsMsg.c 90180 2016-12-13 23:00:37Z olivierh $
 * 
 */

/*
 * This sample demonstrates how enumerate message
 * properties and fields.
 *
 * This sample subscribes to specified destination and
 * receives messages published on that destination.
 *
 * Each received message is printed using functions which
 * allow to query the type of the message and enumerate
 * message properties, fields in the map message and in the
 * stream message.
 *
 * This sample quits when 'Enter' is pressed.
 *
 * Notice that the specified destination should exist in your configuration
 * or your topics/queues configuration file should allow
 * creation of the specified destination. 
 *
 * Usage:  tibemsMsg [options]
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

#include "tibemsUtilities.h"

/*-----------------------------------------------------------------------
 * Parameters
 *----------------------------------------------------------------------*/

char*                           serverUrl    = NULL;
char*                           userName     = NULL;
char*                           password     = NULL;
char*                           pk_password  = NULL;
char*                           destName     = "topic.sample";
tibems_bool                     useTopic     = TIBEMS_TRUE;

/*-----------------------------------------------------------------------
 * Variables
 *----------------------------------------------------------------------*/
tibemsConnectionFactory         factory      = NULL;
tibemsConnection                connection   = NULL;
tibemsSession                   session      = NULL;
tibemsMsgConsumer               msgConsumer  = NULL;
tibemsDestination               destination  = NULL;   

tibemsSSLParams                 sslParams    = NULL;

tibemsErrorContext              errorContext = NULL; 

/*-----------------------------------------------------------------------
 * usage
 *----------------------------------------------------------------------*/
void usage() 
{
    printf("\nUsage: tibemsMsg [options] [ssl options]\n");
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
        if (strcmp(argv[i],"-topic")==0) 
        {
            if ((i+1) >= argc) usage();
            destName = argv[i+1];
            i += 2;
        }
        else
        if (strcmp(argv[i],"-queue")==0) 
        {
            if ((i+1) >= argc) usage();
            destName = argv[i+1];
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

/*---------------------------------------------------------------------
 * print_field
 *---------------------------------------------------------------------*/
void print_field(
    tibemsMsgField*             field)
{
    switch (field->type)
    {
      case TIBEMS_NULL:
          printf("null");
          break;
      case TIBEMS_BOOL:
          printf("boolean:%s", field->data.boolValue?"true":"false");
          break;
      case TIBEMS_BYTE:
          printf("byte:%d", field->data.byteValue);
          break;
      case TIBEMS_SHORT:
          printf("short:%d", field->data.shortValue);
          break;
      case TIBEMS_INT:
          printf("int:%d", field->data.intValue);
          break;
      case TIBEMS_LONG:
          /* scale it back to int for printing */
          printf("long:%d", (int)field->data.longValue);
          break;
      case TIBEMS_FLOAT:
          printf("float:%f", field->data.floatValue);
          break;
      case TIBEMS_DOUBLE:
          printf("double:%f", field->data.doubleValue);
          break;
      case TIBEMS_UTF8:
          printf("string:'%s'", field->data.utf8Value);
          break;

      /* next two can not happen if it's a property field */

      case TIBEMS_WCHAR:
          printf("wide_char:0x%2x", field->data.wcharValue);
          break;

      case TIBEMS_BYTES:
          printf("bytes:[%d bytes]", field->size);
          break;

      /* following are proprietary extensions */

      case TIBEMS_MAP_MSG:
          printf("MAP-Message:{");
          tibemsMsg_Print(field->data.msgValue);
          printf("}");
          break;
      case TIBEMS_SHORT_ARRAY:
          printf("short-array:[%d elements]", (int)field->count);
          break;
      case TIBEMS_INT_ARRAY:
          printf("int-array:[%d elements]", (int)field->count);
          break;
      case TIBEMS_LONG_ARRAY:
          printf("long-array:[%d elements]", (int)field->count);
          break;
      case TIBEMS_FLOAT_ARRAY:
          printf("float-array:[%d elements]", (int)field->count);
          break;
      case TIBEMS_DOUBLE_ARRAY:
          printf("double-array:[%d elements]", (int)field->count);
          break;
      default:
          printf("Error: unknown field type");
          break;
    }
}

/*---------------------------------------------------------------------
 * print_message
 *---------------------------------------------------------------------*/
void print_message(
    tibemsMsg                   msg)
{
    tibems_status               status       = TIBEMS_OK;
    tibemsMsgType               message_type = TIBEMS_MESSAGE_UNKNOWN;
    const char*                 type_name    = "";
    const char*                 name;
    tibemsMsgField              field; /* this is not a pointer, it's a structure */
    const char*                 text;
    tibemsMsgEnum               enumeration;

    status = tibemsMsg_GetBodyType(msg,&message_type);
    if (status != TIBEMS_OK)
        fail("Error trying to obtain message type", errorContext);

    switch(message_type)
    {
    case TIBEMS_MESSAGE:
        type_name = "MESSAGE";
        break;
    case TIBEMS_BYTES_MESSAGE:
        type_name = "BYTES";
        break;
    case TIBEMS_MAP_MESSAGE:
        type_name = "MAP";
        break;
    case TIBEMS_OBJECT_MESSAGE:
        type_name = "OBJECT";
        break;
    case TIBEMS_STREAM_MESSAGE:
        type_name = "STREAM";
        break;
    case TIBEMS_TEXT_MESSAGE:
        type_name = "TEXT";
        break;
    default:
        /* this can not happen */
        type_name = "<UNKNOWN:Error>";
        break;
    }

    printf(">>>>>>>>> Received %s message:\n",type_name);

    /*
     * Print message properties
     */

    status = tibemsMsg_GetPropertyNames(msg,&enumeration);
    if (status != TIBEMS_OK)
        fail("Error trying to get properties enumerator", errorContext);

    printf("    Message Properties:\n");

    while((status = tibemsMsgEnum_GetNextName(enumeration,&name)) == TIBEMS_OK)
    {
        status = tibemsMsg_GetProperty(msg,name,&field);
        if (status != TIBEMS_OK)
            fail("Error trying to get property by name", errorContext);

        printf("        Name: %-16s   Value: ",name);
        print_field(&field);
        printf("\n");
    }

    tibemsMsgEnum_Destroy(enumeration);
    
    printf("    ----------- end of properties -----------\n");

    /* 
     * Print message body for Map, Stream and Text messages
     */

    if (message_type == TIBEMS_MAP_MESSAGE)
    {

        status = tibemsMapMsg_GetMapNames(msg,&enumeration);
        if (status != TIBEMS_OK)
            fail("Error trying to get map fields enumerator", errorContext);

        printf("    Map Fields:\n");

        while((status = tibemsMsgEnum_GetNextName(enumeration,&name)) == TIBEMS_OK)
        {
            status = tibemsMapMsg_GetField(msg,name,&field);
            if (status != TIBEMS_OK)
                fail("Error trying to get map field by name", errorContext);

            printf("        Name: %-16s   Value: ",name);
            print_field(&field);
            printf("\n");
        }

        tibemsMsgEnum_Destroy(enumeration);

        printf("    ----------- end of map fields -----------\n");
    }
    else
    if (message_type == TIBEMS_TEXT_MESSAGE)
    {
        status = tibemsTextMsg_GetText(msg,&text);
        if (status != TIBEMS_OK)
            fail("Error trying to get message text", errorContext);

        printf("    Message text: %s\n", text ? text : "(NULL)" );
    }
    else
    if (message_type == TIBEMS_STREAM_MESSAGE)
    {
        tibems_int idx = 1;
        
        printf("    Stream Fields:\n");

        while((status = tibemsStreamMsg_ReadField(msg,&field)) == TIBEMS_OK)
        {
            printf("        Index: %-3d   Value: ",idx);
            print_field(&field);
            printf("\n");
            idx++;
        }

        /* we should break out with status = TIBEMS_MSG_EOF otherwise
         * it's some kind of error
         */
        if (status != TIBEMS_MSG_EOF)
        {
            fail("Error trying to get message text", errorContext);
        }

        printf("    ----------- end of stream fields --------\n");
    }
    else
    {
        /* for bytes and stream we could only print number of bytes... */
    }

}

/*---------------------------------------------------------------------
 * onMessage
 *---------------------------------------------------------------------*/
void onMessage(
    tibemsMsgConsumer           consumer,
    tibemsMsg                   msg,
    void*                       closure)
{
    tibems_status               status = TIBEMS_OK;

    /* read messages */
    if (!msg)
        printf("Received NULL message!");

    print_message(msg);
    
    /* destroy the message */
    status = tibemsMsg_Destroy(msg);
    if (status != TIBEMS_OK)
    {
        fail("Error destroying tibemsMsg", errorContext);
    }
}

/*-----------------------------------------------------------------------
 * run
 *----------------------------------------------------------------------*/
void run() 
{
    tibems_status               status = TIBEMS_OK;
    tibems_int                  c; 
    
    if (!destName) {
        printf("***Error: must specify destination name\n");
        usage();
    }
    
    printf("Subscribing to destination: '%s'\n",destName);
    
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

    /* create the destination */
    if (useTopic)
        status = tibemsTopic_Create(&destination,destName);
    else
        status = tibemsQueue_Create(&destination,destName);
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
        
    /* create the consumer */
    status = tibemsSession_CreateConsumer(session,
            &msgConsumer,destination,NULL,TIBEMS_FALSE);
    if (status != TIBEMS_OK)
    {
        fail("Error creating tibemsMsgConsumer", errorContext);
    }

    /* set the message listener */
    status = tibemsMsgConsumer_SetMsgListener(msgConsumer, onMessage, NULL);
    if (status != TIBEMS_OK)
    {
        fail("Error setting message listener", errorContext);
    }
    
    /* start the connection */
    status = tibemsConnection_Start(connection);
    if (status != TIBEMS_OK)
    {
        fail("Error starting tibemsConnection", errorContext);
    }

    /* 'Enter' key quits this sample */
    printf("\nPress 'Enter' to quit tibemsMsg SAMPLE\n");
    c = getchar();


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
    parseArgs(argc,argv);

    /* print parameters */
    printf("------------------------------------------------------------------------\n");
    printf("tibemsMsg SAMPLE\n");
    printf("------------------------------------------------------------------------\n");
    printf("Server....................... %s\n",serverUrl?serverUrl:"localhost");
    printf("User......................... %s\n",userName?userName:"(null)");
    printf("Destination.................. %s\n",destName);
    printf("------------------------------------------------------------------------\n");

    run();

    return 1;
}
