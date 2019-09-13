import logging


OP_MAP_SET = 20
OP_MAP_GET = 22
OP_MAP_REMOVE = 24
OP_MAP_RESPONSE = 26

OP_FIELD = "op"
MAP_FIELD = "map"
KEY_FIELD = "key"
VALUE_FIELD = "value"
SEQ_NUM_FIELD = "seq"


logger = logging.getLogger(__name__)
logger.addHandler(logging.NullHandler())


class _EftlKVMap:
    """Represents a single Key/Value map stored on the server."""

    def __init__(self, connection, name):
        """Initialize the Key/Value Map, bound to a connection."""
        self.connection = connection
        self.name = name

    def get(self, key, timeout="use default", **kwargs):
        """
        Send a request to get a value based on a key in this map.

        key : str
            Key to retrieve the value of.
        timeout : optional
            Number of seconds to halt waiting on acknowledgement from
            the server (default set in EftlConnection.connect()).
        on_complete, on_error : function, optional
            Function to be run on completion or failure of the delivery.
        """
        if not self.connection._permanently_closed():
            self._create_kv_request(OP_MAP_GET, key, timeout=timeout, **kwargs)

    def set(self, key, value, timeout="use default", **kwargs):
        """
        Send a request to set a key/value pair of this map.

        key : str
            Value of this key.
        value
            Value to associate with this key.
        timeout : optional
            Number of seconds to halt waiting on acknowledgement from
            the server (default set in EftlConnection.connect()).
        on_complete, on_error : function, optional
            Function to be run on completion or failure of the delivery.
        """
        if not self.connection._permanently_closed():
            self._create_kv_request(OP_MAP_SET, key, value, timeout=timeout, **kwargs)

    def remove(self, key, timeout="use default", **kwargs):
        """
        Send a request to remove a key/value pair of this map.

        key : str
            Key to remove.
        timeout : optional
            Number of seconds to halt waiting on acknowledgement from
            the server (default set in EftlConnection.connect()).
        on_complete, on_error : function, optional
            Function to be run on completion or failure of the delivery.
        """
        if not self.connection._permanently_closed():
            self._create_kv_request(OP_MAP_REMOVE, key, timeout=timeout, **kwargs)

    def _create_kv_request(self, op, key, message=None, timeout="use default", **kwargs):
        conn = self.connection._ws

        envelope = {
            OP_FIELD: op,
            MAP_FIELD: self.name,
            KEY_FIELD: key,
            }
        if message:
            envelope[VALUE_FIELD] = message

        sequence = conn._next_sequence()
        if conn.qos or op == OP_MAP_GET:
            envelope[SEQ_NUM_FIELD] = sequence

        request_options = {}
        success = kwargs.get("on_success")
        if success is not None:
            request_options["on_complete"] = success
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

        conn._send_request(request, sequence, timeout, verify_size=(op == OP_MAP_SET))
