'''
 Copyright (c) 2013-2020 TIBCO Software Inc.
 All Rights Reserved.
 For more information, please contact:
  TIBCO Software Inc., Palo Alto, California, USA
'''

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
        Usage: python3 kvremove.py [options] url
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

    print("kvremove.py: " + eFTL.get_version())


# internal disconnect function
async def _disconnect():
    global connection

    if connection.is_connected():
        await connection.disconnect()

# on_success callback invoked an a successful get
async def on_success_remove_key(message,key):
    print("Success removing key-value pair for key %s" % key)

    #cleanup connection.
    await _disconnect()

# on_error_map_get callback invoked if get fails
async def on_error_map_remove(message, code, reason, key):
    global error

    print("Error getting key-value pair: %s\n", reason);

    #Asynchronously disconnect from the eFTL.
    error = True
    cleanup()

# on_connect callback called on a successful connect
async def on_connect(connection):
    global map
    print("Connected to eFTL server: %s" % url)
    print("Waiting to remove key-value from map..")

    # create the map
    map = await connection.create_kv_map(mapName)
        
    await map.remove(keyName, 
                     on_success=on_success_remove_key,
                     on_error=on_error_map_remove)

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

