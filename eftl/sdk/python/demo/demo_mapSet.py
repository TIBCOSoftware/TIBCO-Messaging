#!/usr/bin/env python

import asyncio
import os
import sys
import warnings
from pathlib import Path
warnings.filterwarnings("ignore", category=DeprecationWarning)
import datetime
from messaging.eftl.connection import Eftl

class demo_mapSet:
    def __init__(self, loop):
        self.loop = loop
        self.ec = None

        # Set environment variables TIBCO_TEST_URL and TIBCO_TEST_PASSWORD to test your desired FTL server.
        
        # for map, use correct channel name "wss://localhost:9191/map"
        #self.url = os.environ.get("TIBCO_TEST_URL", "wss://localhost:9191/channel")
        #self.password = os.environ.get("TIBCO_TEST_PASSWORD", "")

        self.url = "ws://localhost:9191/map"
        self.password = 'admin-pw'

    def onDisconnect(self, **kwargs):
        print("disconnected from eftlServer")
        self.loop.stop()

    async def run(self):
        eftl =Eftl()

        print(eftl.get_version())

        self.ec = await eftl.connect(
            self.url,
            client_id='python-demo-sender',
            event_loop=self.loop,
            username='admin',
            password=self.password,
            on_disconnect = self.onDisconnect,
            trust_all=False,
            trust_store=Path("/Users/nsalwantibco.com/ftlnew/build/install/bin/ftl-trust.pem")
        )

        try:
            msg = self.ec.create_message()
            msg.set_long("demo", 345)
         
            sub_msg1 = self.ec.create_message()
            sub_msg1.set_double_array("demo1", [34.5, 78.9])
         
            sub_msg2 = self.ec.create_message()
            sub_msg2.set_datetime("demo2", datetime.datetime.now())
         
            sub_msg1.set_message("sub_msg2", sub_msg2)

            msg.set_message("sub_msg1", sub_msg1)

            sub_msg3 = self.ec.create_message()   
            list1 = [msg, sub_msg1]
            sub_msg3.set_message_array("array of messages", list1)
                
            map1 = await self.ec.create_kv_map("map1")
            await map1.set("map_field", sub_msg3)

            print("map 'map1' has been set")
            await self.ec.disconnect()
        except (EOFError, KeyboardInterrupt):
            print("\nKeyboard interrupt issued; now exiting...")
            await self.ec.disconnect()

if __name__ == '__main__':
    loop = asyncio.get_event_loop()
    map_setter = demo_mapSet(loop)
    loop.run_until_complete(map_setter.run())
    loop.run_forever()
    loop.close()
