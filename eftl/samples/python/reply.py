'''
 Copyright (c) 2013-2022 Cloud Software Group, Inc.
 All Rights Reserved.
'''

import asyncio
import getopt, sys, time
from messaging.eftl.connection import Eftl

user       = "user"
password   = "password"
url        = "ws://localhost:8585/channel"
connection = None
loop       = None
eFTL       = Eftl()
error      = False
requestTimeout = 60 * 60  # wait for 1 hour for reply (60 * 60 seconds)

def usage():
    print (usage_lines)
    sys.exit(0)

usage_lines = """
        Usage: python3 request.py [options] url
        Options:
        \t-user        <str>    Username for authentication
        \t-password    <str>    Password for authentication
      """

def parseArgs(argv):
    count = len(argv)
    i = 1

    while i < count:
        if (argv[i].lower() == '-user'):
            global user
            i += 1
            user = argv[i]
        elif (argv[i].lower() == "-password"):
            global password
            i += 1
            password = argv[i]
        elif (argv[i].lower() == "-help") or (argv[i].lower() == "-h"):
            usage()
        else:
           global url
           url = argv[i]

        i += 1

    print("reply.py: " + eFTL.get_version())


# internal disconnect function
async def _disconnect():
    global connection

    if connection.is_connected():
        await connection.disconnect()

async def on_reply_complete(message):
    global connection

    #Asynchronously disconnect from the eFTL.
    if connection.is_connected():
        await _disconnect()

async def on_reply_error(message, err_code, reason):
    printUtil("Error code = %s and Reason = %s" % (err_code, reason))

    #Asynchronously disconnect from the eFTL.
    if connection.is_connected():
        await _disconnect()

async def on_message(message):
    global error

    print("Request message: %s" % message)

    reply = connection.create_message()
    reply.set_string("type", "reply");
    reply.set_string("text", "This is a sample eFTL reply message");

    # send the request 
    await connection.send_reply(message,
                                reply,
                                on_complete=on_reply_complete,
                                on_error=on_reply_error)
    print("Sent reply: %s" % reply)


async def on_subscribe(id):
    print("Subscribed to receive request messages")
    print("waiting for requests to handle..")

async def on_subscribe_error(message, code, reason):
    print("Subscribe error: %s" % reason)

    #Disconnect from the server.
    if connection.is_connected():
        await _disconnect()


# connection callback on_connect called on a successful connect
async def on_connect(connection):
    print("Connected to eFTL server: %s" % url)

    matcher = "{\"type\": \"request\"}"
    await connection.subscribe(matcher=matcher,
                               on_subscribe=on_subscribe,
                               on_error=on_subscribe_error,
                               on_message=on_message)


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
        print("Initiated connect()..")

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

