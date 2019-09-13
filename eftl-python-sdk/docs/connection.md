
## eftl.asyncio.connection.EftlConnection or eftl.twisted.connection.EftlConnection
A websocket wrapper exchanging eftl protocol messages with a server.

Different functions implement each of the types of protocol messages (e.g. publish, subscribe, disconnect). The messages are sent asynchronously, but by default these functions halt waiting for a reponse. The user may set a timeout for this waiting period if they desire.

Users also may specify callbacks to trigger when messages get delivered, fail to get delivered, etc.

### Methods
***run_event_loop(self, interval=None)***
>Run the current event loop.
>
>**Parameters**
*interval*
Number of seconds to run the loop. If None, run it forever.

***connect(self, url, \*\*kwargs)***
>Try connecting to one of the urls until success or a timeout.
>
>**Parameters**
*url (str)*
Potentially multiple urls separated by the pipe character. The url scheme must be ws or wss, and must have a host name.
>
>*qos (bool, optional)*
Whether or not Quality of Service is enabled.
>
>*auto_reconnect_attempts (int, optional)*
Maximum number of reconnect attempts.
>
>*auto_reconnect_max_delay (optional)*
Maximum reconnect delay in milliseconds.
>
>*username (optional)*
Login credentials to use if not found in the url.
>
>*user (optional)*
Alternative if username is not specified.
>
>*password (optional)*
Login credentials to use if not found in the url.
>
>*client_id (optional)*
>User-specified client identifier.
>
>*handshake_timeout (optional)*
Seconds to wait for websocket handshake to complete.
>
>*login_timeout (optional)*
Seconds to halt waiting for a login message reply. If a reply is not received in time, raise an EftlClientError.
>*response_timeout (optional)*
Seconds to halt waiting for other message replies. If a reply is not received in time, resume normal operation.
>
>*polling_interval (optional)*
Seconds to wait between each message reply check.
>
>*serializer (function, optional)*
Function that must be able to serialize a message (arbitrary python object) to JSON. Default is jsonpickle.encode.
>
>**Raises**
*ValueError*
If any provided urls lack a host name or the correct scheme.
>
>*EftlClientError*
If the connection is not established in time.

***is_connected(self, warn=False)***
>Return whether or not this connection is still open.
>
>**Parameters**
>*warn*
>Whether or not a warning message should be logged.

***disconnect(self)***
>Gracefully disconnect, sending a message to the server.

***subscribe(self, timeout="use default", \*\*kwargs)***
>Send a message to register a matcher-based subscription.
>
>**Parameters**
>*timeout (optional)*
>Number of seconds to halt waiting on acknowledgement from the server (default set in EftlConnection.connect()).
>
>*matcher (str, optional)*
>JSON content matcher to subscribe to.
>
>*type ({"standard", "shared", "last-value"}, optional)*
>Durable type.
>
>*key (optional)*
>The last-value index key, if `type` is "last-value".
>
>*durable (str, optional)*
>Name to give the durable.
>
>**Returns**
>*sub_id*
>The subcription identifier of the new subscription.

***unsubscribe(self, sub_id, timeout="use default")***
>Unsubscribe from the given subscription identifier.
>
>**Parameters**
>*sub_id*
Subscription identifier of the subscription to delete.
>
>*timeout (optional)*
Number of seconds to halt waiting on acknowledgement from the server (default set in EftlConnection.connect()).

***unsubscribe_all(self, timeout="use default")***
>Unsubscribe from all subscriptions.
>
>**Parameters**
>*timeout (optional)*
Number of seconds to halt waiting on acknowledgement from the server (default set in EftlConnection.connect()).

***publish(self, message, timeout="use default", \*\*kwargs)***
>Publish a message to the server.
>
>**Parameters**
>*message*
Object to be serialized to JSON and sent.
>
>*timeout (optional)*
Number of seconds to halt waiting on acknowledgement from the server (default set in EftlConnection.connect()).
>
>*on_complete, on_error (function, optional)*
Function to be run on completion or failure of the delivery.
>
>**Raises**
>*ValueError*
If the message size exceeds the maximum.

***map(self, name)***
>Return a new \_EftlKVMap associated with this connection.