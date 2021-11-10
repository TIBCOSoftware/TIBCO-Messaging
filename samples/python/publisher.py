'''
 Copyright (c) 2013-2020 TIBCO Software Inc.
 All Rights Reserved.
 For more information, please contact:
  TIBCO Software Inc., Palo Alto, California, USA
'''

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

    print("publisher.py: " + eFTL.get_version())


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
    except CancelledError:
        logging.info('CancelledError')
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

