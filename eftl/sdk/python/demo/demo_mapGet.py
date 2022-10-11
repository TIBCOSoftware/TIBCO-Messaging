#!/usr/bin/env python

import asyncio
import os
import time
import warnings
from pathlib import Path
warnings.filterwarnings("ignore", category=DeprecationWarning)

#use_twisted = os.environ.get("TIBCO_TEST_TWISTED")
from messaging.eftl.connection import Eftl

class demo_mapGet:
    def __init__(self,loop):
        # Set environment variables TIBCO_TEST_URL and TIBCO_TEST_PASSWORD to test your desired FTL server.
        # for map, use correct channel name "wss://localhost:9191/map"
        self.url = os.environ.get("TIBCO_TEST_URL", "wss://localhost:9191/channel")
        self.password = os.environ.get("TIBCO_TEST_PASSWORD", "")

        self.loop = loop
        self.ec = None

    async def onDisconnect(self, **kwargs):
        self.loop.stop()

    def print_message(self, **kwargs):
        print("got a message for you")    
        new_message = kwargs.get("message", {})
        print("whole message is ")
        print(new_message) 

    async def print_map(self, **kwargs):
        print("Got a map value for you")
        new_message = kwargs.get("message", {})
        print("whole message is ")
        print(new_message)
        await self.ec.disconnect()        
         
    async def run(self):
        eftl = Eftl()
        print(eftl.get_version())
        
        self.ec = await eftl.connect(
            "ws://localhost:9191/map|ws://localhost:9191/map",
            client_id='python-demo-receiver',
            event_loop=self.loop,
            username='admin',
            password='admin-pw',
            on_disconnect= self.onDisconnect,
            trust_all=False,
            trust_store=Path("/Users/nsalwantibco.com/ftlnew/build/install/bin/ftl-trust.pem")
            )
        
        map1 = await self.ec.create_kv_map("map1")
        print("Going to get Map value")
        await map1.get("map_field", on_success=self.print_map)
       
        #print("Going ot remove the previously set Key by demo_mapSet.py") 
        #map1.remove("map_field")
        #print("Removed map_field key , now trying to get the key value again")
        #map1.get("map_field", on_success=self.print_map)
        #print("Now, goingto destroy the map")
        #self.ec.remove_kv_map("map1")
        #print("map deleted") 
        #self.ec.disconnect()

if __name__ == '__main__':
    loop = asyncio.get_event_loop()
    map_getter = demo_mapGet(loop)
    loop.run_until_complete(map_getter.run())
    loop.run_forever()
    loop.close()
