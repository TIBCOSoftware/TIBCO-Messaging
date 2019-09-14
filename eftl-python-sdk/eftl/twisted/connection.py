import logging
import threading
import time

from autobahn.twisted.websocket import WebSocketClientProtocol, connectWS
from autobahn.twisted.websocket import WebSocketClientFactory
from twisted.internet import reactor

from eftl.connection_base import EftlConnectionBase, CLOSED


logger = logging.getLogger(__name__)
logger.addHandler(logging.NullHandler())


class EftlConnection(_EftlConnectionBase):

    def __init__(self):
        super().__init__()
        self.client_protocol = WebSocketClientProtocol
        self.client_factory = WebSocketClientFactory

    def run_event_loop(self, interval=None):
        if interval == None:
            while not self._permanently_closed(warn=False):
                time.sleep(1)
        else:
            time.sleep(interval)

    def _do_connect(self, url, timeout, polling_interval):
        connectWS(self.factory, timeout=timeout)
        timeout_counter = timeout
        if not reactor.running:
            self.reactor_thread = threading.Thread(target=reactor.run, args=(False,))
            self.reactor_thread.setDaemon(True)
            self.reactor_thread.start()

        while self._ws is None:
            time.sleep(polling_interval)
            timeout_counter -= polling_interval
            if timeout_counter <= 0:
                raise EftlClientError("Connection timed out for url {}".format(url.geturl()))
        while self._ws is not None and not self._ws.got_login_reply:
            time.sleep(polling_interval)
        return self._ws is not None

    def _halt_until_signal(self, timeout, polling_interval):
        timeout_counter = 0
        err_msg = "Socket closed while waiting on server reply"
        while self.halt and not self._permanently_closed(error=err_msg, warn=False):
            if timeout is not None and timeout_counter >= timeout:
                logger.debug("Exceeded timeout waiting on response from server")
                break
            time.sleep(polling_interval)
            timeout_counter += polling_interval
        self.halt = True
