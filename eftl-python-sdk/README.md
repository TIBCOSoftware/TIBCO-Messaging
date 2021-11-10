## API Overview
The TIBCO eFTL API is a compact version of the TIBCO FTL API, small enough for mobile applications and browser applications. It includes everything you need to exchange messages, and it omits everything else.

## Functionality
Programs use the API for these operations:

* Connecting to an eFTL service
* Publishing messages (one-to-many)
* Subscribing to messages (one-to-many) and filtering with content matchers
* Constructing and unpacking messages

## Asynchronous Callback Architecture
API calls return quickly, which reflects a requirement of mobile device applications. All API calls that depend on a TIBCO eFTL service are asynchronous. When the service responds, the TIBCO eFTL library invokes a user-defined callback method.

## Message Data Types
The field data types within eFTL messages are similar to those in FTL messages, except that eFTL messages do not support inbox types.

However, the field data types within eFTL messages are not similar to those in EMS messages. For details of message translation, see “Message Translation: TIBCO eFTL and TIBCO EMS” in TIBCO eFTL Concepts (https://docs.tibco.com/pub/eftl/6.6.0/doc/pdf/TIB_eftl_6.6_concepts.pdf).

## Installation
The eFTL python library is hosted on PyPI and can be installed with `pip3 install eftl`

## Documentation
https://docs.tibco.com/pub/eftl/6.6.0/doc/html/api-reference/python/connection.m.html

## Prerequisites
Please make sure that you have the FTL Server running with an eFTL service.
By default, the following sample programs assume that server is running at localhost:8585.

For more information, see 
https://docs.tibco.com/products/tibco-ftl-enterprise-edition-6-6-0 (FTL), and
https://docs.tibco.com/products/tibco-eftl-enterprise-edition-6-6-0 (eFTL).

## Sample Programs

# Publisher

A publisher could look like this:

```
import asyncio
import getopt, sys, time
import datetime
from messaging.eftl.connection import Eftl

user       = "user"
password   = "password"
url        = "ws://localhost:8585/channel"
connection = None
loop       = None
sendCount  = 10
rate       = 1
eFTL       = Eftl()
error      = False
sent       = 0

def usage():
    print (usage_lines)
    sys.exit(0)

usage_lines = """
        Usage: python3 publisher.py [options] url
        Options:
        \t-user        <str>    Username for authentication
        \t-password    <str>    Password for authentication
        \t-count       <int>    Number of message to publish
        \t-rate        <int>    Number of message per second
      """

def parseArgs(argv):
    count = len(argv)
    i = 1

    while i < count:
        if (argv[i].lower() == '-user'):
            global user
            user = argv[i+1]
        elif (argv[i].lower() == "-password"):
            global password
            password = argv[i+1]
        elif (argv[i].lower() == "-count"):
            global sendCount
            sendCount = int(argv[i+1])
        elif (argv[i].lower() == "-rate"):
            global rate
            rate = int(argv[i+1])
        elif (argv[i].lower() == "-help") or (argv[i].lower() == "-h"):
            usage()
        else:
           global url
           url = argv[i]

        i += 1

# internal disconnect function
async def _disconnect():
    global connection

    if connection.is_connected():
        await connection.disconnect()

async def on_publish_complete(message):
    global sent
    print("Successfully Sent message [%s]: {%s}" % (sent, message))
    sent += 1

    if sent == sendCount:
        done = True
        print("Total messages sent: %s" % sent)

        if connection.is_connected():
            await _disconnect()

async def on_publish_error(message, code, reason):
    print("Failed to send message: %s, code: %s, reason: %s" %(message, code, reason))

    if connection.is_connected():
        await _disconnect()


# connection callback on_connect called on a successful connect
async def on_connect(connection):
    print("Connected to eFTL server: %s" % url)

# connection callback on_reconnect 
async def on_reconnect(connection):
    print("Reconnect attempt: ")

# connection callback on_error called if an error on the connection 
async def on_error(connection, code, reason):
    global error
    print("Connection error [%s], reason %s" %(code, reason))
    error = True

# connection callback on_disconnect called upon a disconnect from eFTL
async def on_disconnect(connection, loop, code, reason):
    print("Disconnected [%s]: %s" %(code, reason))

    loop.stop()

async def run():
    global connection
    global sendCount

    try:
        # connect to the eftl 
        connection = await eFTL.connect(url,
                                    user=user,
                                    password=password,
                                    trust_all=True,
                                    event_loop=loop,
                                    on_connect=on_connect,
                                    on_reconnect=on_reconnect,
                                    on_disconnect=on_disconnect,
                                    on_error=on_error)

        for i in range(sendCount):
            msg = connection.create_message()
            msg.set_string("type", "hello")
            msg.set_string("text", "This is a sample eFTL message")
            msg.set_long("long", i)
            msg.set_datetime("time", datetime.datetime.now())

            await connection.publish(msg,
                                     on_complete=on_publish_complete,
                                     on_error=on_publish_error)

            if rate > 0:
                await asyncio.sleep(float(1/rate))
            else:
                await asyncio.sleep(1)

    finally:
        print("Done..")

def cleanup():
    global loop
    global connection

    for task in asyncio.Task.all_tasks():
        task.cancel()

    if connection.is_connected():
        loop.run_until_complete(_disconnect())

    loop.stop()
    loop.close()

def main(argv):
    errors = None
    global loop

    parseArgs(argv)

    try:
        loop = asyncio.get_event_loop();
        loop.run_until_complete(run())
        loop.run_forever()
    except (EOFError, KeyboardInterrupt):
        print("Disconnected from eFTL server: %s" % url)
    except Exception as err:
        errors = True
    finally:
           cleanup()
           if errors:
               sys.exit(1)
           else:
               sys.exit(0)

if __name__ == "__main__":
    main(sys.argv)

```

# Subscriber

A sample subscriber would like :

```
import asyncio
import getopt, sys, time
import datetime
import threading
from messaging.eftl.connection import Eftl

user       = "user"
password   = "password"
url        = "ws://localhost:8585/channel"
connection = None
loop       = None
eFTL       = Eftl()
error      = False
clientId   = "sample-python-client"
received   = 0
durName    = "sample-durable"
clientAck  = False
ackMode    = "auto"
timer      = None
subscriberId = None

def usage():
    print (usage_lines)
    sys.exit(0)

usage_lines = """
        Usage: python3 subscriber.py [options] url
        Options:
        \t-user        <str>    Username for authentication
        \t-password    <str>    Password for authentication
        \t-clientId    <str>    unique client id
        \t-name        <str>    durable name
        \t-clientAcknowledge    Flag to indicate that client ack
      """

def parseArgs(argv):
    count = len(argv)
    i = 1

    while i < count:
        if (argv[i].lower() == '-user'):
            global user
            user = argv[i+1]
        elif (argv[i].lower() == "-password"):
            global password
            password = argv[i+1]
        elif (argv[i].lower() == "-clientid"):
            global clientId
            clientId = argv[i+1]
        elif (argv[i].lower() == "-name"):
            global durName
            durName = argv[i+1]
        elif (argv[i].lower() == "-clientacknowledge"):
            global clientAck
            clientAck = True
        elif (argv[i].lower() == "-help") or (argv[i].lower() == "-h"):
            usage()
        else:
           global url
           url = argv[i]

        i += 1


# internal disconnect function
async def _disconnect():
    global connection

    if connection.is_connected():
        await connection.disconnect()


async def _unsubscribe(id):
    global connection

    await connection.unsubscribe(id)


def on_recv_timeout():
    global timer
    global subscriberId
    event_loop = None

    event_loop = asyncio.new_event_loop()

    timer.cancel()
    event_loop.run_until_complete(_unsubscribe(subscriberId))
    event_loop.run_until_complete(_disconnect())

async def on_subscribe(id):
    global timer
    global subscriberId

    subscriberId = id
    print("Successfully subscribed to receive messages")

    timer = threading.Timer(30.0, on_recv_timeout)
    timer.start()

async def on_subscribe_error(code, reason):
    print("Failed to subscribe, code: %s, reason: %s" %(code, reason))

    if connection.is_connected():
        await _disconnect()

async def on_message(message):
    global received
    global clientAck

    print("Received message: [%s] %s" %(received, message))
    received += 1

    # if client ack mode, then acknowledge the message
    if clientAck and message is not None:
        await connection.acknowledge(message)


# on_connect callback called on a successful connect
async def on_connect(connection):
    global clientAck
    global ackMode
    print("Connected to eFTL server: %s" % url)

    if clientAck:
        ackMode = "client"

    # subscribe to the matcher, once connected
    matcher = "{\"type\":\"hello\"}"
    await connection.subscribe(matcher=matcher,
                              ack=ackMode,
                              durable=durName,
                              on_subscribe=on_subscribe,
                              on_error=on_subscribe_error,
                              on_message=on_message)

# on_reconnect callback
async def on_reconnect(connection):
    print("Reconnect attempt: ")

# on_error callback called if an error on the connection 
async def on_error(connection, code, reason):
    global error
    print("Connection error [%s], reason %s" %(code, reason))
    error = True

# on_disconnect callback called upon a disconnect from eFTL
async def on_disconnect(connection, loop, code, reason):
    print("Disconnected [%s]: %s" %(code, reason))

    loop.stop()

async def run():
    global connection
    global sendCount
    global clientId

    try:
        # connect to the eftl 
        connection = await eFTL.connect(url,
                                    user=user,
                                    password=password,
                                    client_id=clientId,
                                    trust_all=True,
                                    event_loop=loop,
                                    on_connect=on_connect,
                                    on_reconnect=on_reconnect,
                                    on_disconnect=on_disconnect,
                                    on_error=on_error)

    finally:
        print("Waiting for messages..")

def cleanup():
    global loop
    global connection
    global subscriberId

    if connection.is_connected():
        loop.run_until_complete(_disconnect())

    loop.stop()
    loop.close()

def main(argv):
    errors = None
    global loop

    parseArgs(argv)

    try:
        loop = asyncio.get_event_loop();
        loop.run_until_complete(run())
        loop.run_forever()
    except (EOFError, KeyboardInterrupt):
        print("Disconnected from eFTL server: %s" % url)
    except Exception as err:
        errors = True
    finally:
           cleanup()
           if errors:
               sys.exit(1)
           else:
               sys.exit(0)

if __name__ == "__main__":
    main(sys.argv)
```

# Key/Value map

You may also create a key/value map associated with your EftlConnection. Once created, you can get, set, and remove key/value mappings:

Following is the sample to set key/value pair:

```
import asyncio
import getopt, sys, time
import datetime
from messaging.eftl.connection import Eftl

user       = "user"
password   = "password"
mapName    = "sample_map"
keyName    = "key1"
url        = "ws://localhost:8585/map"
connection = None
loop       = None
text       = "This is sample eFTL text"
eFTL       = Eftl()

def usage():
    print (usage_lines)
    sys.exit(0)

usage_lines = """
        Usage: python3 kvset.py [options] url
        Options:
        \t-user        <str>    Username for authentication
        \t-password    <str>    Password for authentication
        \t-map         <str>    Map name where we want to the key-value to be stored
        \t-key         <str>    The name of the key
        \t-text        <str>    The value associated with the key
      """

def parseArgs(argv):
    count = len(argv)
    i = 1

    while i < count:
        if (argv[i].lower() == '-user'):
            global user
            user = argv[i+1]
        elif (argv[i].lower() == "-password"):
            global password
            password = argv[i+1]
        elif (argv[i].lower() == "-map"):
            global mapName
            mapName = argv[i+1]
        elif (argv[i].lower() == "-key"):
            global keyName
            keyName = argv[i+1]
        elif (argv[i].lower() == "-text"):
            global text
            text = argv[i+1]
        elif (argv[i].lower() == "-help") or (argv[i].lower() == "-h"):
            usage()
        else:
           global url
           url = argv[i]

        i += 1

# internal disconnect function
async def _disconnect():
    global connection

    if connection.is_connected():
        await connection.disconnect()

# on_success callback invoked an a successful get
async def on_success_set_key(message,key):
    print("Sucess setting key-value pair %s = %s" % (key, message))

    #Asynchronously disconnect from the eFTL.
    if connection.is_connected():
        await _disconnect()

# on_error_map_get callback invoked if get fails
async def on_error_map_set(key, message, code, reason):
    print("Error setting key-value pair: %s\n", reason);

    #Asynchronously disconnect from the eFTL.
    cleanup()

# on_connect callback called on a successful connect
async def on_connect(connection):
    print("Connected to eFTL server: %s" % url)

# on_reconnect callback
async def on_reconnect(connection):
    print("Reconnect attempt: ")

# on_error callback called if an error on the connection 
async def on_error(connection, code, reason):
    print("Connection error [%s], reason %s" %(code, reason))

# on_disconnect callback called upon a disconnect from eFTL
async def on_disconnect(connection, loop, code, reason):
    print("Disconnected [%s]: %s" %(code, reason))
    loop.stop()

async def run():
    global connection

    try:
        # connect to the eftl 
        connection = await eFTL.connect(url,
                                    user=user,
                                    password=password,
                                    trust_all=True,
                                    event_loop=loop,
                                    on_connect=on_connect,
                                    on_reconnect=on_reconnect,
                                    on_disconnect=on_disconnect,
                                    on_error=on_error)

        # create the map
        map = await connection.create_kv_map(mapName)

        msg = connection.create_message()
        msg.set_string("text", text)
        msg.set_long("long", 101)
        msg.set_datetime("time", datetime.datetime.now())

        # set the key-value pair in the map object
        await map.set(keyName,
                      msg,
                      on_success=on_success_set_key,
                      on_error=on_error_map_set)
    finally:
        print("Done..")

def cleanup():
    global loop
    global connection

    if connection.is_connected():
        loop.run_until_complete(_disconnect())

    loop.stop()
    loop.close()

def main(argv):
    errors = None
    global loop

    parseArgs(argv)

    try:
        loop = asyncio.get_event_loop();
        loop.run_until_complete(run())
        loop.run_forever()
    except (EOFError, KeyboardInterrupt):
        print("Disconnected from eFTL server: %s" % url)
    except Exception as err:
        errors = True
    finally:
           cleanup()
           if errors:
               sys.exit(1)
           else:
               sys.exit(0)

if __name__ == "__main__":
    main(sys.argv)
 
```

Sample to get key/value pair:

```
import asyncio
import getopt, sys, time
from messaging.eftl.connection import Eftl

user       = "user"
password   = "password"
mapName    = "sample_map"
keyName    = "key1"
url        = "ws://localhost:8585/map"
connection = None
loop       = None
eFTL       = Eftl()
error      = False

def usage():
    print (usage_lines)
    sys.exit(0)

usage_lines = """
        Usage: python3 kvget.py [options] url
        Options:
        \t-user        <str>    Username for authentication
        \t-password    <str>    Password for authentication
        \t-map         <str>    Map name from which to retrieve the key
        \t-key         <str>    The key's value to be retrieved
      """

def parseArgs(argv):
    count = len(argv)
    i = 1

    while i < count:
        if (argv[i].lower() == '-user'):
            global user
            user = argv[i+1]
        elif (argv[i].lower() == "-password"):
            global password
            password = argv[i+1]
        elif (argv[i].lower() == "-map"):
            global mapName
            mapName = argv[i+1]
        elif (argv[i].lower() == "-key"):
            global keyName
            keyName = argv[i+1]
        elif (argv[i].lower() == "-help") or (argv[i].lower() == "-h"):
            usage()
        else:
           global url
           url = argv[i]

        i += 1

# internal disconnect function
async def _disconnect():
    global connection

    if connection.is_connected():
        await connection.disconnect()

# on_success callback invoked an a successful get
async def on_success_get_key(message,key):

    if message is not None:
        print("Get map='%s' key='%s' value=%s" % (mapName, key, message))
    else:
        print("Get map='%s' key='%s' value=null" % (mapName, key))

    #cleanup connection.
    await _disconnect()

# on_error_map_get callback invoked if get fails
async def on_error_map_get(message, code, reason, key):
    global error

    print("Error getting key-value pair: %s\n", reason);

    #Asynchronously disconnect from the eFTL.
    error = True
    cleanup()

# on_connect callback called on a successful connect
async def on_connect(connection):
    global map
    print("Connected to eFTL server: %s" % url)

    # create the map
    map = await connection.create_kv_map(mapName)

    await map.get(keyName,
                  on_success=on_success_get_key,
                  on_error=on_error_map_get)

# on_reconnect callback
async def on_reconnect(connection):
    print("Reconnect attempt: ")

# on_error callback
async def on_error(connection, code, reason):
    global error
    print("Connection error [%s], reason %s" %(code, reason))
    error = True

# on_disconnect callback called upon a disconnect from eFTL
async def on_disconnect(connection, loop, code, reason):
    print("Disconnected [%s]: %s" %(code, reason))
    loop.stop()

async def run():
    global connection

    try:
        # connect to the eftl 
        connection = await eFTL.connect(url,
                                    user=user,
                                    password=password,
                                    trust_all=True,
                                    event_loop=loop,
                                    on_connect=on_connect,
                                    on_reconnect=on_reconnect,
                                    on_disconnect=on_disconnect,
                                    on_error=on_error)
    finally:
        print("Waiting to get key-value from map..")

def cleanup():
    global loop
    global connection

    if connection.is_connected():
        loop.run_until_complete(_disconnect())

    loop.stop()
    loop.close()

def main(argv):
    errors = None
    global loop

    # parse args
    parseArgs(argv)

    try:
        loop = asyncio.get_event_loop();
        loop.run_until_complete(run())
        loop.run_forever()
    except (EOFError, KeyboardInterrupt):
        print("Disconnected from eFTL server: %s" % url)
    except Exception as err:
        errors = True
    finally:
           cleanup()
           if errors:
               sys.exit(1)
           else:
               sys.exit(0)

if __name__ == "__main__":
    main(sys.argv)



```
