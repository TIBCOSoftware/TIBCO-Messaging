import datetime
import json
import logging
import numbers
import random
import threading
import time
import urllib
import concurrent.futures
from abc import ABC, abstractmethod

import jsonpickle

from eftl.kvmap import _EftlKVMap


EFTL_VERSION = "6.2.0 dev 1"
EFTL_WS_PROTOCOL = "v1.eftl.tibco.com"

OP_HEARTBEAT = 0
OP_LOGIN = 1
OP_WELCOME = 2
OP_SUBSCRIBE = 3
OP_SUBSCRIBED = 4
OP_UNSUBSCRIBE = 5
OP_UNSUBSCRIBED = 6
OP_EVENT = 7
OP_MESSAGE = 8
OP_ACK = 9
OP_ERROR = 10
OP_DISCONNECT = 11
OP_MAP_SET = 20
OP_MAP_GET = 22
OP_MAP_REMOVE = 24
OP_MAP_RESPONSE = 26
ERR_PUBLISH_FAILED = 11

WS_NORMAL_CLOSE = 1000
WS_ABNORMAL_CLOSE = 1006

USER_PROP = "user"
USERNAME_PROP = "username"
PASSWORD_PROP = "password"
CLIENT_ID_PROP = "client_id"
AUTO_RECONNECT_ATTEMPTS_PROP = "auto_reconnect_attempts"
AUTO_RECONNECT_MAX_DELAY_PROP = "auto_reconnect_max_delay"

OP_FIELD = "op"
USER_FIELD = "user"
PASSWORD_FIELD = "password"
CLIENT_ID_FIELD = "client_id"
CLIENT_TYPE_FIELD = "client_type"
CLIENT_VERSION_FIELD = "client_version"
ID_TOKEN_FIELD = "id_token"
TIMEOUT_FIELD = "timeout"
HEARTBEAT_FIELD = "heartbeat"
MAX_SIZE_FIELD = "max_size"
MATCHER_FIELD = "matcher"
DURABLE_FIELD = "durable"
ACK_FIELD = "ack"
ERR_CODE_FIELD = "err"
REASON_FIELD = "reason"
ID_FIELD = "id"
TO_FIELD = "to"
BODY_FIELD = "body"
SEQ_NUM_FIELD = "seq"
LOGIN_OPTIONS_FIELD = "login_options"
RESUME_FIELD = "_resume"
QOS_FIELD = "_qos"
MAP_FIELD = "map"
KEY_FIELD = "key"
VALUE_FIELD = "value"

SUBSCRIPTION_TYPE = ".s."

OPENING = 0
OPEN = 1
CLOSING = 2
CLOSED = 3

logger = logging.getLogger(__name__)
logger.addHandler(logging.NullHandler())


class EftlClientError(Exception):
    """Raise when establishing a connection to the server fails."""


def _call_dict_function(dic, name, **kwargs):
    """Call dic[name](**kwargs), if it exists."""
    try:
        func = dic.get(name)
        if func is None:
            logger.debug("User did not give a callback for this event. kwargs:\n{}".format(kwargs))
        else:
            func(**kwargs)
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


