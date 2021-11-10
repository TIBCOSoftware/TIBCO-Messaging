'''
 Copyright (c) 2013-2020 TIBCO Software Inc.
 All Rights Reserved.
 For more information, please contact:
  TIBCO Software Inc., Palo Alto, California, USA
'''

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
clientId   = None
received   = 0
durName    = "sample-shared-durable"
ackMode    = "auto"
timer      = None
subscriberId = None

def usage():
    print (usage_lines)
    sys.exit(0)

usage_lines = """
        Usage: python3 shared_durable.py [options] url
        Options:
        \t-user        <str>    Username for authentication
        \t-password    <str>    Password for authentication
        \t-clientId    <str>    unique client id
        \t-name        <str>    durable name
        \t-count       <int>    Number of message to receive
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
            global recvCount
            recvCount = int(argv[i+1])
        elif (argv[i].lower() == "-clientid"):
            global clientId
            clientId = argv[i+1]
        elif (argv[i].lower() == "-name"):
            global durName
            durName = argv[i+1]
        elif (argv[i].lower() == "-help") or (argv[i].lower() == "-h"):
            usage()
        else:
           global url
           url = argv[i]

        i += 1

    print("shared_durable.py: " + eFTL.get_version())


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
    global recvCount
    print("Received message: [%s] %s" %(received, message))
    received += 1


# on_connect callback called on a successful connect
async def on_connect(connection):
    print("Connected to eFTL server: %s" % url)

    # change the matcher to "{}" if you wish to receive all messages
    matcher = "{\"type\":\"hello\"}"

    await connection.subscribe(matcher=matcher,
                              ack=ackMode,
                              durable=durName,
                              type="shared",
                              on_subscribe=on_subscribe,
                              on_error=on_subscribe_error,
                              on_message=on_message)

# on_reconnect callback
async def on_reconnect(connection):
    print("Reconnect attempt: ")

# on_error callback
async def on_error(connection, code, reason):
    global error
    print("Connection error [%s], reason %s" %(code, reason))
    error = True

# on_disconnect callback
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
    finally:
        print("Initiated connect..")
        

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

