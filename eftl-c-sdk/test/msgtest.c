/*
 * msgtest.c
 *
 *  Created on: May 8, 2017
 *      Author: bpeterse
 */

#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>

#include "eftl.h"

void checkField(
    tibeftlMessage msg,
    const char* field,
    tibeftlFieldType fieldType,
    bool isFieldSet)
{
	tibeftlErr err;
    tibeftlFieldType type;

    err = tibeftlErr_Create();

    if (tibeftlMessage_IsFieldSet(err, msg, field) != isFieldSet)
    {
        printf("tibeftlMessage_IsFieldSet for '%s' should return %s\n", field, (isFieldSet ? "true" : "false"));
        exit(EXIT_FAILURE);
    }

    if ((type = tibeftlMessage_GetFieldType(err, msg, field)) != fieldType)
    {
        printf("tibeftlMessage_GetFieldType for '%s' returned %d, should return %d\n", field, type, fieldType);
        exit(EXIT_FAILURE);
    }

    tibeftlErr_Destroy(err);
}

void print(tibeftlMessage msg)
{
    int len;
    char* buf;

    len = tibeftlMessage_ToString(NULL, msg, NULL, 0);
    if (len < 1)
    {
        printf("tibeftlMessage_ToString failed: %d\n", len);
        exit(EXIT_FAILURE);
    }

    buf = malloc(len);

    len = tibeftlMessage_ToString(NULL, msg, buf, len);
    if (len < 1)
    {
        printf("tibeftlMessage_ToString failed: %d\n", len);
        exit(EXIT_FAILURE);
    }

    printf("message: %s\n", buf);

    free(buf);
}

