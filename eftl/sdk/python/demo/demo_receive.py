#!/usr/bin/env python

import asyncio
import os
import time
import warnings
import sys
from pathlib import Path
warnings.filterwarnings("ignore", category=DeprecationWarning)

use_twisted = os.environ.get("TIBCO_TEST_TWISTED")
from messaging.eftl.connection import Eftl

class demo_receive:
    def __init__(self,loop):
        # Set environment variables TIBCO_TEST_URL and TIBCO_TEST_PASSWORD to test your desired FTL server.
        # for map, use correct channel name "wss://localhost:9191/map"
        self.url = os.environ.get("TIBCO_TEST_URL", "wss://localhost:9191/channel")
        self.password = os.environ.get("TIBCO_TEST_PASSWORD", "")

        self.loop = loop
        self.ec = None
   
    async def onConnect(self, **kwargs):
        print("connection has been estabished")

    async def onDisconnect(self, **kwargs):
        print("onClose has been executed fully")
        self.loop.stop()

    async def on_message(self, message):
        print("in on_message callback")
        print(message)
         
    async def run(self):
        eftl = Eftl()
        print(eftl.get_version())
        
        try:        
            self.ec = await eftl.connect(
                "ws://localhost:9191/channel|ws://localhost:9192/channel",
                client_id='python-demo-receiver',
                event_loop=self.loop,
                username='admin',
                password='admin-pw',
                on_connect=self.onConnect,
                on_disconnect=self.onDisconnect,
                trust_all=False,
                trust_store=Path("/Users/nsalwantibco.com/ftlnew/build/install/bin/ftl-trust.pem")
                )
            print("ready to recieve messages")
        
            await self.ec.subscribe(matcher="""{}""", on_message=self.on_message)
        except (EOFError, KeyboardInterrupt):
            print("\nKeyboard interrupt issued; now exiting...")
            self.loop.close()
            sys.exit(-1)

if __name__ == '__main__':
    loop = asyncio.get_event_loop()
    receiver = demo_receive(loop)
    try:        
        loop.run_until_complete(receiver.run())
        loop.run_forever()
        loop.close()
    except (EOFError, KeyboardInterrupt):
        print("\nKeyboard interrupt issued; now exiting ...")
        sys.exit(-1)
