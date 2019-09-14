#!/usr/bin/env python

import os
import time
import warnings
warnings.filterwarnings("ignore", category=DeprecationWarning)

use_twisted = os.environ.get("TIBCO_TEST_TWISTED")
if not use_twisted or use_twisted == "False" or use_twisted == "0":
    from eftl.asyncio.connection import EftlConnection
else:
    from eftl.twisted.connection import EftlConnection

# Set environment variables TIBCO_TEST_URL and TIBCO_TEST_PASSWORD to test your desired FTL server.
url = os.environ.get("TIBCO_TEST_URL", "ws://localhost:9191/channel")
password = os.environ.get("TIBCO_TEST_PASSWORD", "")


def print_message(**kwargs):
    message_content = kwargs.get("message", {}).get("demo")
    if message_content:
        print("Received message: {}".format(message_content))

ec = EftlConnection()
ec.connect(
    url,
    client_id='python-demo-receiver',
    password=password,
    )
print("Ready to recieve messages...")

ec.subscribe(
    matcher="""{"demo":true}""",
    on_message=print_message,
    )
try:
    ec.run_event_loop()
except KeyboardInterrupt:
    print("Keyboard interrupt issued; now exiting...")