class EftlConnectionBase(ABC):
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
        self.client_protocol = None
        self.client_factory = None
        self.halt = True

    @abstractmethod
    def _do_connect(self):
        raise NotImplementedError

    @abstractmethod
    def _halt_until_signal(self):
        raise NotImplementedError

    @abstractmethod
    def run_event_loop(self, interval=None):
        """
        Run the current event loop.
        Parameters
        ----------
        interval
            Number of seconds to run the loop. If None, run it forever.
        """
        raise NotImplementedError

    def connect(self, url, **kwargs):
        """
        Try connecting to one of the urls until success or a timeout.

        Parameters
        ----------
        url : str
            Potentially multiple urls separated by the pipe character.
            The url scheme must be ws or wss, and must have a host name.
        qos : bool, optional
            Whether or not Quality of Service is enabled.
        auto_reconnect_attempts : int, optional
            Maximum number of reconnect attempts.
        auto_reconnect_max_delay : optional
            Maximum reconnect delay in milliseconds.
        username : optional
            Login credentials to use if not found in the url.
        user : optional
            Alternative if username is not specified.
        password : optional
            Login credentials to use if not found in the url.
        client_id : optional
            User-specified client identifier.
        handshake_timeout : optional
            Seconds to wait for websocket handshake to complete.
        login_timeout : optional
            Seconds to halt waiting for a login message reply. If a
            reply is not received in time, raise an EftlClientError.
        response_timeout : optional
            Seconds to halt waiting for other message replies. If a
            reply is not received in time, resume normal operation.
        polling_interval : optional
            Seconds to wait between each message reply check.
        serializer : function, optional
            Function that must be able to serialize a message (arbitrary
            python object) to JSON. Default is jsonpickle.encode.

        Raises
        ------
        ValueError
            If any provided urls lack a host name or the correct scheme.
        EftlClientError
            If the connection is not established in time.
        """
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

        self._attempt_connection(**kwargs)

    def _attempt_connection(self, **kwargs):
        timeout = kwargs.get("handshake_timeout", 15)
        polling_interval = kwargs.get("polling_interval", 0.2)

        self.url_index = 0
        while self.url_index < len(self.urls):
            try:
                url = self.urls[self.url_index]

                # This client factory is used to create websocket instances
                factory = self.client_factory(url.geturl())
                factory.setProtocolOptions(openHandshakeTimeout=timeout)
                """The websockets will be instances of a protocol, which contains definitions for
                websocket events like onOpen and onMessage. While the details of _do_connect vary
                between asyncio and twisted, both initalize a new _EftlClientProtocol instance and 
                its onConnecting event will occur. Our connection object (ie self) will have its 
                '_ws' field set as a reference to this new _EftlClientProtocol instance."""
                factory.protocol = self._get_client_protocol()
                factory.conn = self
                self.factory = factory

                if self._do_connect(url, timeout, polling_interval):
                    return
            except (concurrent.futures._base.TimeoutError, OSError, EftlClientError) as e:
                logger.warning(e)

            self.url_index += 1

        raise EftlClientError("Connection failed across all provided urls")

    def is_connected(self):
        """Return whether or not this connection is still open."""
        return self._ws is not None and self._ws.status == OPEN

    def _permanently_closed(self, error="Attempted op while connection is closed.", warn=True):
        if self._ws is None or self._ws.status == CLOSED and not self._ws.is_reconnecting:
            raise EftlClientError(message)
        elif warn and self._ws is not None and self._ws.status != OPEN:
            logger.warning("Connection is not open")
        return False

    def disconnect(self):
        """Gracefully disconnect, sending a message to the server."""
        if not self._permanently_closed():
            self._ws._disconnect()

    def subscribe(self, timeout="use default", **kwargs):
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

        Returns
        -------
        sub_id
            The subcription identifier of the new subscription.
        """
        if not self._permanently_closed():
            sub_id = "{}{}{}".format(
                self._ws.client_id,
                SUBSCRIPTION_TYPE,
                self._ws._next_subscription_sequence())
            return self._ws._subscribe(sub_id, timeout, **kwargs)

    def unsubscribe(self, sub_id, timeout="use default"):
        """
        Unsubscribe from the given subscription identifier.

        Parameters
        ----------
        sub_id
            Subscription identifier of the subscription to delete.
        timeout : optional
            Number of seconds to halt waiting on acknowledgement from
            the server (default set in EftlConnection.connect()).
        """
        if not self._permanently_closed():
            self._ws._unsubscribe(sub_id, timeout)

    def unsubscribe_all(self, timeout="use default"):
        """
        Unsubscribe from all subscriptions.

        Parameters
        ----------
        timeout : optional
            Number of seconds to halt waiting on acknowledgement from
            the server (default set in EftlConnection.connect()).
        """
        if not self._permanently_closed():
            self._ws._unsubscribe_all(timeout)

    def publish(self, message, timeout="use default", **kwargs):
        """
        Publish a message to the server.

        Parameters
        ----------
        message
            Object to be serialized to JSON and sent.
        timeout: optional
            Number of seconds to halt waiting on acknowledgement from
            the server (default set in EftlConnection.connect()).
        on_complete, on_error : function, optional
            Function to be run on completion or failure of the delivery.

        Raises
        ------
        ValueError
            If the message size exceeds the maximum.
        """
        if not self._permanently_closed():
            self._ws._publish(message, timeout, kwargs)

    def map(self, name):
        """Return a new _EftlKVMap associated with this connection."""
        return _EftlKVMap(self, name)

    def _get_client_protocol(self):
        class _EftlClientProtocol(self.client_protocol):

            def __init__(self):
                super().__init__()
                self.url_index = 0
                self.status = CLOSED
                self.client_id = None
                self.reconnect_token = None
                self.timeout = 600
                self.heartbeat = 240
                self.max_message_size = 0
                self.last_message = 0
                self.timeout_check = None
                self.subscriptions = {}
                self.sequence_counter = 0
                self.subscription_counter = 0
                self.reconnect_counter = 0
                self.auto_reconnect_attempts = 5
                self.auto_reconnect_max_delay = 30
                self.reconnect_timer = None
                self.connect_timer = None
                self.is_reconnecting = False
                self.got_login_reply = False
                self.qos = True
                self.requests = {}
                self.last_sequence_number = 0

            ######################### Websocket events ###############################

            def onConnecting(self, transport_details):

                self.factory.conn._ws = self
                self.url_list = self.factory.conn.urls
                self.url_index = self.factory.conn.url_index

                conn_opts = self.factory.conn.connect_options

                self.connect_options = conn_opts
                self.login_timeout = conn_opts.get("login_timeout", 15)
                self.default_response_timeout = conn_opts.get("response_timeout", None)
                self.serialize = conn_opts.get("serializer", jsonpickle.encode)

                auto_reconn_att = conn_opts.get(AUTO_RECONNECT_ATTEMPTS_PROP, None)
                if isinstance(auto_reconn_att, numbers.Number):
                    self.auto_reconnect_attempts = auto_reconn_att
                elif auto_reconn_att is not None:
                    raise ValueError("Auto reconnect attempts value was non-numeric")

                auto_reconn_delay = conn_opts.get(AUTO_RECONNECT_MAX_DELAY_PROP, None)
                if isinstance(auto_reconn_delay, numbers.Number):
                    self.auto_reconnect_max_delay = auto_reconn_delay
                elif auto_reconn_delay is not None:
                    raise ValueError("Auto reconnect max delay value was non-numeric")

                self.status = OPENING

            def onOpen(self):
                login_message = {
                    OP_FIELD: OP_LOGIN,
                    CLIENT_TYPE_FIELD: "python",
                    CLIENT_VERSION_FIELD: EFTL_VERSION,
                    }

                conn_opts = self.connect_options
                url = self.url_list[self.url_index]

                password = url.password
                if password is None:
                    password = conn_opts.get(PASSWORD_PROP, "")
                login_message[PASSWORD_FIELD] = password

                username = url.username
                if username is None:
                    username = conn_opts.get(USERNAME_PROP, conn_opts.get(USER_PROP, ""))
                login_message[USER_FIELD] = username

                conn_opts[QOS_FIELD] = conn_opts.get("qos", "true")#bool(conn_opts.get("qos", True))

                
                if self.client_id is None:
                    client_id = urllib.parse.parse_qs(url.query).get("client_id", [None])[0]
                    if client_id is None:
                        client_id = conn_opts.get(CLIENT_ID_PROP)
                elif self.reconnect_token is not None:
                    client_id = self.client_id
                    login_message[ID_TOKEN_FIELD] = self.reconnect_token
                login_message[CLIENT_ID_FIELD] = client_id

                options = _safe_extend({}, conn_opts)
                if self.is_reconnecting:
                    options[RESUME_FIELD] = "true"

                for opt in [USER_PROP, USERNAME_PROP, PASSWORD_PROP, CLIENT_ID_PROP]:
                    if opt in options:
                        del options[opt]

                login_message[LOGIN_OPTIONS_FIELD] = options

                self.status = OPENING
                self._send(login_message, 0, False)

                # Set up a timeout to be cancelled in _handle_welcome()
                self.welcome_timer = threading.Timer(self.login_timeout, self._dc_without_welcome)

            def _dc_without_welcome(self):
                self.sendClose(code=WS_NORMAL_CLOSE)

            def onMessage(self, payload, is_binary=False):
                msg = json.loads(payload.decode('utf8'))
                logger.debug("Received message:\n{}".format(msg))

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
                    self._handle_event(msg)
                elif op_code == OP_ERROR:
                    self._handle_error(msg)
                elif op_code == OP_ACK:
                    self._handle_ack(msg)
                elif op_code == OP_MAP_RESPONSE:
                    self._handle_map_response(msg)
                else:
                    logger.warning("Received unknown op code ({}):\n{}".format(op_code, msg))

            def onClose(self, was_clean, code, reason="connection failed"):
                logger.warning("Websocket closed. Reason: {} Code: {}".format(reason, code))

                if self.status == CLOSED:
                    return

                self.status = CLOSED

                if self.welcome_timer:
                    self.welcome_timer.cancel()
                    self.welcome_timer = None

                if self.timeout_check:
                    self.timeout_check.cancel()
                    self.timeout_check = None

                if was_clean or code != WS_ABNORMAL_CLOSE or not self._schedule_reconnect():
                    self.factory.conn._ws = None
                    self.got_login_reply = False
                    self._clear_pending(ERR_PUBLISH_FAILED, "Closed")
                    _call_dict_function(
                        self.connect_options,
                        "on_disconnect",
                        connection=self, code=code, reason=reason
                        )

            def _clean_up(self, code, reason):
                self.factory.conn._ws = None
                self.got_login_reply = False
                self._clear_pending(ERR_PUBLISH_FAILED, "Closed")
                _call_dict_function(
                    self.connect_options,
                    "on_disconnect",
                    connection=self, code=code, reason=reason
                    )

            ##################### Protocol message handlers ##########################

            def _handle_welcome(self, message):
                previously_connected = self.reconnect_token is not None
                resume = str(message.get(RESUME_FIELD, "")).lower() == "true"
                self.client_id = message.get(CLIENT_ID_FIELD)
                self.reconnect_token = message.get(ID_TOKEN_FIELD)
                self.timeout = message.get(TIMEOUT_FIELD)
                self.heartbeat = message.get(HEARTBEAT_FIELD)
                self.max_message_size = message.get(MAX_SIZE_FIELD)

                if not resume:
                    self._clear_pending(ERR_PUBLISH_FAILED, "Reconnect")
                    self.factory.last_sequence_number = 0

                self.qos = str(message.get(QOS_FIELD, "")).lower() == "true"

                if self.heartbeat > 0:
                    self.timeout_check = threading.Timer(
                        self.heartbeat,
                        self._do_timeout_check)

                self.status = OPEN
                self.reconnect_timer = 0
                self.url_index = 0

                for k, v in self.subscriptions:
                    if v.get("pending") or not resume:
                        v["pending"] = True
                        self._subscribe(k, v.get("options"))

                self._send_pending()

                if not self.is_reconnecting:
                    func_name = "on_reconnect" if previously_connected else "on_connect"
                    _call_dict_function(self.connect_options, func_name, connection=self)

                self.is_reconnecting = False
                self.got_login_reply = True
                self.welcome_timer.cancel()

            def _do_timeout_check(self):
                now = datetime.datetime.now()
                diff = datetime.datetime.now() - self.last_message

                if diff > self.timeout:
                    logger.debug("last_message in timer, ", {
                        "id": self.client_id,
                        "now": now,
                        "last_message": self.last_message,
                        "timeout": self.timeout,
                        "diff": diff,
                        "evaluate": (diff > self.timeout)
                        })

                    self._close("Connection timeout", False)


            def _subscribe(self, sub_id, timeout, **kwargs):
                subscription = {
                    "options": kwargs,
                    "id": sub_id,
                    "pending": True,
                    }
                self.subscriptions[sub_id] = subscription

                sub_message = {
                    OP_FIELD: OP_SUBSCRIBE,
                    ID_FIELD: sub_id,
                    }
                sub_message = _safe_extend(sub_message, kwargs)

                sub_matcher = kwargs.get(MATCHER_FIELD)
                if sub_matcher:
                    sub_message[MATCHER_FIELD] = sub_matcher
                sub_durable = kwargs.get(DURABLE_FIELD)
                if sub_durable:
                    sub_message[DURABLE_FIELD] = sub_durable
                
                self._send(sub_message, 0, False)
                self._halt_until_signal(timeout)
                return sub_id

            def _next_subscription_sequence(self):
                self.subscription_counter += 1
                return self.subscription_counter


            def _handle_subscribed(self, message):
                sub_id = message.get(ID_FIELD)

                sub = self.subscriptions.get(sub_id)
                if sub and sub.get("pending") is not None:
                    sub["pending"] = False
                    _call_dict_function(sub.get("options"), "on_subscribe", id=sub_id)

                self.factory.conn.halt = False


            def _unsubscribe(self, sub_id, timeout):
                unsub_message = {
                    OP_FIELD: OP_UNSUBSCRIBE,
                    ID_FIELD: sub_id,
                    }

                self._send(unsub_message, 0, False)
                if sub_id in self.subscriptions:
                    del self.subscriptions[sub_id]

                self._halt_until_signal(timeout)

            def _unsubscribe_all(self, timeout):
                for k in self.subscriptions:
                    self._unsubscribe(k, timeout)

            def _handle_unsubscribed(self, message):
                sub_id = message.get(ID_FIELD)

                sub = self.subscriptions.get(sub_id)
                if sub is not None:
                    del self.subscriptions[sub_id]
                    err_code = message.get(ERR_CODE_FIELD)
                    reason = message.get(REASON_FIELD)
                    _call_dict_function(
                        sub.get("options"),
                        "on_error",
                        id=sub_id, code=err_code, reason=reason)
                self.factory.conn.halt = False


            def _publish(self, message, timeout, options):
                envelope = {
                    OP_FIELD: OP_MESSAGE,
                    BODY_FIELD: message,
                    }

                sequence = self._next_sequence()
                if self.qos:
                    envelope[SEQ_NUM_FIELD] = sequence

                publish = {
                    "options": options,
                    "message": message,
                    "envelope": envelope,
                    "sequence": sequence,
                    }

                self._send_request(publish, sequence, timeout, verify_size=True)

            def _send_request(self, request, sequence, timeout, verify_size=False):
                envelope = request.get("envelope")
                data = self.serialize(envelope)
                json.loads(data)  # Test if this is valid JSON

                if verify_size and self.max_message_size > 0 and len(data) > self.max_message_size:
                    raise ValueError("Message exceeds max size ({})".format(self.max_message_size))

                self.requests[sequence] = request

                if self.status == OPEN:
                    self._send(envelope, sequence, False, data)
                    self._halt_until_signal(timeout)

            def _next_sequence(self):
                self.sequence_counter += 1
                return self.sequence_counter


            def _handle_ack(self, message):
                sequence = message.get(SEQ_NUM_FIELD)
                if sequence is not None:
                    err_code = message.get(ERR_CODE_FIELD)
                    if not err_code:
                        self._pending_complete(sequence)
                    else:
                        reason = message.get(REASON_FIELD)
                        self._pending_error(sequence, err_code, reason)
                self.factory.conn.halt = False


            def _handle_map_response(self, message):
                sequence = message.get(SEQ_NUM_FIELD)
                if sequence is not None:
                    err_code = message.get(ERR_CODE_FIELD)
                    if err_code is None:
                        value = message.get(VALUE_FIELD)
                        if value is None:
                            self._pending_complete(sequence)
                        else:
                            self._pending_response(sequence, value)
                    else:
                        reason = message.get(REASON_FIELD)
                        self._pending_error(sequence, err_code, reason)
                self.factory.conn.halt = False


            def _handle_event(self, message):
                seq_num = message.get(SEQ_NUM_FIELD)
                if not self.qos or not seq_num or seq_num > self.last_sequence_number:
                    sub = self.subscriptions.get(message.get(TO_FIELD))
                    if sub and sub.get("options") is not None:
                        data = message.get(BODY_FIELD)
                        _call_dict_function(sub.get("options"), "on_message", message=data)

                    if self.qos:
                        self.last_sequence_number = seq_num

                self._ack(seq_num)

            def _ack(self, seq_num):
                if not self.qos or not seq_num:
                    return

                envelope = {
                    OP_FIELD: OP_ACK,
                    SEQ_NUM_FIELD: seq_num,
                    }
                self._send(envelope, 0, False)


            def _handle_error(self, message):
                err_code = message.get(ERR_CODE_FIELD)
                reason = message.get(REASON_FIELD)

                _call_dict_function(
                    self.connect_options,
                    "on_error",
                    connection=self, code=err_code, reason=reason
                    )

                if self.status == OPENING:
                    self._close(reason, False)
                self.factory.conn.halt = False


            def _halt_until_signal(self, timeout="use default"):
                if timeout == "use default":
                    timeout = self.default_response_timeout
                interval = self.connect_options.get("polling_interval", 0.2)
                self.factory.conn._halt_until_signal(timeout, interval)

            #################### Buffering and sending messages ######################

            def _send(self, message, seq_num, force_send, stringified=None):
                data = stringified if stringified else self.serialize(message)
                data = data.encode('utf8')
                if self.status < CLOSING or force_send:
                    try:
                        logger.debug("Sending message: {}".format(data))
                        self.sendMessage(data)

                        if not self.qos and seq_num > 0:
                            self._pending_complete(seq_num)
                    except Exception as e:
                        logger.error("Exception during send:\n{}".format(e))
                        raise

            def _send_pending(self):
                for k in sorted(self.requests):
                    req = self.requests[k]
                    self._send(req["envelope"], req["sequence"], False)

            def _clear_pending(self, err_code, reason):
                for k in sorted(self.requests):
                    req = self.requests[k]
                    if req is not None:
                        _call_dict_function(
                            req.get("options"),
                            "on_error",
                            message=req["message"], code=err_code, reason=reason
                            )
                    del self.requests[k]

            def _pending_complete(self, sequence):
                req = self.requests.get(sequence)
                if req is not None:
                    del self.requests[sequence]
                    if "options" in req:
                        key = req.get("envelope", {}).get(KEY_FIELD)
                        _call_dict_function(
                            req.get("options"),
                            "on_complete",
                            message=req.get("message"), key=key,
                            )

            def _pending_response(self, sequence, response):
                req = self.requests.get(sequence)
                if req is not None:
                    del self.requests[sequence]
                    if "options" in req:
                        key = req.get("envelope", {}).get(KEY_FIELD)
                        _call_dict_function(
                            req.get("options"),
                            "on_complete",
                            message=response,
                            key=key)

            def _pending_error(self, sequence, err_code, reason):
                req = self.requests.get(sequence)
                if req is not None:
                    del self.requests[sequence]
                    if "options" in req:
                        key = req.get("envelope", {}).get(KEY_FIELD)
                        _call_dict_function(
                            req.get("options"),
                            "on_error",
                            message=req.get("message"), code=err_code, reason=reason, key=key,
                            )
                    else:
                        _call_dict_function(
                            self.connect_options,
                            "on_error",
                            connection=self, code=err_code, reason=reason
                            )

            ############################ Disconnecting ###############################

            def _disconnect(self):
                if self.reconnect_timer:
                    self.reconnect_timer.cancel()
                    self.reconnect_timer = None
                    self._clear_pending(ERR_PUBLISH_FAILED, "Disconnected")
                    _call_dict_function(
                        self.connect_options,
                        "on_disconnect",
                        connection=self, code=WS_NORMAL_CLOSE, reason="Disconnect")

                self._close("Connection closed by user.", True)

            def _close(self, reason="User Action", notify_server=False):
                if self.status != OPENING and self.status != OPEN:
                    return
                self.status = CLOSING

                if self.timeout_check:
                    self.timeout_check.cancel()
                    self.timeout_check = None

                if notify_server:
                    dc_message = {
                        OP_FIELD: OP_DISCONNECT,
                        REASON_FIELD: reason,
                        }
                    self._send(dc_message, 0, True)

                self.sendClose(WS_NORMAL_CLOSE)

            def _schedule_reconnect(self):
                reconn_delay = None

                if self.reconnect_counter < self.auto_reconnect_attempts:
                    reconn_delay = 0
                    if not self._next_url():
                        reconn_delay = min(
                            (2 ** self.reconnect_counter),
                            self.auto_reconnect_max_delay / 1000
                            )
                        self.reconnect_counter += 1

                if reconn_delay is None:
                    return False

                self.reconnect_timer = threading.Timer(
                    reconn_delay,
                    self._try_reconnect()
                    )
                return True

            def _next_url(self):
                self.url_index += 1
                if self.url_index < len(self.url_list):
                    return True
                else:
                    self.url_index = 0
                    return False

            def _try_reconnect(self):
                self.reconnect_timer = None
                self.is_reconnecting = True
                try:
                    self.onOpen()
                except concurrent.futures._base.TimeoutError:
                    logger.warning("Connection attempt timed out")

        return _EftlClientProtocol
