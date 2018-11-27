/*
 * Copyright (c) 2001-$Date: 2018-09-04 17:57:51 -0500 (Tue, 04 Sep 2018) $ TIBCO Software Inc.
 * Licensed under a BSD-style license. Refer to [LICENSE]
 * For more information, please contact:
 * TIBCO Software Inc., Palo Alto, California, USA
 *
 * $Id: ProtocolConstants.cs 103512 2018-09-04 22:57:51Z bpeterse $
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
        
        // Access Point protocol field name
        internal static readonly String OP_FIELD =         "op";
        internal static readonly String USER_FIELD =       "user";
        internal static readonly String PASSWORD_FIELD =   "password";
        internal static readonly String CLIENT_ID_FIELD =  "client_id";
        internal static readonly String CLIENT_TYPE_FIELD =  "client_type";
        internal static readonly String CLIENT_VERSION_FIELD =  "client_version";
        internal static readonly String HEARTBEAT_FIELD =  "heartbeat";
        internal static readonly String TIMEOUT_FIELD =    "timeout";
        internal static readonly String MAX_SIZE_FIELD =   "max_size";
        internal static readonly String MATCHER_FIELD =    "matcher";
        internal static readonly String DURABLE_FIELD =    "durable";
        internal static readonly String ACK_FIELD =        "ack";
        internal static readonly String ERR_CODE_FIELD =   "err";
        internal static readonly String REASON_FIELD =     "reason";
        internal static readonly String ID_FIELD =         "id";
        internal static readonly String IDS_FIELD =        "ids";
        internal static readonly String MSG_FIELD =        "msg";
        internal static readonly String TO_FIELD =         "to";
        internal static readonly String BODY_FIELD =       "body";
        internal static readonly String SEQ_NUM_FIELD =    "seq";
        internal static readonly String RESUME_FIELD =     "_resume";
        internal static readonly String LOGIN_OPTIONS_FIELD = "login_options";
        internal static readonly String ID_TOKEN_FIELD =   "id_token";
        internal static readonly String SERVICE_FIELD =    "_service";
        internal static readonly String QOS_FIELD =        "_qos";
        internal static readonly String MAP_FIELD =        "map";
        internal static readonly String KEY_FIELD =        "key";
        internal static readonly String VALUE_FIELD =      "value";

        // Data loss service
        internal static readonly String DATALOSS_TYPE =    "data_loss";
        internal static readonly String COUNT_FIELD =      "count";
        
        // Presence service
        internal static readonly String IDENTITY_PRESENCE_TYPE = "identity_presence";
        internal static readonly String GROUP_PRESENCE_TYPE = "group_presence";
        internal static readonly String ACTION_FIELD =     "_action";
        internal static readonly String IDENTITY_FIELD =   "_identity";
        internal static readonly String GROUP_FIELD =      "_group";
        internal static readonly String REQUEST_FIELD =    "_request";
        internal static readonly String ACTION_JOIN =      "join";
        internal static readonly String ACTION_LEAVE =     "leave";
        internal static readonly String ACTION_TIMEOUT =   "timeout";
        internal static readonly String REQUEST_CURRENT =  "current";
    }
}
