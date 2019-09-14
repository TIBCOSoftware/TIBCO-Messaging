import asyncio
import logging
import socket
import ssl
import concurrent.futures

from autobahn.asyncio.websocket import WebSocketClientProtocol
from autobahn.asyncio.websocket import WebSocketClientFactory

from eftl.connection_base import EftlConnectionBase


logger = logging.getLogger(__name__)
logger.addHandler(logging.NullHandler())


class EftlConnection(EftlConnectionBase):

    def __init__(self):
        super().__init__()
        self.client_protocol = WebSocketClientProtocol
        self.client_factory = WebSocketClientFactory

    def run_event_loop(self, interval=None):

        async def async_sleep(t):
            await asyncio.sleep(t)

        if interval == None:
            while not self._permanently_closed(warn=False):
                asyncio.get_event_loop().run_until_complete(async_sleep(1))
        else:
            asyncio.get_event_loop().run_until_complete(async_sleep(interval))

    def _do_connect(self, url, timeout, polling_interval):
        async def async_connect(self, loop, url, polling_interval):
            if url.scheme == "wss":
                sock = socket.create_connection((url.hostname, 443))
                sslcontext = ssl.SSLContext(protocol=ssl.PROTOCOL_TLS)
                sock = sslcontext.wrap_socket(sock, server_hostname=url.hostname)
                await loop.create_connection(self.factory, sock=sock)
            else:
                await loop.create_connection(self.factory, url.hostname, url.port)

            while self._ws is not None and not self._ws.got_login_reply:
                await asyncio.sleep(polling_interval)
            return self._ws is not None

        loop = asyncio.get_event_loop()
        return loop.run_until_complete(async_connect(self, loop, url, polling_interval))

    def _halt_until_signal(self, timeout, polling_interval):
        async def wait_loop():
            err_msg = "Socket closed while waiting on server reply"
            while self.halt and not self._permanently_closed(error=err_msg, warn=False):
                await asyncio.sleep(polling_interval)

        async def wait_with_timeout(timeout):
            try:
                await asyncio.wait_for(wait_loop(), timeout=timeout)
            except concurrent.futures._base.TimeoutError as e:
                logger.debug("Exceeded timeout waiting on response from server:\n{}".format(e))
            self.halt = True

        asyncio.get_event_loop().run_until_complete(wait_with_timeout(timeout))
