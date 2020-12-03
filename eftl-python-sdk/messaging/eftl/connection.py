'''
 @COPYRIGHT_BANNER@
'''

"""Documentation of the TIBCO eFTL™ Python application programming interface."""
import asyncio
import datetime
import json
import logging
import numbers
import random
import socket
import ssl
import threading
import time
import urllib
import base64
import sys
import math
import copy
from pathlib import Path

import concurrent.futures

from abc import ABC, abstractmethod
from autobahn.asyncio.websocket import WebSocketClientProtocol
from autobahn.asyncio.websocket import WebSocketClientFactory


import jsonpickle

_EFTL_VERSION = "@EFTL_VERSION_MAJOR@.@EFTL_VERSION_MINOR@.@EFTL_VERSION_UPDATE@ V@EFTL_VERSION_BUILD@"
EFTL_WS_PROTOCOL = "v1.eftl.tibco.com"

OP_HEARTBEAT        = 0
OP_LOGIN            = 1
OP_WELCOME          = 2
OP_SUBSCRIBE        = 3
OP_SUBSCRIBED       = 4
OP_UNSUBSCRIBE      = 5
OP_UNSUBSCRIBED     = 6
OP_EVENT            = 7
OP_MESSAGE          = 8
OP_ACK              = 9
OP_ERROR            = 10
OP_DISCONNECT       = 11
OP_REQUEST          = 13
OP_REQUEST_REPLY    = 14
OP_REPLY            = 15
OP_MAP_CREATE       = 16
OP_MAP_DESTROY      = 18
OP_MAP_SET          = 20
OP_MAP_GET          = 22
OP_MAP_REMOVE       = 24
OP_MAP_RESPONSE     = 26
ERR_PUBLISH_FAILED  = 11

WS_NORMAL_CLOSE     = 1000
WS_ABNORMAL_CLOSE   = 1006
RESTART             = 1012

# subscription related constants
SUBSCRIPTIONS_DISALLOWED = 13
SUBSCRIPTION_FAILED = 21
SUBSCRIPTION_INVALID = 22

# request/reply related constants
REQUEST_FAILED = 41
REQUEST_DISALLOWED = 40
REQUEST_TIMEOUT = 99

#PROPERTY_TIMEOUT                    = "timeout"
#PROPERTY_NOTIFICATION_TOKEN         = "notification_token"

ACKNOWLEDGE_MODE_CLIENT             = "client"
ACKNOWLEDGE_MODE_NONE               = "none"
ACKNOWLEDGE_MODE_AUTO               = "auto"
ACKNOWLEDGE_MODE                    = "ack_mode"
ACK_FIELD                           = "ack"

PROPERTY_USERNAME                   = "username"
PROPERTY_PASSWORD                   = "password"
PROPERTY_CLIENT_ID                  = "client_id"
PROPERTY_AUTO_RECONNECT_ATTEMPTS    = "auto_reconnect_attempts"
PROPERTY_AUTO_RECONNECT_MAX_DELAY   = "auto_reconnect_max_delay"
PROPERTY_HANDSHAKE_TIMEOUT          = "handshake_timeout"
PROPERTY_POLLING_INTERVAL           = "polling_interval"
PROPERTY_LOGIN_TIMEOUT              = "login_timeout"
PROPERTY_DURABLE_TYPE               = "type"
PROPERTY_DURABLE_KEY                = "key"
PROPERTY_MAX_PENDING_ACKS           = "max_pending_acks"
DURABLE_TYPE_STANDARD               = "standard"
DURABLE_TYPE_SHARED                 = "shared"
DURABLE_TYPE_LAST_VALUE             = "last-value"
MATCHER_FIELD                       = "matcher"
DURABLE_FIELD                       = "durable"

OP_FIELD                = "op"
USER_FIELD              = "user"
PASSWORD_FIELD          = "password"
PROTOCOL_FIELD          = "protocol"
CLIENT_ID_FIELD         = "client_id"
CLIENT_TYPE_FIELD       = "client_type"
CLIENT_VERSION_FIELD    = "client_version"
ID_TOKEN_FIELD          = "id_token"
TIMEOUT_FIELD           = "timeout"
HEARTBEAT_FIELD         = "heartbeat"
MAX_SIZE_FIELD          = "max_size"
STORE_MSG_ID_FIELD      = "sid"
DELIVERY_COUNT_FIELD    = "cnt"
ERR_CODE_FIELD          = "err"
REASON_FIELD            = "reason"
ID_FIELD                = "id"
TO_FIELD                = "to"
BODY_FIELD              = "body"
SEQ_NUM_FIELD           = "seq"
LOGIN_OPTIONS_FIELD     = "login_options"
RESUME_FIELD            = "_resume"
QOS_FIELD               = "_qos"
REPLY_TO_FIELD          = "reply_to"
REQ_ID_FIELD            = "req"
DEL_FIELD               = "del"
MAX_PENDING_ACKS_FIELD  = "max_pending_acks"

MAP_FIELD               = "map"
KEY_FIELD               = "key"
VALUE_FIELD             = "value"
DOUBLE_FIELD            = "_d_"
MILLISECOND_FIELD       = "_m_"
OPAQUE_FIELD            = "_o_"

SUBSCRIPTION_TYPE       = ".s."

SECOND                  = 1000 #milliseconds

logger = logging.getLogger()
handler = logging.StreamHandler(sys.stdout)
formatter = logging.Formatter(
        '%(asctime)s %(name)-12s %(levelname)-8s %(message)s')
handler.setFormatter(formatter)
logger.addHandler(handler)
logger.setLevel(logging.WARN)

class EftlClientError(Exception):
    """Raise when establishing a connection to the server fails."""

class EftlMessageSizeTooLarge(Exception):
    """Raise when EftlMesssage size is larger than the maximum allowed size."""

class EftlAlreadyConnected(Exception):
    """Raise when connect method is called but connection is already connected."""

def _call_dict_function(dic, name, **kwargs):
    """Call dic[name](**kwargs), if it exists."""
    try:
        logger.debug("name %s", name)
        func = dic.get(name)
        if func is None:
            logger.debug("User did not give a callback for this event. kwargs:\n{}".format(kwargs))
        else:
            evnt_loop = asyncio.get_event_loop()
            evnt_loop.create_task(func(**kwargs))
    except Exception as e:
        logger.error(e)
        raise


def _safe_extend(target, source):
    """Add non-None, non-callable values from source to target."""
    try:
        for k, v in source.items():
            if v is None or callable(v):
                continue
            if isinstance(v, bool):
                v = str(v)
            target[k] = v
    except Exception as e:
        logger.error(e)
        raise
    return target

class Eftl():
    """Programs use Eftl Class to connect to an eFTL server."""

    def __init__(self):
        """Initialize the Eftl instance."""
        self.ec = None

    CONNECTING = 0
    CONNECTED = 1
    RECONNECTING = 2
    DISCONNECTING = 3
    DISCONNECTED = 4


    def get_version(self):
        """
        Return the eFTL Python library version.

        Returns:
        str: The eFTL Python library version
        """
        return "TIBCO eFTL Client Version " + _EFTL_VERSION

    async def connect(self, url, **kwargs):
        """
        Connect to an eFTL server.

        This call returns immediately; connecting continues asynchronously.
        When the connection is ready to use, the eFTL library calls your
        on_connect method, passing a EftlConnection object that you can
        use to publish and subscribe.

        When a pipe-separated list of URLs is specified this call will attempt
        a connection to each in turn, in a random order, until one is connected.
        A program that uses more than one server channel must connect
        separately to each channel.

        :param:
        url:
            The call connects to the eFTL server at this URL. This can be
            a single URL, or a pipe ('|') separated list of URLs. URLs can
            be in either of these forms

                ws://host:port/channel
                wss://host:port/channel

            Optionally, the URLs can contain the username, password,
            and/or client identifier

                ws://username:password@host:port/channel?clientId=<identifier>
                wss://username:password@host:port/channel?clientId=<identifier

        :param:
        kwargs
            auto_reconnect_attempts : int, optional
                Maximum number of reconnect attempts. The default is 
                256 attempts.
           auto_reconnect_max_delay : optional
                Maximum reconnect delay in milliseconds. The default is
                30 seconds.
           user : optional
                Login credentials to use if not found in the url.
           password : optional
                Login credentials to use if not found in the url.
           client_id : optional
                User-specified client identifier.
           handshake_timeout : optional
                Seconds to wait for websocket handshake to complete.
           login_timeout : optional
                Seconds to halt waiting for a login message reply. If a
                reply is not received in time, raise an EftlClientError.
           polling_interval : optional
                Seconds to wait between each message reply check.
           trust_all : true/false (optional)
           trust_store :  certificate path (optional)
           loop : event loop provided by user (optional)
           Callbacks :
                on_connect(connection):
                on_disconnect(connection, loop, code, reason):
                on_reconnect(connection):
                on_error(connection, code, reason):
                on_state_change(connection, state):
        :return:
            The EftlConnection object that can used to publish and subscribe
        Raises
        ------
        ValueError
            If any provided urls lack a host name or the correct scheme.
        EftlClientError
            If the connection is not established in time.
        """
        if self.ec is not None:
            if self.ec.is_connected():
                return self.ec
        else:
            self.ec = EftlConnection()

        await self.ec._connect(url, **kwargs)
        return self.ec

