import os
import pytest
import warnings
warnings.filterwarnings("ignore", category=DeprecationWarning)

if bool(os.environ.get("TIBCO_TEST_TWISTED")):
    from eftl.twisted.connection import EftlConnection
else:
    from eftl.asyncio.connection import EftlConnection

import logging
logging.basicConfig(level=logging.DEBUG)

# Set environment variables TIBCO_TEST_URL and TIBCO_TEST_PASSWORD to test your desired FTL server.
url = os.environ.get("TIBCO_TEST_URL", "ws://localhost:9191/channel")
password = os.environ.get("TIBCO_TEST_PASSWORD", "")


def test_connect():
    ec = EftlConnection()
    ec.connect(
        url,
        client_id='python_example',
        login_timeout=5,
        password=password)
    assert ec.is_connected()


def test_bad_scheme():
    with pytest.raises(ValueError):
        ec = EftlConnection()
        ec.connect(
            "invalid://localhost:9191",
            client_id='python-example',
            )


def test_no_host():
    with pytest.raises(ValueError):
        ec = EftlConnection()
        ec.connect(
            "wss://:9191",
            client_id='python-example',
            )


def test_multiple_urls():
    ec = EftlConnection()
    ec.connect(
        "ws://localhost:5555|{}".format(url),
        client_id='python-example',
        password=password,
        )
    assert ec.is_connected()


def test_multiple_clients():
    ec = EftlConnection()
    ec2 = EftlConnection()
    ec.connect(
        url,
        client_id='python-example',
        password=password,
        )
    ec2.connect(
        url,
        client_id='python-example2',
        password=password,
        )
    assert ec.is_connected()
    assert ec2.is_connected()


def test_pub_sub():
    ec = EftlConnection()
    ec2 = EftlConnection()
    ec.connect(
        url,
        client_id='python-subscriber',
        password=password,
        )
    ec2.connect(
        url,
        client_id='python-publisher',
        password=password,
        )

    def save_arbitrary(**kwargs):
        ec.arbitrary = kwargs.get("message", {}).get("arbitrary")

    ec.subscribe(
        matcher="""{"_dest": "sample"}""",
        durable='sample',
        on_message=save_arbitrary,
        )

    arbitrary = 30
    ec2.publish({
        "_dest": "sample",
        "text": "Sample from publisher containing an arbitrary number",
        "arbitrary": arbitrary,
        })

    assert ec.arbitrary == arbitrary


def test_map_operations():
    ec = EftlConnection()
    ec.connect(
        url,
        client_id='python-example',
        password=password,
        )

    def save_kv_pair(**kwargs):
        message = kwargs.get("message")
        key = kwargs.get("key")
        if message and key:
            ec.kv_pair = (key, message)

    my_map = ec.map("menus")
    my_map.set("buona", {"pizza": 8, "shake": 5})
    my_map.get("buona", on_success=save_kv_pair)

    assert ec.kv_pair == ("buona", {"pizza": 8, "shake": 5})