int main(int argc, char** argv)
{
	tibeftlErr err;
    tibeftlMessage msg;
    tibeftlMessage sub;
    char opaque[256];
    const char* stringvalue;
    const char* field;
    tibeftlFieldType type;
    int64_t longvalue;
    double doublevalue;
    int64_t timevalue;
    tibeftlMessage msgvalue;
    char* opaquevalue;
    char* stringarray[3] = {"abc", "def", "ghi"};
    char** stringarrayvalue;
    int64_t longarray[3] = {256, 512, 1024};
    int64_t* longarrayvalue;
    double doublearray[3] = {0.1, 0.2, 0.3};
    double* doublearrayvalue;
    int64_t timearray[3] = {0, 100000000, 200000000};
    int64_t* timearrayvalue;
    tibeftlMessage messagearray[3];
    tibeftlMessage* messagearrayvalue;
    size_t size;
    int len, i;

    err = tibeftlErr_Create();

    msg = tibeftlMessage_Create(err);
    if (!msg)
    {
        printf("tibeftlMessage_Create created failed\n");
        exit(EXIT_FAILURE);
    }

    // check an unset field
    checkField(msg, "invalid", TIBEFTL_FIELD_TYPE_UNKNOWN, false);

    // set bad array field
    tibeftlMessage_SetArray(err, msg, TIBEFTL_FIELD_TYPE_STRING, "invalid", (void*)stringarray, 3);
    if (tibeftlErr_GetCode(err) != TIBEFTL_ERR_INVALID_TYPE)
    {
        printf("tibeftlMessage_SetArray succeeded with invalid field type\n");
        exit(EXIT_FAILURE);
    }
    tibeftlErr_Clear(err);

    // add string field
    tibeftlMessage_SetString(err, msg, "string", "stringvalue");
    if (tibeftlErr_IsSet(err))
    {
        printf("tibeftlMessage_SetString failed\n");
        exit(EXIT_FAILURE);
    }
    checkField(msg, "string", TIBEFTL_FIELD_TYPE_STRING, true);
    stringvalue = tibeftlMessage_GetString(err, msg, "string");
    if (tibeftlErr_IsSet(err))
    {
        printf("tibeftlMessage_GetString failed\n");
        exit(EXIT_FAILURE);
    }
    if (strcmp(stringvalue, "stringvalue") != 0)
    {
        printf("tibeftlMessage_GetString failed: value does not match\n");
        exit(EXIT_FAILURE);
    }

    // add string array field
    tibeftlMessage_SetArray(err, msg, TIBEFTL_FIELD_TYPE_STRING_ARRAY, "stringarray", stringarray, 3);
    if (tibeftlErr_IsSet(err))
    {
        printf("tibeftlMessage_SetArray:string failed\n");
        exit(EXIT_FAILURE);
    }
    checkField(msg, "stringarray", TIBEFTL_FIELD_TYPE_STRING_ARRAY, true);
    stringarrayvalue = tibeftlMessage_GetArray(err, msg, TIBEFTL_FIELD_TYPE_STRING_ARRAY, "stringarray", &len);
    if (tibeftlErr_IsSet(err))
    {
        printf("tibeftlMessage_GetArray:string failed\n");
        exit(EXIT_FAILURE);
    }
    if (len != 3)
    {
        printf("tibeftlMessage_GetArray:string failed: lengths do not match\n");
        exit(EXIT_FAILURE);
    }
    for (i = 0; i < 3; i++)
    {
        if (strcmp(stringarrayvalue[i], stringarray[i]) != 0)
        {
            printf("tibeftlMessage_GetArray:string failed: values do not match\n");
            exit(EXIT_FAILURE);
        }
    }

    // get bad array type
    tibeftlMessage_GetArray(err, msg, TIBEFTL_FIELD_TYPE_STRING, "stringarray", NULL);
    if (tibeftlErr_GetCode(err) != TIBEFTL_ERR_INVALID_TYPE)
    {
        printf("tibeftlMessage_GetArray succeeded with invalid field type\n");
        exit(EXIT_FAILURE);
    }
    tibeftlErr_Clear(err);

    // add long field
    tibeftlMessage_SetLong(err, msg, "long", 42);
    if (tibeftlErr_IsSet(err))
    {
        printf("tibeftlMessage_SetLong failed\n");
        exit(EXIT_FAILURE);
    }
    checkField(msg, "long", TIBEFTL_FIELD_TYPE_LONG, true);
    longvalue = tibeftlMessage_GetLong(err, msg, "long");
    if (tibeftlErr_IsSet(err))
    {
        printf("tibeftlMessage_GetLong failed\n");
        exit(EXIT_FAILURE);
    }
    if (longvalue != 42)
    {
        printf("tibeftlMessage_GetLong failed: value does not match\n");
        exit(EXIT_FAILURE);
    }

    // add long array field
    tibeftlMessage_SetArray(err, msg, TIBEFTL_FIELD_TYPE_LONG_ARRAY, "longarray", longarray, 3);
    if (tibeftlErr_IsSet(err))
    {
        printf("tibeftlMessage_SetArray:long failed\n");
        exit(EXIT_FAILURE);
    }
    checkField(msg, "longarray", TIBEFTL_FIELD_TYPE_LONG_ARRAY, true);
    longarrayvalue = tibeftlMessage_GetArray(err, msg, TIBEFTL_FIELD_TYPE_LONG_ARRAY, "longarray", &len);
    if (tibeftlErr_IsSet(err))
    {
        printf("tibeftlMessage_GetArray:long failed\n");
        exit(EXIT_FAILURE);
    }
    if (len != 3)
    {
        printf("tibeftlMessage_GetArray:long failed: lengths do not match\n");
        exit(EXIT_FAILURE);
    }
    for (i = 0; i < 3; i++)
    {
        if (longarrayvalue[i] != longarray[i])
        {
            printf("tibeftlMessage_GetArray:long failed: values do not match\n");
            exit(EXIT_FAILURE);
        }
    }

    // add double field
    tibeftlMessage_SetDouble(err, msg, "double", 0.42);
    if (tibeftlErr_IsSet(err))
    {
        printf("tibeftlMessage_SetDouble failed\n");
        exit(EXIT_FAILURE);
    }
    checkField(msg, "double", TIBEFTL_FIELD_TYPE_DOUBLE, true);
    doublevalue = tibeftlMessage_GetDouble(err, msg, "double");
    if (tibeftlErr_IsSet(err))
    {
        printf("tibeftlMessage_GetDouble failed\n");
        exit(EXIT_FAILURE);
    }
    if (doublevalue != 0.42)
    {
        printf("tibeftlMessage_GetDouble failed: value does not match: %f vs 0.42\n", doublevalue);
        exit(EXIT_FAILURE);
    }

    // add double array field
    tibeftlMessage_SetArray(err, msg, TIBEFTL_FIELD_TYPE_DOUBLE_ARRAY, "doublearray", doublearray, 3);
    if (tibeftlErr_IsSet(err))
    {
        printf("tibeftlMessage_SetArray:double failed\n");
        exit(EXIT_FAILURE);
    }
    checkField(msg, "doublearray", TIBEFTL_FIELD_TYPE_DOUBLE_ARRAY, true);
    doublearrayvalue = tibeftlMessage_GetArray(err, msg, TIBEFTL_FIELD_TYPE_DOUBLE_ARRAY, "doublearray", &len);
    if (tibeftlErr_IsSet(err))
    {
        printf("tibeftlMessage_GetArray:double failed\n");
        exit(EXIT_FAILURE);
    }
    if (len != 3)
    {
        printf("tibeftlMessage_GetArray:double failed: lengths do not match\n");
        exit(EXIT_FAILURE);
    }
    for (i = 0; i < 3; i++)
    {
        if (doublearrayvalue[i] != doublearray[i])
        {
            printf("tibeftlMessage_GetArray:double failed: values do not match\n");
            exit(EXIT_FAILURE);
        }
    }

    // add time field
    tibeftlMessage_SetTime(err, msg, "time", 100000);
    if (tibeftlErr_IsSet(err))
    {
        printf("tibeftlMessage_SetTime failed\n");
        exit(EXIT_FAILURE);
    }
    checkField(msg, "time", TIBEFTL_FIELD_TYPE_TIME, true);
    timevalue = tibeftlMessage_GetTime(err, msg, "time");
    if (tibeftlErr_IsSet(err))
    {
        printf("tibeftlMessage_GetTime failed\n");
        exit(EXIT_FAILURE);
    }
    if (timevalue != 100000)
    {
        printf("tibeftlMessage_GetTime failed: value does not match\n");
        exit(EXIT_FAILURE);
    }

    // add time array field
    tibeftlMessage_SetArray(err, msg, TIBEFTL_FIELD_TYPE_TIME_ARRAY, "timearray", timearray, 3);
    if (tibeftlErr_IsSet(err))
    {
        printf("tibeftlMessage_SetArray:time failed\n");
        exit(EXIT_FAILURE);
    }
    checkField(msg, "timearray", TIBEFTL_FIELD_TYPE_TIME_ARRAY, true);
    timearrayvalue = tibeftlMessage_GetArray(err, msg, TIBEFTL_FIELD_TYPE_TIME_ARRAY, "timearray", &len);
    if (tibeftlErr_IsSet(err))
    {
        printf("tibeftlMessage_GetArray:time failed\n");
        exit(EXIT_FAILURE);
    }
    if (len != 3)
    {
        printf("tibeftlMessage_GetArray:time failed: lengths do not match\n");
        exit(EXIT_FAILURE);
    }
    for (i = 0; i < 3; i++)
    {
        if (timearrayvalue[i] != timearray[i])
        {
            printf("tibeftlMessage_GetArray:time failed: values do not match\n");
            exit(EXIT_FAILURE);
        }
    }

    // add message field
    sub = tibeftlMessage_Create(err);
    if (sub)
    {
        tibeftlMessage_SetString(err, sub, "sub string", "this is a substring");
        tibeftlMessage_SetDouble(err, sub, "sub double", 0.2);
    }

    tibeftlMessage_SetMessage(err, msg, "message", sub);
    if (tibeftlErr_IsSet(err))
    {
        printf("tibeftlMessage_SetMessage failed\n");
        exit(EXIT_FAILURE);
    }
    checkField(msg, "message", TIBEFTL_FIELD_TYPE_MESSAGE, true);
    msgvalue = tibeftlMessage_GetMessage(err, msg, "message");
    if (tibeftlErr_IsSet(err))
    {
        printf("tibeftlMessage_GetMessage failed\n");
        exit(EXIT_FAILURE);
    }
    print(msgvalue);

    // add message array field
    messagearray[0] = tibeftlMessage_Create(err);
    tibeftlMessage_SetString(err, messagearray[0], "msg0", "test string");
    messagearray[1] = tibeftlMessage_Create(err);
    tibeftlMessage_SetDouble(err, messagearray[1], "msg1", 0.99);
    messagearray[2] = tibeftlMessage_Create(err);
    tibeftlMessage_SetLong(err, messagearray[2], "msg2", 99);

    tibeftlMessage_SetArray(err, msg, TIBEFTL_FIELD_TYPE_MESSAGE_ARRAY, "messagearray", messagearray, 3);
    if (tibeftlErr_IsSet(err))
    {
        printf("tibeftlMessage_SetArray:message failed\n");
        exit(EXIT_FAILURE);
    }
    checkField(msg, "messagearray", TIBEFTL_FIELD_TYPE_MESSAGE_ARRAY, true);
    messagearrayvalue = tibeftlMessage_GetArray(err, msg, TIBEFTL_FIELD_TYPE_MESSAGE_ARRAY, "messagearray", &len);
    if (tibeftlErr_IsSet(err))
    {
        printf("tibeftlMessage_GetArray:message failed\n");
        exit(EXIT_FAILURE);
    }
    if (len != 3)
    {
        printf("tibeftlMessage_GetArray:message failed: lengths do not match\n");
        exit(EXIT_FAILURE);
    }
    for (i = 0; i < 3; i++)
    {
        print(messagearrayvalue[i]);
    }
    tibeftlMessage_Destroy(err, messagearray[0]);
    tibeftlMessage_Destroy(err, messagearray[1]);
    tibeftlMessage_Destroy(err, messagearray[2]);

    // add opaque field
    memset(opaque, 'x', sizeof(opaque));

    tibeftlMessage_SetOpaque(err, msg, "opaque", opaque, sizeof(opaque));
    if (tibeftlErr_IsSet(err))
    {
        printf("tibeftlMessage_SetOpaque failed\n");
        exit(EXIT_FAILURE);
    }
    checkField(msg, "opaque", TIBEFTL_FIELD_TYPE_OPAQUE, true);
    opaquevalue = tibeftlMessage_GetOpaque(err, msg, "opaque", &size);
    if (tibeftlErr_IsSet(err))
    {
        printf("tibeftlMessage_GetOpaque failed\n");
        exit(EXIT_FAILURE);
    }
    if (sizeof(opaque) != size)
    {
        printf("tibeftlMessage_GetOpaque failed: length does not match: %d vs %d\n", (int)sizeof(opaque), len);
        exit(EXIT_FAILURE);
    }
    if (memcmp(opaque, opaquevalue, sizeof(opaque)) != 0)
    {
        printf("tibeftlMessage_GetOpaque failed: value does not match\n");
        exit(EXIT_FAILURE);
    }

    print(msg);

    // iterate
    while (tibeftlMessage_NextField(err, msg, &field, &type))
    {
        printf("field: %s, type: %d\n", field, type);
    }

    // remove string field
    tibeftlMessage_ClearField(err, msg, "string");
    if (tibeftlErr_IsSet(err))
    {
        printf("tibeftlMessage_ClearField failed\n");
        exit(EXIT_FAILURE);
    }
    checkField(msg, "string", TIBEFTL_FIELD_TYPE_UNKNOWN, false);

    print(msg);

    // remove all fields
    tibeftlMessage_ClearAllFields(err, msg);
    if (tibeftlErr_IsSet(err))
    {
        printf("tibeftlMessage_ClearAllFields failed\n");
        exit(EXIT_FAILURE);
    }

    print(msg);

    tibeftlMessage_Destroy(err, msg);
    tibeftlMessage_Destroy(err, sub);

    tibeftlErr_Destroy(err);

    printf("success\n");
    exit(EXIT_SUCCESS);
}