class _EFTLJSONEncoder(json.JSONEncoder):
    def default(self, o):
        if isinstance(o, EftlMessage):
            return o._EftlMessage__msg_obj
        return json.JSONEncoder.default(self, o)

class EftlMessage():
    """
    Programs use message objects to publish messages on an eFTL connection or receive messages for a given subscription.

    Message objects contain typed fields that map names to values.

    """

    def __init__(self, obj=None):
        """Initialize the EftlMessage instance properties."""
        self.__sequence_numbr_ = None
        self.__subscriber_identifier_ = None
        self.__sid_ = None
        self.__cnt_ = None
        self.__request_id_ = None
        self.__reply_to_ = None

        if obj is not None:
            self.__msg_obj = obj
        else:
            self.__msg_obj = {}


    ######################## Overridden Class methods ####################################

    def __str__(self):
        """Representation of EftlMessage Instance in str."""
        result = ""
        result += "{"
        index = -1
        for key, value in self.__msg_obj.items():
            index = index + 1
            result += "'" + key + "' : "
            if not isinstance(value, list):
                if isinstance(value, str):
                    result += self.get_string(key)
                elif isinstance(value, int):
                    result += str(self.get_long(key))
                elif isinstance(value, dict):
                    if DOUBLE_FIELD in value:
                        double_value = self.get_double(key)
                        if double_value is not None:
                            result += str(double_value)
                        else:
                            result += "None"
                    elif MILLISECOND_FIELD in value:
                        datetime_value = self.get_datetime(key)
                        if datetime_value is not None:
                            result += str(datetime_value)
                        else:
                            result += "None"
                    elif OPAQUE_FIELD in value:
                        opaque_value = self.get_opaque(key)
                        if opaque_value is not None:
                            result += str(opaque_value)
                        else:
                            result += "None"
                    else:
                        message_value = self.get_message(key)
                        if message_value is not None:
                            result += str(message_value)
                        else:
                            result += "None"
            else:
                if len(value) > 0:
                    firstValue = value[0]
                    if isinstance(firstValue, str):
                        result += str(self.get_string_array(key))
                    elif isinstance(firstValue, int):
                        result += str(self.get_long_array(key))
                    elif isinstance(firstValue, dict):
                        if DOUBLE_FIELD in firstValue:
                            double_value_arr = self.get_double_array(key)
                            if double_value_arr is not None:
                                result += str(double_value_arr)
                            else:
                                result += "[]"
                        elif MILLISECOND_FIELD in firstValue:
                            datetime_value_arr = self.get_datetime_array(key)
                            if datetime_value_arr is not None:
                                result += str(datetime_value_arr)
                            else:
                                 result += "[]"
                        elif OPAQUE_FIELD in firstValue:
                            result += "invalid array of opaque type"
                        else:
                            message_value_arr = self.get_message_array(key)
                            if message_value_arr is not None:
                                result += str(message_value_arr)
                            else:
                                result += "[]"
                else:
                    result += "[]"
            if index != len(self.__msg_obj.items())-1:
                result += ", "
        result += "}"
        return result
        

    ######################## Protected Setter and Getter methods ####################################

    def _set_sequence_number_(self, seq_num):
        self.__sequence_numbr_ = seq_num

    def _get_sequence_number_(self):
        return self.__sequence_numbr_

    def _set_subscriber_id_(self, sub_id):
        self.__subscriber_identifier_ = sub_id

    def  _get_subscriber_id_(self):
        return self.__subscriber_identifier_

    def _set_sid_(self, sid):
        self.__sid_ = sid

    def _set_cnt_(self, cnt):
        self.__cnt_ = cnt

    def _set_request_id_(self, request_id):
        self.__request_id_ = request_id

    def  _get_request_id_(self):
        return self.__request_id_

    def _set_reply_to_(self, reply_to):
        self.__reply_to_ = reply_to

    def  _get_reply_to_(self):
        return self.__reply_to_

    # public APIs
    def set_string(self, field_name, value):
        """
        Set a string field in a message.

        :param field_name: The call sets this field
        :param value: The call sets this value, to remove
                      the field, supply None.
        :return:
        """
        if isinstance(field_name, str):
            if isinstance(value, str):
                self._set_field(field_name, value)
            else:
                raise TypeError("Expected to have 'value' of type 'str'")
        else:
            raise TypeError("Expected to have 'field_name' of type 'str'")

    def set_string_array(self, field_name, values):
        """
        Set a string array field in a message.

        :param field_name: The call sets this field
        :param values: The call sets this value, to remove
                      the field, supply None.
        :return:
        """
        if isinstance(field_name, str):
            if isinstance(values, list):
                if len(values) > 0:
                    for value in values:
                        if not isinstance(value, str):
                            raise TypeError("Expected to have 'list' of elements of type 'str'")
                self._set_field(field_name, values)
            else:
                raise TypeError("Expected to have 'values' of type 'list'")
        else:
            raise TypeError("Expected to have 'field_name' of type 'str'")


    def set_long(self, field_name, value):
        """
        Set a long field in a message.

        :param field_name: The call sets this field
        :param value: The call sets this value, to remove
                      the field, supply None.
        :return:
        """
        if isinstance(field_name, str):
            if isinstance(value, int):
                self._set_field(field_name, value)
            else:
                raise TypeError("Expected to have 'value' of type 'int'")
        else:
            raise TypeError("Expected to have 'field_name' of type 'str'")

    def set_long_array(self, field_name, values):
        """
        Set a long array field in a message.

        :param field_name: The call sets this field
        :param value: The call sets this value, to remove
                      the field, supply None.
        :return:
        """
        if isinstance(field_name, str):
            if isinstance(values, list):
                if len(values) > 0:
                    for value in values:
                        if not isinstance(value, int):
                            raise TypeError("Expected to have 'list' of elements of type 'int'")
                self._set_field(field_name, values)
            else:
                raise TypeError("Expected to have 'values' of type 'list'")
        else:
            raise TypeError("Expected to have 'field_name' of type 'str'")

    def set_double(self, field_name, value):
        """
        Set a double field in a message.

        :param field_name: The call sets this field
        :param value: The call sets this value, to remove
                      the field, supply None.
        :return:
        """
        if isinstance(field_name, str):
            if isinstance(value, float):
                if math.isnan(value):
                    dvalue = "NaN"
                    encoded_value = {DOUBLE_FIELD : dvalue}
                elif value == float('inf'):
                    dvalue = "Infinity"
                    encoded_value = {DOUBLE_FIELD : dvalue}
                elif value == float('-inf'):
                    dvalue = "-Infinity"
                    encoded_value = {DOUBLE_FIELD : dvalue}
                else:
                    encoded_value = {DOUBLE_FIELD : value}

                self._set_field(field_name, encoded_value)
            else:
                raise TypeError("Expected to have 'value' of type 'float'")
        else:
            raise TypeError("Expected to have 'field_name' of type 'str'")

    def set_double_array(self, field_name, values):
        """
        Set a double array field in a message.

        :param field_name: The call sets this field
        :param value: The call sets this value, to remove
                      the field, supply None.
        :return:
        """
        if isinstance(field_name, str):
            if isinstance(values, list):
                if len(values) > 0:
                    encode_values_array = []
                    for value in values:
                        if isinstance(value, float):
                            encode_values_array.append({DOUBLE_FIELD : value})
                        else:
                            raise TypeError("Expected to have 'list' of elements of type 'float'")
                    self._set_field(field_name, encode_values_array)
                else:
                    self._set_field(field_name, values)
            else:
                raise TypeError("Expected to have 'values' of type 'list'")
        else:
            raise TypeError("Expected to have 'field_name' of type 'str'")

    def set_datetime(self, field_name, value):
        """
        Set a datetime field in a message.

        :param field_name: The call sets this field
        :param value: The call sets this value, to remove
                      the field, supply None.
        :return:
        """
        if isinstance(field_name, str):
            if isinstance(value, datetime.datetime):
                encoded_value = { MILLISECOND_FIELD : int(value.timestamp() * 1000) }
                self._set_field(field_name, encoded_value)
            else:
                raise TypeError("Expected to have 'value' of type 'datetime.datetime'")
        else:
            raise TypeError("Expected to have 'field_name' of type 'str'")

    def set_datetime_array(self, field_name, values):
        """
        Set a datetime array field in a message.

        :param field_name: The call sets this field
        :param value: The call sets this value, to remove
                      the field, supply None.
        :return:
        """
        if isinstance(field_name, str):
            if isinstance(values, list):
                if len(values) > 0:
                    encode_values_array = []
                    for value in values:
                        if isinstance(value, datetime.datetime):
                            encode_values_array.append({ MILLISECOND_FIELD : int(value.timestamp() * 1000) })
                        else:
                            raise TypeError("Expected to have 'list' of elements of type 'datetime.datetime'")
                    self._set_field(field_name, encode_values_array)
                else:
                    self._set_field(field_name, values)
            else:
                raise TypeError("Expected to have 'values' of type 'list'")
        else:
            raise TypeError("Expected to have 'field_name' of type 'str'")

    def set_message(self, field_name, value):
        """
        Set a message field in a message.

        :param field_name: The call sets this field
        :param value: The call sets this value, to remove
                      the field, supply None.
        :return:
        """
        if isinstance(field_name, str):
            if isinstance(value, EftlMessage):
                self._set_field(field_name, value)
            else:
                raise TypeError("Expected to have 'value' of type 'EftlMessage'")
        else:
            raise TypeError("Expected to have 'field_name' of type 'str'")

    def set_message_array(self, field_name, values):
        """
        Set a message array field in a message.

        :param field_name: The call sets this field
        :param value: The call sets this value, to remove
                      the field, supply None.
        :return:
        """
        if isinstance(field_name, str):
            if isinstance(values, list):
                if len(values) > 0:
                    for value in values:
                        if not isinstance(value, EftlMessage):
                             raise TypeError("Expected to have 'list' of elements of type 'EftlMessage'")
                self._set_field(field_name, values)
            else:
                raise TypeError("Expected to have 'values' of type 'list'")
        else:
            raise TypeError("Expected to have 'field_name' of type 'str'")

    def set_opaque(self, field_name, value):
        """
        Set an opaque field in a message.

        :param field_name: The call sets this field
        :param value: The call sets this value, to remove
                      the field, supply None.
        :return:
        """
        if isinstance(field_name, str):
            if value is not None:
                encoded_data = base64.b64encode(value).decode("ascii")
                encoded_value_str = { OPAQUE_FIELD : encoded_data}
                self._set_field(field_name, encoded_value_str)
        else:
            raise TypeError("Expected to have 'field_name' of type 'str'")


    ########################## Protected Helper Setter methods for EftlMessage ##############################

    def _set_field(self, field_name, value):
        if isinstance(field_name, str):
            self.__msg_obj[field_name] = value
        else:
            raise TypeError("Expected to have 'field_name' of type 'str'")

    def _remove_field(self, field_name):
        if isinstance(field_name, str):
            if field_name in self.__msg_obj:
                del self.__msg_obj[field_name]
            else:
                raise LookupError("field not found")
        else:
            raise TypeError("field name should be of type 'str'")

    ########################## Protected Helper Getter methods for EftlMessage ##############################

    def _check_type(self, value):
        if isinstance(value, str):
            return "str"
        elif isinstance(value, int):
            return "int"
        elif isinstance(value, dict):
            if DOUBLE_FIELD in value:
                return "float"
            elif MILLISECOND_FIELD in value:
                return "datetime.datetime"
            elif OPAQUE_FIELD in value:
                return "opaque"
            else:
                return "message"
        else:
            return "unknown"

    def _get_field_type(self, field_name):
        if isinstance(field_name, str):
            whole_value = self.__msg_obj.get(field_name)
            if whole_value is not None:
                if isinstance(whole_value, list):
                    if len(whole_value) >0:
                        value = whole_value[0]
                        return self._check_type(value)
                    else:
                        return "unknown"
                else:
                    return self._check_type(whole_value)
            else:
                raise LookupError("field not found")
        else:
            raise TypeError("field name should be of type 'str'")

    def _get_field(self, field_name, field_type):
        if isinstance(field_name, str):
            value = self.__msg_obj.get(field_name)
            if value is not None:
                if field_type == "string_type":
                    if isinstance(value, str):
                        return value
                    else:
                        raise TypeError("Expected to have value of type 'str', got " + type(value))
                elif field_type == "long_type":
                    if isinstance(value, int):
                        return value
                    else:
                        raise TypeError("Expected to have value of type 'int', got " + type(value))
                elif field_type == "double_type":
                    valid_value = value[DOUBLE_FIELD]
                    if valid_value is not None:
                        if valid_value == "Infinity":
                            valid_value = float("inf")
                        elif valid_value == "-Infinity":
                            valid_value = float("-inf")
                        elif valid_value == "NaN":
                            valid_value = float("nan")
                        else:
                            if not isinstance(valid_value,float):
                                raise TypeError("Expected to have value of type 'float', got " + type(value))
                    return valid_value
                elif field_type == "datetime_type":
                    valid_value = value[MILLISECOND_FIELD]
                    if valid_value is not None:
                        if isinstance(valid_value,int):
                            timestamp = valid_value / 1000
                            return datetime.datetime.fromtimestamp(timestamp)
                        else:
                            raise TypeError("Expected to have value of type datetime.datetime")
                    else:
                        return valid_value
                elif field_type == "opaque_type":
                    valid_value = value[OPAQUE_FIELD]
                    if valid_value is not None:
                        base64_value_bytes = valid_value.encode('utf-8')
                        return base64.decodebytes(base64_value_bytes)
                    else:
                        return valid_value
                elif field_type == "message_type":
                    if value is not None:
                        if isinstance(value, dict):
                            new_msg = EftlMessage(value)
                            return new_msg
                        else:
                            raise TypeError("Expected to have value of type EftlMessage")
                    else:
                        return value
                else:
                    raise TypeError("Not a valid Eftl field type")
            else:
                raise LookupError("field not found")
        else:
            raise TypeError("field name should be of type 'str'")

    def _get_array(self, field_name, field_type):
        if isinstance(field_name, str):
            values = self.__msg_obj.get(field_name)
            if values is not None:
                if isinstance(values, list):
                    if len(values) > 0:
                       if field_type == 'string_type':
                           return self._to_str_array(values)
                       elif field_type == 'long_type':
                           return self._long_to_int_array(values)
                       elif field_type == 'double_type':
                           return self._double_to_float_array(values)
                       elif field_type == 'datetime_type':
                           return self._timestamp_to_datetime_array(values)
                       elif field_type == 'message_type':
                           return self._obj_to_eftlmessage_array(values)
                    else:
                        return []
                else:
                    raise TypeError("Expected the value of type 'list'")
            else:
                raise LookupError("field not found")
        else:
            raise TypeError("field name should be of type 'str'")

    def _to_str_array(self, values):
        for value in values:
            if not isinstance(value, str):
                raise TypeError("'list' containing elements of type 'str' was expected.")
        return values

    def _long_to_int_array(self, values):
        for value in values:
            if not isinstance(value, int):
                raise TypeError("'list' containing elements of type 'int' was expected.")
        return values

    def _double_to_float_array(self, values):
        return_array = []
        for value in values:
            valid_value = value[DOUBLE_FIELD]
            if valid_value is not None:
                if not isinstance(valid_value,float):
                    raise TypeError("'list' containing elements of type 'float' was expected.")
                return_array.append(valid_value)
        return return_array

    def _timestamp_to_datetime_array(self, values):
        return_array = []
        for value in values:
            valid_value = value[MILLISECOND_FIELD]
            if valid_value is not None:
                if not isinstance(valid_value,int):
                    raise TypeError("'list' containing elements of type 'datetime.datetime' was expected.")
                timestamp = valid_value / 1000
                return_array.append(datetime.datetime.fromtimestamp(timestamp))
        return return_array

    def _obj_to_eftlmessage_array(self, values):
        return_array = []
        for value in values:
            if not isinstance(value, dict):
                raise TypeError("'list' containing elements of type 'EftlMessage' was expected.")
            new_msg = EftlMessage(value)
            return_array.append(new_msg)
        return return_array

    # public APIs getters methods for EFTLMessage

    def get_store_message_id(self):
        """
        Message's unique store identifier assigned by the persistence service.

        :return:
              A monotonically increasing long value that represents
              message's unique store identifier
        """
        return self.__sid_

    def get_delivery_count(self):
        """
        Message's delivery count assigned by the persistence service.

        :return:
              The message delivery count.
        """
        return self.__cnt_

    def get_string(self, field_name):
        """
        Get the value of a string field from a message.

        :param field_name: The name of the field

        :return: The value of the field if the field is present
                 and has type string
        """
        return self._get_field(field_name, "string_type")

    def get_string_array(self, field_name):
        """
        Get the value of a string array field from a message.

        :param field_name: The name of the field

        :return: The value of the field if the field is present
                 and has type string array
        """
        return self._get_array(field_name, "string_type")

    def get_long(self, field_name):
        """
        Get the value of a long field from a message.

        :param field_name: The name of the field

        :return: The value of the field if the field is present
                 and has type long
        """
        return self._get_field(field_name, "long_type")

    def get_long_array(self, field_name):
        """
        Get the value of a long array field from a message.
        
        :param field_name: The name of the field

        :return: The value of the field if the field is present
                 and has type long array
        """
        return self._get_array(field_name, "long_type")

    def get_double(self, field_name):
        """
        Get the value of a long field from a message.

        :param field_name: The name of the field

        :return: The value of the field if the field is present
                 and has type double
        """
        return self._get_field(field_name, "double_type")

    def get_double_array(self, field_name):
        """
        Get the value of a long field from a message.

        :param field_name: The name of the field

        :return: The value of the field if the field is present
                 and has type double array
        """
        return self._get_array(field_name, "double_type")

    def get_datetime(self, field_name):
        """
        Get the value of a long field from a message.

        :param field_name: The name of the field

        :return: The value of the field if the field is present
                 and has type datetime
        """
        return self._get_field(field_name, "datetime_type")

    def get_datetime_array(self, field_name):
        """
        Get the value of a long field from a message.

        :param field_name: The name of the field

        :return: The value of the field if the field is present
                 and has type datetime array
        """
        return self._get_array(field_name, "datetime_type")

    def get_message(self, field_name):
        """
        Get the value of a long field from a message.

        :param field_name: The name of the field

        :return: The value of the field if the field is present
                 and has type message
        """
        return self._get_field(field_name, "message_type")

    def get_message_array(self, field_name):
        """
        Get the value of a message array field from a message.

        :param field_name: The name of the field

        :return: The value of the field if the field is present
                 and has type message array (list of messages)
        """
        return self._get_array(field_name, "message_type")

    def get_opaque(self, field_name):
        """
        Get the value of opaque field from a message.
        
        :param field_name: The name of the field
        :return: The value of the field if the field is present
        """
        return self._get_field(field_name, "opaque_type")

    def get_field_names(self):
        """
        Return the list of field names.
        
        :return: The field names of this message as a list object.
        """
        return self.__msg_obj.keys()

    def is_field_set(self, field_name):
        """
        Return True if the field is set, False otherwise.

        :param field_name: The name of the field
        :return: True if the field is set, False otherwise

        """
        field_present = self.__msg_obj.get(field_name)
        if field_present is not None:
            return True
        else:
            return False

    def get_field_type(self, field_name):
        """
        Return the type of value of this field.
        
        :param field_name: The name of the field
        :return: type of value of this field.
        
            Possible return values : 
	            'int' represents Integer
	            'str' represents String
	            'float' represents Float
	            'datetime.datetime' represents datetime.datetime 
	            'message' represents EftlMessage object type
	            'opaque' represents opaque type (bytes)
	            'unknown'
        """
        return self._get_field_type(field_name)

    ########################## Other Public methods for EftlMessage ##############################

    def clear_field(self, field_name):
        """
        Remove the given field from this message.
        
        :param field_name: 
        """
        self._remove_field(field_name)

    def clear_all_fields(self):
        """Remove all the fields in this message."""
        self.__msg_obj.clear()

