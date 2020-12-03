/*
 * Copyright (c) 2001-$Date: 2020-09-24 12:20:18 -0700 (Thu, 24 Sep 2020) $ TIBCO Software Inc.
 * Licensed under a BSD-style license. Refer to [LICENSE]
 * For more information, please contact:
 * TIBCO Software Inc., Palo Alto, California, USA
 *
 * $Id: ProtocolConstants.cs 128796 2020-09-24 19:20:18Z bpeterse $
 *
 */
using System;
using System.Collections;
using System.Linq;
using System.Text;

namespace TIBCO.EFTL
{
    internal sealed class ProtocolConstants
    {
        // WebSocket protocol
        internal static readonly String EFTL_WS_PROTOCOL = "v1.eftl.tibco.com";
        
        // eFTL protocol version
        internal static readonly int PROTOCOL_VERSION = 1;
        
        // Access Point protocol field name
        internal static readonly String OP_FIELD =               "op";
        internal static readonly String USER_FIELD =             "user";
        internal static readonly String PASSWORD_FIELD =         "password";
        internal static readonly String CLIENT_ID_FIELD =        "client_id";
        internal static readonly String CLIENT_TYPE_FIELD =      "client_type";
        internal static readonly String CLIENT_VERSION_FIELD =   "client_version";
        internal static readonly String HEARTBEAT_FIELD =        "heartbeat";
        internal static readonly String TIMEOUT_FIELD =          "timeout";
        internal static readonly String MAX_SIZE_FIELD =         "max_size";
        internal static readonly String MATCHER_FIELD =          "matcher";
        internal static readonly String DURABLE_FIELD =          "durable";
        internal static readonly String ACK_FIELD =              "ack";
        internal static readonly String ERR_CODE_FIELD =         "err";
        internal static readonly String REASON_FIELD =           "reason";
        internal static readonly String ID_FIELD =               "id";
        internal static readonly String IDS_FIELD =              "ids";
        internal static readonly String MSG_FIELD =              "msg";
        internal static readonly String REPLY_TO_FIELD =         "reply_to";
        internal static readonly String TO_FIELD =               "to";
        internal static readonly String BODY_FIELD =             "body";
        internal static readonly String SEQ_NUM_FIELD =          "seq";
        internal static readonly String REQ_ID_FIELD =           "req";
        internal static readonly String STORE_MSG_ID_FIELD =     "sid";
        internal static readonly String DELIVERY_COUNT_FIELD =   "cnt";
        internal static readonly String RESUME_FIELD =           "_resume";
        internal static readonly String LOGIN_OPTIONS_FIELD =    "login_options";
        internal static readonly String ID_TOKEN_FIELD =         "id_token";
        internal static readonly String QOS_FIELD =              "_qos";
        internal static readonly String MAP_FIELD =              "map";
        internal static readonly String KEY_FIELD =              "key";
        internal static readonly String DEL_FIELD =              "del";
        internal static readonly String VALUE_FIELD =            "value";
        internal static readonly String PROTOCOL_FIELD =         "protocol";
        internal static readonly String MAX_PENDING_ACKS_FIELD = "max_pending_acks";
    }
}
