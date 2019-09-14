
## eftl.kvmap.\_EftlKVMap
Represents a single key/value map stored on the server.

### Methods
***get(self, key, timeout="use default", \*\*kwargs)***
>Send a request to get a value based on a key in this map.
>
>**Parameters**
*key (str)*
Key to retrieve the value of.
>
>*timeout (optional)*
Number of seconds to halt waiting on acknowledgement from the server (default set in EftlConnection.connect()).
>
>*on_complete, on_error (function, optional)*
Function to be run on completion or failure of the delivery.

***set(self, key, value, timeout="use default", \*\*kwargs)***
>Send a request to set a key/value pair of this map.
>
>**Parameters**
*key (str)*
Value of this key.
>
>*value*
>Value to associate with this key.
>
>*timeout (optional)*
Number of seconds to halt waiting on acknowledgement from the server (default set in EftlConnection.connect()).
>
>*on_complete, on_error (function, optional)*
Function to be run on completion or failure of the delivery.

***remove(self, key, timeout="use default", \*\*kwargs)***
>Send a request to remove a key/value pair of this map.
>
>**Parameters**
*key (str)*
Key to remove.
>
>*timeout (optional)*
Number of seconds to halt waiting on acknowledgement from the server (default set in EftlConnection.connect()).
>
>*on_complete, on_error (function, optional)*
Function to be run on completion or failure of the delivery.