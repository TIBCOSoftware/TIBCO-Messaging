#!/usr/bin/env python

import os
import sys
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


ec = EftlConnection()
ec.connect(
    url,
    client_id='python-demo-sender',
    password=password,
    )

if len(sys.argv) > 1:
    ec.publish({"demo": ' '.join(sys.argv[1:])})
else:
    try:
        while True:
            message_text = input("Enter a message: ")
            ec.publish({"demo": message_text})
    except (EOFError, KeyboardInterrupt):
        print("\nKeyboard interrupt issued; now exiting...")