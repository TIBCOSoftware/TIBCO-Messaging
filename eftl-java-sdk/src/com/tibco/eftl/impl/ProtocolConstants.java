/*
 * Copyright (c) 2001-$Date: 2020-09-24 12:20:18 -0700 (Thu, 24 Sep 2020) $ TIBCO Software Inc.
 * All rights reserved.
 * For more information, please contact:
 * TIBCO Software Inc., Palo Alto, California, USA
 *
 * $Id: ProtocolConstants.java 128796 2020-09-24 19:20:18Z bpeterse $
 *
 */
package com.tibco.eftl.impl;

public interface ProtocolConstants
{
    // WebSocket protocol
    public static String EFTL_WS_PROTOCOL = "v1.eftl.tibco.com";
 
    // eFTL protocol version
    public static int PROTOCOL_VERSION = 1;
   
    // Access Point protocol op codes
    public static int OP_HEARTBEAT =     0;
    public static int OP_LOGIN =         1;
    public static int OP_WELCOME =       2;
    public static int OP_SUBSCRIBE =     3;
    public static int OP_SUBSCRIBED =    4;
    public static int OP_UNSUBSCRIBE =   5;
    public static int OP_UNSUBSCRIBED =  6;
    public static int OP_EVENT =         7;
    public static int OP_MESSAGE =       8;
    public static int OP_ACK =           9;
    public static int OP_ERROR =         10;
    public static int OP_DISCONNECT =    11;
    public static int OP_REQUEST =       13;
    public static int OP_REQUEST_REPLY = 14;
    public static int OP_REPLY =         15;
    public static int OP_MAP_CREATE =    16;
    public static int OP_MAP_DESTROY =   18;
    public static int OP_MAP_SET =       20;
    public static int OP_MAP_GET =       22;
    public static int OP_MAP_REMOVE =    24;
    public static int OP_MAP_RESPONSE =  26;

    // Access Point protocol field name
    public static String OP_FIELD =             "op";
    public static String USER_FIELD =           "user";
    public static String PASSWORD_FIELD =       "password";
    public static String CLIENT_ID_FIELD =      "client_id";
    public static String CLIENT_TYPE_FIELD =    "client_type";
    public static String CLIENT_VERSION_FIELD = "client_version";
    public static String HEARTBEAT_FIELD =      "heartbeat";
    public static String TIMEOUT_FIELD =        "timeout";
    public static String MAX_SIZE_FIELD =       "max_size";
    public static String MATCHER_FIELD =        "matcher";
    public static String DURABLE_FIELD =        "durable";
    public static String ACK_FIELD =            "ack";
    public static String ERR_CODE_FIELD =       "err";
    public static String REASON_FIELD =         "reason";
    public static String ID_FIELD =             "id";
    public static String IDS_FIELD =            "ids";
    public static String MSG_FIELD =            "msg";
    public static String TO_FIELD =             "to";
    public static String REPLY_TO_FIELD =       "reply_to";
    public static String BODY_FIELD =           "body";
    public static String SEQ_NUM_FIELD =        "seq";
    public static String REQ_ID_FIELD =         "req";
    public static String STORE_MSG_ID_FIELD =   "sid";
    public static String DELIVERY_COUNT_FIELD = "cnt";
    public static String RESUME_FIELD =         "_resume";
    public static String LOGIN_OPTIONS_FIELD =  "login_options";
    public static String ID_TOKEN_FIELD =       "id_token";
    public static String QOS_FIELD =            "_qos";
    public static String MAP_FIELD =            "map";
    public static String KEY_FIELD =            "key";
    public static String DEL_FIELD =            "del";
    public static String VALUE_FIELD =          "value";
    public static String PROTOCOL =             "protocol";
    public static String MAX_PENDING_ACKS =     "max_pending_acks";
}
