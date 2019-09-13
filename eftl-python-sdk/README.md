## Usage
This library is used to make clients that connect to an FTL server. Clients express interest in types of messages based on a content matcher. When a client publishes a message to the server, it is sent to all clients with a matching subscription. Clients may also specify callback functions to trigger when events - like a message being received - occur.

```
from eftl.asyncio.connection import EftlConnection

sender = EftlConnection()
receiver = EftlConnection()
sender.connect("ws://localhost:9191/channel", client_id="sender")
receiver.connect("ws://localhost:9191/channel", client_id="receiver")

def print_headline(**kwargs):
	print("Headline: ", kwargs.get("message", {}).get("headline"))

receiver.subscribe(matcher="""{"tag": "news"}""", on_message=print_message)
sender.publish({"Headline": "TIBCO Releases eFTL Client Library", "tag": "news"})
```

You may also create a key/value map associated with your EftlConneciton. Once created, you can get, set, and remove key/value mappings:

```
from eftl.asyncio.connection import EftlConnection

ec = EftlConnection()
ec.connect("ws://localhost:9191/channel")
menu = ec.map("menu")
menu.set("burger": 7)
menu.set("burger": 8)
print(menu.get("burger"))  # Prints 8
menu.remove("burger")
```

## Installation
The eFTL python library is hosted on PyPI and can be installed with `pip install eftl`

## Documentation
[EftlConnection documentation](github.com/TIBCOSoftware/TIBCO-Messaging/tree/master/eftl-python-sdk/docs/connection.md)
[\_EftlKVMap documentation](github.com/TIBCOSoftware/TIBCO-Messaging/tree/master/eftl-python-sdk/docs/kvmap.md)