class EftlKVMap():
    """
    A key-value map  object represents a program's connection to an FTL map.

    Programs use key-value map objects to set, get, and remove key-value
    pairs in an FTL map.

    """

    def __init__(self, connection, name):
        self.connection = connection
        self.name = name

    async def get(self, key, **kwargs):
        """
        Get the value of a key from the map, or <c>null</c> if the key is not set.
        
        :param key: Get the value for this key
        :param kwargs:
            Callbacks:
                on_success:
                on_error:
        :return: The value as a EftlMessage
        """
        if not self.connection._permanently_closed():
            if isinstance(key, str):
                self._create_kv_request(OP_MAP_GET, key, None, **kwargs)
            else:
                raise TypeError("key should be of type 'str'")
        else:
            raise ConnectionError("Connection is closed.")

    async def set(self, key, value, **kwargs):
        """
        Set a key-value pair in the map, overwriting any existing value.

        This call returns immediately; setting continues
        asynchronously.  When the set completes successfully,
        the eFTL library calls your on_success callback.

        :param key: Set the value for this key.
        :param value: Set this value for the key.
        :param kwargs:
              on_success
              on_error
        :return:
        """
        if not self.connection._permanently_closed():
            if isinstance(key, str):
                if isinstance(value, EftlMessage):
                    self._create_kv_request(OP_MAP_SET, key, value, **kwargs)
                else:
                    raise TypeError("Only EftlMessage is allowed as value.")
            else:
                raise TypeError("key should be of type 'str'")
        else:
            raise ConnectionError("Connection is closed.")

    async def remove(self, key, **kwargs):
        """
        Remove a key-value pair from the map.

        This call returns immediately; removing continues
        asynchronously.  When the remove completes successfully,
        the eFTL library calls your on_success callback.

        :param key:  Revove the value for this key.
        :param kwargs:
               on_success
               on_error
        :return:
        """
        if not self.connection._permanently_closed():
            if isinstance(key, str):
                self._create_kv_request(OP_MAP_REMOVE, key, **kwargs)
            else:
                raise TypeError("key should be of type 'str'")
        else:
            raise ConnectionError("Connection is closed.")

    def _create_kv_request(self, op, key, message=None, **kwargs):
        conn = self.connection._ws

        envelope = {
            OP_FIELD: op,
            MAP_FIELD: self.name,
            KEY_FIELD: key,
        }

        if message is not None:
            m = json.dumps(message, cls=_EFTLJSONEncoder)
            encodedMsg = json.loads(m)
            envelope[VALUE_FIELD] = encodedMsg

        sequence = conn._next_sequence()
        envelope[SEQ_NUM_FIELD] = sequence

        request_options = {}
        success = kwargs.get("on_success")
        if success is not None:
            request_options["on_success"] = success
        error = kwargs.get("on_error")
        if error is not None:
            request_options["on_error"] = error

        request = {
            "envelope": envelope,
            "sequence": sequence,
            "options": request_options
        }
        if message:
            request["message"] = message

        verify_size = (op == OP_MAP_SET)
        conn._send_request_on_conn(request, sequence, verify_size)

class EftlConnection():
    """
    A websocket wrapper exchanging eftl protocol messages with a server.

    Different functions implement each of the types of protocol messages
    (e.g. publish, subscribe, disconnect). The messages are sent
    asynchronously, but by default these functions halt waiting for a
    reponse. The user may set a timeout for this waiting period if they
    desire.

    Users also may specify callbacks to trigger when messages get
    delivered, fail to get delivered, etc.
    """

    def __init__(self):
        """Initialize the connection object, which is unconnected."""
        self._ws = None
        self.client_protocol = WebSocketClientProtocol
        self.client_factory = WebSocketClientFactory
        self.kv_pair = None
        self.trust_all = False
        self.trust_store = None
        self.client_id = None
        #self.is_reconnecting = False
        self.previously_connected = False
        self.reconnect_token = None
        self.sequence_counter = 0

        self.first_retry_delay = 0.0
        self.connection_closed = False

        self.status = Eftl.DISCONNECTED
        self.subscriptions = {}
        self.subscription_counter = 0
        self.reconnect_counter = 0

        # user configurable settings
        self.auto_reconnect_attempts = 256
        self.auto_reconnect_max_delay = 30

        self.requests = {}
        self.got_login_reply = False
        self.reconnect_timer = None

        self.attempt_manual_reconnect = False

        self.url_index = 0
        self.url_list = None

        self.user_event_loop = asyncio.get_event_loop()
        self.timeout = 15
        self.polling_interval = 0.2
        self.trust_all = False
        self.trust_store = None
        self.login_timeout = 15

        self.code = None
        self.reason = None

    def _set_status(self, status):
        if self.status != status:
            self.status = status
            _call_dict_function(
                self.factory.conn.connect_options,
                "on_state_change",
                connection=self, state=self.status
            )

    def get_clientId(self):
        """
        Get the client identifier for this connection.

        :return: The client's identifier.

        """
        return self.client_id

    async def _async_connect(self, url, polling_interval):
        if url.scheme == "wss":
            logger.debug("url scheme is wss, creating a secure connection to eFTLserver")
            if url.port == None:
                port = 443
            else:
                port = url.port
            sock = socket.create_connection((url.hostname, port))
            sslcontext = ssl.SSLContext(ssl.PROTOCOL_TLS_CLIENT)

            if self.trust_all :
                sslcontext.check_hostname = False
                sslcontext.verify_mode = ssl.CERT_NONE
            else:
                # For ssl.PROTOCOL_TLS_CLIENT context,
                # by default, verify_mode is set to CERT_REQUIRED and check_hostname is set to True

                if self.trust_store:
                    sslcontext.load_verify_locations(cafile=self.trust_store)
                else:
                    sslcontext.load_default_certs(purpose=ssl.Purpose.SERVER_AUTH)

            await self.user_event_loop.create_connection(self.factory, sock=sock, ssl=sslcontext, server_hostname=url.hostname)
        else:
            logger.debug("url scheme is ws, creating a non secure connection to eFTLserver")
            if url.port == None:
                port = 80
            else:
                port = url.port
            await self.user_event_loop.create_connection(self.factory, url.hostname, port)

        while self._ws is not None and not self.got_login_reply and not self.connection_closed:
            await asyncio.sleep(polling_interval)

        return self._ws is not None and self.got_login_reply and not self.connection_closed

    async def _do_connect(self, url, timeout, polling_interval):
        self.connection_closed = False

        try:
            res = await self._async_connect(url, polling_interval)
            return res

        except Exception as e:
            return False

    def _do_reconnect(self, url, timeout, polling_interval):
        self.connection_closed = False

        task = self.user_event_loop.create_task(self._async_connect(url, polling_interval))
        task.add_done_callback(self._after_reconnect_attempt)

    def _after_reconnect_attempt(self,task):
        try:
            res = task.result()
            if res:
                self.code = None
                self.reason = None
                self._set_status(Eftl.CONNECTED)
                self.got_login_reply = False
                self.connection_closed = False
                self.reconnect_counter = 0
                return
            else:
                return
        except:
                logger.debug("Reconnnect task raised an exception ..")

        will_reconnect = self._schedule_reconnect()
        if not will_reconnect:
            # execute on_disconnect
            # clean up stuff
            self._set_status(Eftl.DISCONNECTED)
            if self._ws is not None:
                self._ws._clean_up(self.code, self.reason)

    async def reconnect(self):
        """
        Reopen a closed connection.

        You may call this method within your on_disconnect method.

        This call returns immediately; connecting continues asynchronously.
        When the connection is ready to use, the eFTL library calls your
        on_reconnect callback

        Reconnecting automatically re-activates all subscriptions
        on the connection. The eFTL library invokes your
        on_subscribe callback for each successful resubscription.

        :return:
        """
        if not self.is_connected():
            if self.previously_connected:
                self.attempt_manual_reconnect = True
                await self._attempt_connection()
                _call_dict_function(
                self.connect_options,
                    "on_reconnect",
                    connection=self
                )

    async def _connect(self, url, **kwargs):
        self.connect_options = kwargs

        if not isinstance(url, str):
            raise ValueError("URL must be a string")
        urls = [x.strip() for x in url.split("|")]
        self.urls = []

        for u in urls:
            if u[:5] != "ws://" and u[:6] != "wss://":
                raise ValueError("URL scheme must be ws or wss")
            else:
                parsed = urllib.parse.urlparse(u)
                if parsed.hostname is None:
                    raise ValueError("URL must have a valid host name")
                else:
                    self.urls.append(parsed)
        random.shuffle(self.urls)

        await self._attempt_connection(**kwargs)
        _call_dict_function(
            self.connect_options,
            "on_connect",
            connection=self
        )

    async def _attempt_connection(self, **kwargs):

        loop = kwargs.get("event_loop", None)
        if loop is not None:
            self.user_event_loop = loop

        timeout = kwargs.get(PROPERTY_HANDSHAKE_TIMEOUT)
        if timeout is not None:
            self.timeout = timeout

        polling_intrval = kwargs.get(PROPERTY_POLLING_INTERVAL)
        if polling_intrval is not None:
            self.polling_interval = polling_intrval

        trust_all = kwargs.get("trust_all")
        if trust_all is not None:
            self.trust_all = trust_all


        trust_store = kwargs.get("trust_store", None)
        if trust_store is not None:
            self.trust_store = trust_store

        self.url_list = self.urls

        auto_reconnect_attempts = kwargs.get(PROPERTY_AUTO_RECONNECT_ATTEMPTS)
        if auto_reconnect_attempts is not None:
            if not isinstance(auto_reconnect_attempts, numbers.Number):
                raise ValueError("Auto reconnect attempts value was non-numeric")
            else:
                if auto_reconnect_attempts < 0:
                    self.auto_reconnect_attempts = 256
                else:
                    self.auto_reconnect_attempts = auto_reconnect_attempts

        auto_reconnect_max_delay = kwargs.get(PROPERTY_AUTO_RECONNECT_MAX_DELAY)
        if auto_reconnect_max_delay is not None:
            if not isinstance(auto_reconnect_max_delay, numbers.Number):
                raise ValueError("Auto reconnect Max delay value was non-numeric")
            else:
                self.auto_reconnect_max_delay = auto_reconnect_max_delay


        login_timeout = kwargs.get(PROPERTY_LOGIN_TIMEOUT)
        if login_timeout is not None:
            if not isinstance(login_timeout, numbers.Number):
                raise ValueError("Login timeout value was non-numeric")
            else:
                self.login_timeout = login_timeout

        url_index = 0
        while url_index < len(self.urls):
            logger.debug("url index %s", url_index)
            try:
                url = self.urls[url_index]

                # This client factory is used to create websocket instances
                factory = self.client_factory(url.geturl())
                factory.setProtocolOptions(openHandshakeTimeout=self.timeout)

                """The websockets will be instances of a protocol, which contains definitions for
                websocket events like onOpen and onMessage. While the details of _do_connect vary
                between asyncio and twisted, both initalize a new _EftlClientProtocol instance and 
                its onConnecting event will occur. Our connection object (ie self) will have its 
                '_ws' field set as a reference to this new _EftlClientProtocol instance.
                """

                factory.protocol = self._get_client_protocol()
                factory.conn = self
                self.factory = factory

                if await self._do_connect(url, self.timeout, self.polling_interval):
                    return

            except (concurrent.futures._base.TimeoutError, OSError, EftlClientError) as e:
                logger.warning(e)

            url_index += 1

        raise EftlClientError("Connection failed across all provided urls")

    def is_connected(self):
        """
        Determine whether this connection to the eFTL server is open or closed.

        :return: True if this connection is open, False otherwise
        """
        return self._ws is not None and self.status == Eftl.CONNECTED

    def _permanently_closed(self, error="Attempted op while connection is closed.", warn=True):
        if self.status == Eftl.DISCONNECTED:
            return True

        return False

    async def disconnect(self):
        """
        Disconnect from the eFTL server.
        
        Programs may disconnect to free server resources.

        This call returns immediately; disconnecting continues
        asynchronously.

        When the connection has closed, the eFTL library calls your
        on_disconnect callback.
        
        :return:
        """
        if not self._permanently_closed():
            self._ws._disconnect()
        else:
            raise ConnectionError("Connection is closed.")

    async def subscribe(self, **kwargs):
        """
        Send a message to register a matcher-based subscription.

        Parameters
        ----------
        timeout : optional
            Number of seconds to halt waiting on acknowledgement from
            the server (default set in EftlConnection.connect()).
        matcher : str, optional
            JSON content matcher to subscribe to.
        type : {"standard", "shared", "last-value"}, optional
            Durable type.
        key : optional
            The last-value index key, if `type` is "last-value".
        durable : str, optional
            Name to give the durable.
        ack_mode: client, None. Default is auto

        Returns
        -------
        sub_id
            The subcription identifier of the new subscription.
        """
        if not self._permanently_closed():
            #check if matcher is valid JSON string
            try:
                sub_matcher = kwargs.get(MATCHER_FIELD)
                if sub_matcher is not None:
                    valid_matcher = json.loads(sub_matcher)
            except ValueError as error:
                raise ValueError("Invalid Matcher JSON string: " + error)

            sub_id = "{}{}{}".format(
                self.client_id,
                SUBSCRIPTION_TYPE,
                self._ws._next_subscription_sequence())
            return self._ws._subscribe(sub_id, False, **kwargs)
        else:
            raise ConnectionError("Connection is closed.")

    def _get_acknowledgement_mode(self, subscriber_identifier):
        sub = self.subscriptions.get(subscriber_identifier)
        if sub is not None:
            return sub.get(ACKNOWLEDGE_MODE)
        else:
            raise LookupError("No subscriber exist with this identifier")

    async def acknowledge(self, message):
        """
        Acknowledge this message.

        Messages consumed from subscriptions with a client acknowledgment mode
        must be explicitly acknowledged. The eFTL server will stop delivering
        messages to the client once the server's configured maximum number of
        unacknowledged messages is reached.

        :param message: The message being acknowledged
        :return:
        """
        seq_num = message._get_sequence_number_()
        sub_id = message._get_subscriber_id_()

        if not self._permanently_closed():
            if self._get_acknowledgement_mode(sub_id) == ACKNOWLEDGE_MODE_CLIENT:
                if seq_num is not None and sub_id is not None:
                    self._ws._ack(seq_num)
                else:
                    raise ValueError("Message should belong to subscriber for this operation")
            else:
                raise ValueError("Message should belong to subscriber with 'Client' Acknowledgement mode")
        else:
            raise ConnectionError("Connection is closed.")

    async def acknowledge_all(self, message):
        """
        Acknowledge all messages up to and including this message.

        Messages consumed from subscriptions with a client acknowledgment mode
        must be explicitly acknowledged. The eFTL server will stop delivering
        messages to the client once the server's configured maximum number of
        unacknowledged messages is reached.

        :param message: The message being acknowledged1
        :return:
        """
        seq_num = message._get_sequence_number_()
        sub_id = message._get_subscriber_id_()

        if not self._permanently_closed():
            if self._get_acknowledgement_mode(sub_id) == ACKNOWLEDGE_MODE_CLIENT:
                if seq_num is not None and sub_id is not None:
                    self._ws._ack(seq_num, sub_id)
                else:
                    raise ValueError("Message should belong to subscriber for this operation")
            else:
                raise ValueError("Message should belong to subscriber with 'Client' Acknowledgement mode")
        else:
            raise ConnectionError("Connection is closed.")

    async def send_request(self, message, timeout, **kwargs):
        """
        Publish a request message.

        This call returns immediately. When the reply is received
        the eFTL library calls your {@link RequestListener#onReply}
        callback.

        :param message: The request message to publish.
        :param timeout: timeout seconds to wait for reply
        :param kwargs:
              callbacks:
        :return:
        """
        if not self._permanently_closed():
            if isinstance(message, EftlMessage):
                request_metadata ={}
                request_metadata["is_request"] = True
                request_metadata["timeout"] = timeout
                self._ws._send_request(message, request_metadata, kwargs)
            else:
                raise TypeError("Only message of type 'EftlMessage' is allowed.")
        else:
            raise ConnectionError("Connection is closed.")

    async def send_reply(self, requestMessage, replyMessage, **kwargs):
        """
        Send a reply message in response to a request message.

        This call returns immediately. When the send completes successfully
        the eFTL library calls your {@link CompletionListener#onCompletion}
        callback.

        :param requestMessage: The reply message to send
        :param replyMessage: The request msg
        :param kwargs:
               callbacks
        :return:
        """
        if not self._permanently_closed():
            if isinstance(requestMessage, EftlMessage) and isinstance(replyMessage, EftlMessage):
                reply_to = requestMessage._get_reply_to_()
                if reply_to is not None:
                    reply_metadata = {}
                    reply_metadata["to_field"] = reply_to
                    reply_metadata["request_id"] = requestMessage._get_request_id_()
                    self._ws._send_reply(replyMessage, reply_metadata, kwargs)
                else:
                    raise TypeError("requestMessage should be an Eftl request message only.")
            else:
                raise TypeError("Only message of type 'EftlMessage' is allowed.")
        else:
            raise ConnectionError("Connection is closed.")

    async def unsubscribe(self, sub_id):
        """
        Unsubscribe from messages on a subscription.

        For durable subscriptions, this call will cause the persistence
        service to remove the durable subscription, along with any
        persisted messages.

        Programs receive subscription identifiers through their
        methods.

        :param sub_id:  Subscription identifier of the subscription to delete.
        timeout`
        :return:
        """
        if not self._permanently_closed():
            self._ws._unsubscribe(sub_id)
        else:
            raise ConnectionError("Connection is closed.")

    async def unsubscribe_all(self):
        """
        Unsubscribe from all subscriptions.

        Parameters
        ----------
        timeout : optional
            Number of seconds to halt waiting on acknowledgement from
            the server (default set in EftlConnection.connect()).
        """
        if not self._permanently_closed():
            self._ws._unsubscribe_all()
        else:
            raise ConnectionError("Connection is closed.")

    async def close_subscription(self, subscription_id):
        """
        Close subscription for specified subscription identifier.
        
        For durable subscriptions, this call will cause the persistence
        service to stop delivering messages while leaving the durable
        subscriptions to continue accumulating messages. Any unacknowledged
        messages will be made available for redelivery.
        
        Programs receive subscription identifiers through their
        on_subscribe method.

        """
        if not self._permanently_closed():
            self._ws._close_subscription(subscription_id)
        else:
            raise ConnectionError("Connection is closed.")

    async def close_all_subscriptions(self):
        """
        Close all the subscriptions on this connection.

        For durable subscriptions, this call will cause the persistence
        service to stop delivering messages while leaving the durable
        subscriptions to continue accumulating messages. Any unacknowledged
        messages will be made available for redelivery.

        Programs receive subscription identifiers through their
        on_subscribe method.

        """
        if not self._permanently_closed():
            self._ws._close_all_subscriptions()
        else:
            raise ConnectionError("Connection is closed.")

    async def publish(self, message, **kwargs):
        """
        Publish a one-to-many message to all subscribing clients.

        This call returns immediately; publishing continues
        asynchronously.  When the publish completes successfully,
        the eFTL library calls your

        :param message: Publish this message.
        :param kwargs:
               callbacks:
               on_publish
               on_error
        :return:

        ValueError
            If the message size exceeds the maximum.
        TypeError
            If the message is not of type EftlMessage
        ConnectionError
            If the connection is closed.

        """
        if not self._permanently_closed():
            if isinstance(message, EftlMessage):
                self._ws._publish(message, None, None, kwargs)
            else:
                raise TypeError("Only message of type 'EftlMessage' is allowed to publish.")
        else:
            raise ConnectionError("Connection is closed.")

    async def create_kv_map(self, name):
        """
        Return a new EftlKVMap associated with this connection.
        
        :param name: name of the KVMap
        :return:  The KVmap object
        """
        if not self._permanently_closed():
            if isinstance(name, str):
                return EftlKVMap(self, name)
            else:
                raise TypeError("kv_map name should be of type 'str'")
        else:
            raise ConnectionError("Connection is closed.")

    async def remove_kv_map(self, name):
        """
        Remove a EftlKVMap object.
        
        :param name: the name of the map
        :return:
        """
        if not self._permanently_closed():
            if isinstance(name, str):
                self._ws._remove_kv_map(name)
            else:
                raise TypeError("kv_map name should be of type 'str'")
        else:
            raise ConnectionError("Connection is closed.")

    def create_message(self):
        """
        Create an EftlMessage that can be used to publish or send request/replies.

        :return: The EftlMessage object

        """
        if not self._permanently_closed():
            return EftlMessage()
        else:
            raise ConnectionError("Connection is closed.")

    async def _async_sleep(self, t):
        await asyncio.sleep(t)

    def _schedule_reconnect(self):
        logger.debug("Scheduling reconnect")

        # check to see if we were ever connected, if not 
        # schedule reconnect right away with next URL.
        reconn_delay = 0

        if self.reconnect_counter < self.auto_reconnect_attempts:
            self._next_url()
            if self.reconnect_counter < 1:
                self.first_retry_delay = random.random() + 0.5
                reconn_delay = self.first_retry_delay
            else:
                reconn_delay = min(
                            (2 ** self.reconnect_counter) * self.first_retry_delay, 
                            self.auto_reconnect_max_delay
                            )
            self.reconnect_counter += 1
        else:
            # look if it is necessary
            self.connection_closed = True
            return False

        task1 = self.user_event_loop.create_task(self._async_sleep(reconn_delay))
        task1.add_done_callback(self._call_try_reconnect)

        return True

    def _call_try_reconnect(self, task):
        self._try_reconnect()

    def _next_url(self):
        self.url_index += 1
        if self.url_index >= len(self.url_list):
            self.url_index = 0

    def _try_reconnect(self):
        logger.debug("attempting to reconnect ")
        self._set_status(Eftl.RECONNECTING)
        self.reconnect_timer = None
        self.got_login_reply = False
        self._do_reconnect(self.url_list[self.url_index], self.timeout, self.polling_interval)

    def _get_client_protocol(self):
        class _EftlClientProtocol(self.client_protocol):

            def __init__(self):
                super().__init__()

                self.timeout = 600
                self.heartbeat = 240
                self.max_message_size = 0
                self.last_message = 0
                self.timeout_check = None
                self.qos = True

            ######################### Websocket events ###############################

            def onConnecting(self, transport_details):
                self.factory.conn._ws = self
                self.factory.conn._set_status(Eftl.CONNECTING)

            def onOpen(self):
                login_message = {
                    OP_FIELD: OP_LOGIN,
                    PROTOCOL_FIELD: 1,
                    CLIENT_TYPE_FIELD: "python",
                    CLIENT_VERSION_FIELD: _EFTL_VERSION,
                    }

                conn_opts = self.factory.conn.connect_options
                url = self.factory.conn.url_list[self.factory.conn.url_index]

                password = url.password
                if password is None:
                    password = conn_opts.get(PROPERTY_PASSWORD, "")
                login_message[PASSWORD_FIELD] = password

                username = url.username
                if username is None:
                    username = conn_opts.get(PROPERTY_USERNAME, "")
                login_message[USER_FIELD] = username

                max_pending_acks = conn_opts.get(PROPERTY_MAX_PENDING_ACKS, 0)
                login_message[MAX_PENDING_ACKS_FIELD] = max_pending_acks

                client_id = None

                if self.factory.conn.client_id is None:
                    client_id = urllib.parse.parse_qs(url.query).get("client_id", [None])[0]
                    if client_id is None:
                        client_id = conn_opts.get(PROPERTY_CLIENT_ID)
                        self.factory.conn.client_id = client_id

                if self.factory.conn.reconnect_token is not None:
                    login_message[ID_TOKEN_FIELD] = self.factory.conn.reconnect_token

                if self.factory.conn.client_id is not None:
                    login_message[CLIENT_ID_FIELD] = self.factory.conn.client_id

                options = {}
                options[QOS_FIELD] = "true"
                options[RESUME_FIELD] = "true"

                login_message[LOGIN_OPTIONS_FIELD] = options

                self.factory.conn._set_status(Eftl.CONNECTING)
                self._send(login_message, 0, False)

                # Set up a timeout to be cancelled in _handle_welcome()
                self.welcome_timer = threading.Timer(self.factory.conn.login_timeout, self._dc_without_welcome)
                self.welcome_timer.start()

            def _dc_without_welcome(self):
                logger.debug("handle welcome timer ....")
                #asyncio.set_event_loop(asyncio.new_event_loop())
                self.sendClose(code=WS_NORMAL_CLOSE)

            def onMessage(self, payload, is_binary=False):
                msg = json.loads(payload.decode('utf8'))
                logger.debug("Received message:{}".format(msg))

                self.last_message = datetime.datetime.now()

                op_code = msg.get(OP_FIELD)
                if op_code == OP_HEARTBEAT:
                    self._send(msg, 0, False, payload.decode('utf8'))
                elif op_code == OP_WELCOME:
                    self._handle_welcome(msg)
                elif op_code == OP_SUBSCRIBED:
                    self._handle_subscribed(msg)
                elif op_code == OP_UNSUBSCRIBED:
                    self._handle_unsubscribed(msg)
                elif op_code == OP_EVENT:
                    self._handle_message(msg)
                elif op_code == OP_ERROR:
                    self._handle_error(msg)
                elif op_code == OP_ACK:
                    self._handle_ack(msg)
                elif op_code == OP_REQUEST_REPLY:
                    self._handle_reply(msg)
                elif op_code == OP_MAP_RESPONSE:
                    self._handle_map_response(msg)
                else:
                    logger.debug("Received unknown op code ({}):\n{}".format(op_code, msg))

            def onClose(self, was_clean, code, reason="connection failed"):

                self.factory.conn.connection_closed = True
                self.factory.conn.got_login_reply = False

                logger.warning("Websocket closed. Reason: {} Code: {}".format(reason, code))

                if hasattr(self, 'welcome_timer') and self.welcome_timer:
                    self.welcome_timer.cancel()
                    self.welcome_timer = None

                if hasattr(self, 'timeout_check') and self.timeout_check:
                    self.timeout_check.cancel()
                    self.timeout_check = None

                self.factory.conn.code = code
                self.factory.conn.reason = reason

                self.factory.conn._set_status(Eftl.DISCONNECTED)
                
                if code != RESTART and code!=WS_ABNORMAL_CLOSE or self.factory.conn._schedule_reconnect() == False:
                    self._clean_up(code, reason)

            def _clean_up(self, code, reason):
                self._clear_pending(ERR_PUBLISH_FAILED, "Closed")
                self.factory.conn._ws = None
                self.factory.conn.got_login_reply = False
                if not self.factory.conn.attempt_manual_reconnect:
                    _call_dict_function(
                        self.factory.conn.connect_options,
                        "on_disconnect",
                        connection=self.factory.conn, loop=self.factory.conn.user_event_loop, code=code, reason=reason
                    )
                else:
                    _call_dict_function(
                        self.factory.conn.connect_options,
                        "on_error",
                        connection=self.factory.conn, code=code, reason="Reconnect failed"
                    )

            ##################### Protocol message handlers ##########################

            def _handle_welcome(self, message):
                logger.debug("handle_welcome..")
                self.factory.conn.previously_connected = True
                resume = str(message.get(RESUME_FIELD, "")).lower() == "true"
                self.factory.conn.client_id = message.get(CLIENT_ID_FIELD)
                self.factory.conn.reconnect_token = message.get(ID_TOKEN_FIELD)
                self.timeout = message.get(TIMEOUT_FIELD)
                self.heartbeat = message.get(HEARTBEAT_FIELD)
                self.max_message_size = message.get(MAX_SIZE_FIELD)

                self.qos = str(message.get(QOS_FIELD, "")).lower() == "true"

                self.timeout_check = threading.Timer(
                    self.timeout,
                    self._do_timeout_check)

                self.timeout_check.start()

                self.factory.conn._set_status(Eftl.CONNECTED)
                self.factory.conn.reconnect_timer = 0
                self.factory.conn.url_index = 0
                self.factory.conn.reconnect_counter = 0

                # repair subscriptions
                for k, v in self.factory.conn.subscriptions.items():
                    if resume == False:
                        v["last_received_sequence_number"] = -1
                    v["pending"] = True
                    self._subscribe(k, True, **(v.get("options")))

                # resending unacknowledged messages
                logger.debug("send pending unacked msgs")
                self._send_pending()

                """
                if not self.factory.conn.is_reconnecting:
                    func_name = "on_reconnect" if previously_connected else "on_connect"
                    logger.debug("func_name on either connect or reconnect = %s", func_name)
                    _call_dict_function(self.factory.conn.connect_options, func_name, connection=self.factory.conn)
                """

                self.factory.conn.got_login_reply = True
                self.attempt_manual_reconnect = False
                self.welcome_timer.cancel()

            def _do_timeout_check(self):
                now = datetime.datetime.now()
                diff = datetime.datetime.now() - self.last_message

                if diff.total_seconds() > self.timeout:
                    logger.debug("last_message in timer, ", {
                        "id": self.factory.conn.client_id,
                        "now": now,
                        "last_message": self.last_message,
                        "timeout": self.timeout,
                        "diff": diff,
                        "evaluate": (diff.total_seconds() > self.timeout)
                        })

                    self._close("Connection timeout", False)

            def _subscribe(self, sub_id, subscriber_obj_exist, **kwargs):
                if not subscriber_obj_exist:
                    subscription = {
                        "options": kwargs,
                        "id": sub_id,
                        "pending": True,
                        "last_received_sequence_number": -1,
                        }

                    ack_mode_str = kwargs.get(ACK_FIELD)
                    if ack_mode_str is not None:
                        if ack_mode_str == ACKNOWLEDGE_MODE_CLIENT:
                            subscription[ACKNOWLEDGE_MODE] = ACKNOWLEDGE_MODE_CLIENT
                        elif ack_mode_str == ACKNOWLEDGE_MODE_NONE:
                            subscription[ACKNOWLEDGE_MODE] = ACKNOWLEDGE_MODE_NONE
                        else:
                            # ack mode is auto by default
                            subscription[ACKNOWLEDGE_MODE] = ACKNOWLEDGE_MODE_AUTO
                    else:
                        # ack mode is auto by default
                        subscription[ACKNOWLEDGE_MODE] = ACKNOWLEDGE_MODE_AUTO
                    self.factory.conn.subscriptions[sub_id] = subscription

                sub_message = {
                    OP_FIELD: OP_SUBSCRIBE,
                    ID_FIELD: sub_id,
                }

                sub_matcher = kwargs.get(MATCHER_FIELD)
                if sub_matcher:
                    sub_message[MATCHER_FIELD] = sub_matcher
                sub_durable = kwargs.get(DURABLE_FIELD)
                if sub_durable:
                    sub_message[DURABLE_FIELD] = sub_durable

                sub_durable_type = kwargs.get(PROPERTY_DURABLE_TYPE)
                if sub_durable_type == DURABLE_TYPE_SHARED or sub_durable_type == DURABLE_TYPE_LAST_VALUE:
                    sub_message[PROPERTY_DURABLE_TYPE] = sub_durable_type
                    if sub_durable_type == DURABLE_TYPE_LAST_VALUE:
                        sub_last_durable_key = kwargs.get(PROPERTY_DURABLE_KEY)
                        if sub_last_durable_key:
                            sub_message[PROPERTY_DURABLE_KEY] = sub_last_durable_key

                self._send(sub_message, 0, False)
                return sub_id

            def _next_subscription_sequence(self):
                self.factory.conn.subscription_counter += 1
                return self.factory.conn.subscription_counter


            def _handle_subscribed(self, message):
                sub_id = message.get(ID_FIELD)

                sub = self.factory.conn.subscriptions.get(sub_id)
                if sub and sub.get("pending") is not None:
                    sub["pending"] = False
                    _call_dict_function(sub.get("options"), "on_subscribe", id=sub_id, )

            def _remove_kv_map(self, name):
                remove_map_message = {
                    OP_FIELD: OP_MAP_DESTROY,
                    MAP_FIELD: name
                    }
                self._send(remove_map_message, 0, False)

            def _unsubscribe(self, sub_id):
                unsub_message = {
                    OP_FIELD: OP_UNSUBSCRIBE,
                    ID_FIELD: sub_id,
                    }

                # delete the subscription id from the subscriptions dictionary.
                del self.factory.conn.subscriptions[sub_id]
                self._send(unsub_message, 0, False)

            def _unsubscribe_all(self):
                for k in self.factory.conn.subscriptions:
                    self._unsubscribe(k)

            def _close_subscription(self, sub_id):
                close_subscription_message = {
                    OP_FIELD: OP_UNSUBSCRIBE,
                    ID_FIELD: sub_id,
                    DEL_FIELD: False,
                }
                # delete the subscription id from the subscriptions dictionary.
                del self.factory.conn.subscriptions[sub_id]

                self._send(close_subscription_message, 0, False)

            def _close_all_subscriptions(self):
                copy_subs = copy.deepcopy(self.factory.conn.subscriptions)
                for sub_id in copy_subs:
                    self._close_subscription(sub_id)

            def _handle_unsubscribed(self, message):
                sub_id = message.get(ID_FIELD)
                err_code = message.get(ERR_CODE_FIELD)
                reason = message.get(REASON_FIELD)

                sub = self.factory.conn.subscriptions.get(sub_id)
                if sub is not None:
                    if err_code == SUBSCRIPTION_INVALID:
                       del self.factory.conn.subscriptions[sub_id]

                    _call_dict_function(
                        sub.get("options"),
                        "on_error",
                        id=sub_id, code=err_code, reason=reason)


            # for publish only
            def _publish(self, message, request_metadata, reply_metadata, options):
                m = json.dumps(message, cls=_EFTLJSONEncoder)
                encodedMsg = json.loads(m)

                sequence = self._next_sequence()
                envelope = {
                    OP_FIELD: OP_MESSAGE,
                    BODY_FIELD: encodedMsg,
                    SEQ_NUM_FIELD: sequence
                }

                publish = {
                    "options": options,
                    "actual_message": message,
                    "message": encodedMsg,
                    "envelope": envelope,
                    "sequence": sequence,
                    }

                self._send_request_on_conn(publish, sequence, verify_size=True)

 
            # for sending requests (sendRequest API)
            def _send_request(self, message, request_metadata, options):
                m = json.dumps(message, cls=_EFTLJSONEncoder)
                encodedMsg = json.loads(m)

                sequence = self._next_sequence()

                # OP_FIELD should OP_REQUEST since it's a request
                envelope = {
                    OP_FIELD: OP_REQUEST,
                    BODY_FIELD: encodedMsg,
                    SEQ_NUM_FIELD: sequence
                }

                # use the timeout specified by the user
                timeout = request_metadata.get("timeout")
                request_timeout_check_timer = threading.Timer(timeout, self._handle_request_timeout, [None, sequence])

                request = {
                    "options": options,
                    "actual_message": message,
                    "message": encodedMsg,
                    "envelope": envelope,
                    "sequence": sequence,
                    "request_timer": request_timeout_check_timer,
                    }

                self._send_request_on_conn(request, sequence, verify_size=True)

                # now that we have sent our request, start the timer
                request_timeout_check_timer.start()
                

            # for sending replies (sendReply API)
            def _send_reply(self, message, reply_metadata, options):
                m = json.dumps(message, cls=_EFTLJSONEncoder)
                encodedMsg = json.loads(m)

                sequence = self._next_sequence()
                reqId    = reply_metadata.get("request_id")
                replyTo  = reply_metadata.get("to_field")

                # OP_FIELD should OP_REPLY since it's a REPLY
                envelope = {
                    OP_FIELD: OP_REPLY,
                    TO_FIELD: replyTo,
                    REQ_ID_FIELD: reqId,
                    BODY_FIELD: encodedMsg,
                    SEQ_NUM_FIELD: sequence
                }

                reply = {
                    "options": options,
                    "message": encodedMsg,
                    "envelope": envelope,
                    "sequence": sequence,
                    }

                self._send_request_on_conn(reply, sequence, verify_size=True)

             
            # This function used in few places to send the request on web socket connection
            def _send_request_on_conn(self, request, sequence, verify_size=False):
                envelope = request.get("envelope")
                data     = json.dumps(envelope, cls=_EFTLJSONEncoder)

                if verify_size and self.max_message_size > 0 and len(data) > self.max_message_size:
                    raise EftlMessageSizeTooLarge("Message exceeds max size ({})".format(self.max_message_size))

                self.factory.conn.requests[sequence] = request

                if self.factory.conn.status < Eftl.DISCONNECTING:
                    self._send(envelope, sequence, False, data)

            def _next_sequence(self):
                self.factory.conn.sequence_counter += 1
                return self.factory.conn.sequence_counter

            def _handle_ack(self, message):
                sequence = message.get(SEQ_NUM_FIELD)
                err_code = message.get(ERR_CODE_FIELD)
                reason   = message.get(REASON_FIELD)

                if sequence is not None:
                    if not err_code:
                        self._pending_complete(sequence)
                    else:
                        self._pending_error(sequence, err_code, reason)

            def _handle_reply(self, message):
                sequence = message.get(SEQ_NUM_FIELD)
                err_code = message.get(ERR_CODE_FIELD)
                reason   = message.get(REASON_FIELD)
                data     = message.get(BODY_FIELD)

                req = self.factory.conn.requests.get(sequence)
                if req is None:
                    return

                # get the request timer and cancel it
                request_timer = req.get("request_timer")
                if request_timer is not None:
                    request_timer.cancel()

                if sequence is not None:
                    if not err_code:
                        # call on_reply callback given by send_request if body is present
                        if data is not None:
                            eftl_message = EftlMessage(data)
                            if req is not None:
                                del self.factory.conn.requests[sequence]
                                _call_dict_function(
                                    req.get("options"),
                                    "on_reply",
                                    message=eftl_message,
                                    )
                    else:
                        self._pending_error(sequence, err_code, reason)


            def _handle_map_response(self, message):
                sequence = message.get(SEQ_NUM_FIELD)
                err_code = message.get(ERR_CODE_FIELD)
                value    = message.get(VALUE_FIELD)
                reason   = message.get(REASON_FIELD)

                eftl_message = None

                if sequence is not None:
                    if err_code is None:
                        if value is not None:
                            eftl_message = EftlMessage(value)

                        self._pending_response(sequence, eftl_message)
                    else:
                        self._pending_error(sequence, err_code, reason)


            def _handle_message(self, message):
                sub        = self.factory.conn.subscriptions.get(message.get(TO_FIELD))
                seq_num    = message.get(SEQ_NUM_FIELD)
                data       = message.get(BODY_FIELD)
                sid        = message.get(STORE_MSG_ID_FIELD)
                cnt        = message.get(DELIVERY_COUNT_FIELD)
                reply_to   = message.get(REPLY_TO_FIELD)
                request_id = message.get(REQ_ID_FIELD)

                if sub is not None and seq_num is not None:
                    if seq_num > sub.get('last_received_sequence_number'):
                        if sub.get("options") is not None:

                            if data is not None:
                                eftl_message = EftlMessage(data)

                                eftl_message._set_sequence_number_(seq_num)

                                sub_id = sub.get(ID_FIELD)
                                if sub_id is not None:
                                    eftl_message._set_subscriber_id_(sub_id)

                                if sid is not None:
                                    eftl_message._set_sid_(sid)

                                if cnt is not None:
                                    eftl_message._set_cnt_(cnt)

                                if reply_to is not None:
                                    eftl_message._set_reply_to_(reply_to)

                                if request_id is not None:
                                    eftl_message._set_request_id_(request_id)

                                _call_dict_function(sub.get("options"), "on_message", message=eftl_message)

                        sub['last_received_sequence_number'] = seq_num

                    if sub.get(ACKNOWLEDGE_MODE) == ACKNOWLEDGE_MODE_AUTO:
                        sub_id = sub.get(ID_FIELD)
                        self._ack(seq_num, sub_id)


            def _ack(self, seq_num, sub_id=None):
                if seq_num <= 0:
                    return

                envelope = {
                    OP_FIELD: OP_ACK,
                    SEQ_NUM_FIELD: seq_num
                    }

                if sub_id is not None:
                    envelope[ID_FIELD] = sub_id
                self._send(envelope, 0, False)


            def _handle_error(self, message):
                err_code = message.get(ERR_CODE_FIELD)
                reason = message.get(REASON_FIELD)

                _call_dict_function(
                    self.factory.conn.connect_options,
                    "on_error",
                    connection=self, code=err_code, reason=reason
                    )
                """
                if self.factory.conn.status == Eftl.CONNECTING:
                    self._close(reason, False)
                """

            # if send_request fails with a timeout
            def _handle_request_timeout(self, *args, **kwargs):
                loop = self.factory.conn.user_event_loop
                if loop is None:
                    loop = asyncio.new_event_loop()

                asyncio.set_event_loop(loop)
                sequence = args[1]
                err_code = REQUEST_TIMEOUT
                reason = "request timeout"

                logger.debug("request timer fired")
                req = self.factory.conn.requests.get(sequence)
                if req is not None:
                    del self.factory.conn.requests[sequence]
                    message = req.get("actual_message")
                    _call_dict_function(
                        req.get("options"),
                        "on_error",
                        message=message, code=err_code, reason=reason)
	    	

            #################### Buffering and sending messages ######################

            def _send(self, message, seq_num, force_send, stringified=None):
                if stringified is not None:
                    data = stringified
                else:
                    data = json.dumps(message, cls=_EFTLJSONEncoder)

                data = data.encode('utf8')

                if self.factory.conn.status < Eftl.DISCONNECTING or force_send:
                    try:
                        logger.debug("Sending message: {}".format(data))
                        self.sendMessage(data)

                    except Exception as e:
                        logger.error("Exception during send:\n{}".format(e))
                        pass

            def _send_pending(self):
                for k in sorted(self.factory.conn.requests):
                    req = self.factory.conn.requests[k]
                    self._send(req["envelope"], req["sequence"], False)

            def _clear_pending(self, err_code, reason):
                for k in sorted(self.factory.conn.requests):
                    req = self.factory.conn.requests[k]
                    if req is not None:
                        del self.factory.conn.requests[k]
                        _call_dict_function(
                            req.get("options"),
                            "on_error",
                            message=req["actual_message"], code=err_code, reason=reason
                            )

            def _pending_complete(self, sequence):
                req = self.factory.conn.requests.get(sequence)
                if req is not None:
                    msg = req.get("message")
                    del self.factory.conn.requests[sequence]
                    _call_dict_function(
                        req.get("options"),
                        "on_complete",
                        message=msg)

            def _pending_response(self, sequence, response):
                req = self.factory.conn.requests.get(sequence)

                value = req.get("message")
                if value is not None:
                    response = value

                if req is not None:
                    del self.factory.conn.requests[sequence]
                    if "options" in req:
                        key = req.get("envelope", {}).get(KEY_FIELD)

                        _call_dict_function(
                            req.get("options"),
                            "on_success",
                            message=response, key=key)

            def _pending_error(self, sequence, err_code, reason):
                req = self.factory.conn.requests.get(sequence)
                if req is not None:
                    del self.factory.conn.requests[sequence]
                    if "options" in req:
                        key = req.get("envelope", {}).get(KEY_FIELD)
                        if key is not None:
                            _call_dict_function(
                                req.get("options"),
                                "on_error",
                                message=req.get("actual_message"), code=err_code, reason=reason, key=key)
                        else:
                            _call_dict_function(
                                req.get("options"),
                                "on_error",
                                message=req.get("actual_message"), code=err_code, reason=reason)
                    else:
                        _call_dict_function(
                            self.factory.conn.connect_options,
                            "on_error",
                            connection=self, code=err_code, reason=reason
                        )

            ############################ Disconnecting ###############################

            def _disconnect(self):
                self._clear_pending(ERR_PUBLISH_FAILED, "Disconnected")
                _call_dict_function(
                    self.factory.conn.connect_options,
                    "on_disconnect",
                    connection=self, loop=self.factory.conn.user_event_loop, code=WS_NORMAL_CLOSE, reason="Disconnect")

                self._close("Connection closed by user.", True)

            def _close(self, reason="User Action", notify_server=False):
                #if self.factory.conn.status != Eftl.CONNECTING and self.factory.conn.status != Eftl.CONNECTED:
                if self.factory.conn.status > Eftl.RECONNECTING:
                    return

                self.factory.conn._set_status(Eftl.DISCONNECTING)

                if self.timeout_check:
                    self.timeout_check.cancel()
                    self.timeout_check = None


                if notify_server:
                    dc_message = {
                        OP_FIELD: OP_DISCONNECT,
                        REASON_FIELD: reason,
                        }
                    self._send(dc_message, 0, True)
                    logger.debug("Notified the server to close the connection")

                self.sendClose(WS_NORMAL_CLOSE)

        return _EftlClientProtocol
