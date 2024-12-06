import time
import re
from serial_helper import SerialHelper
from topology_helper import Topology
from datetime import datetime, timezone
from util import format_node_id_to_hex_str


class TestTime:
    """Time test class"""

    def __time_process(self, ser: SerialHelper, node: int, now: float = None):
        """Process time response messages

        This obtains the response from the a time get/set message.

        Args:
            ser (SerialHelper): Serial helper instance passed.
            node (int): Node ID to process read message.
            now (float): Optional argument, the current time that was
                         set from a set message.
        """
        pattern = r"([0-9a-fA-F]{16}) to (\d{1,}) (\d{1,}) (\d{1,}) (\d{2}):(\d{2}):(\d{2}).(\d{3})"
        ser.read_until("Response time node ID: ")
        data = ser.read_line()
        match = re.match(pattern, data)
        assert len(match.group) == 8
        assert int(match.group(1), 16) == node
        if now is not None:
            dt = datetime(
                match.group(2),
                match.group(3),
                match.group(4),
                match.group(5),
                match.group(6),
                match.group(7),
            )
            calculated_utc = dt.replace(tzinfo=timezone.utc).timestamp()
            assert calculated_utc == now

    def test_time_set(self, ser: SerialHelper):
        """Time set test

        Will iterate through all nodes on the bus and attempt to set
        the time. This evaluates the console statement from the
        response message from the nodes on the bus. Will ensure the
        response message has the correct time time that was set.

        Args:
            ser (SerialHelper): Serial helper instance passed in from
                                starting the pytests (see: conftest.py).
        """
        topology = Topology(ser).get()
        for node in topology:
            formatted_node = format_node_id_to_hex_str(node)
            now = time.time()
            ser.transmit_str(
                "bm time set " + formatted_node + " " + str(int(now)) + "\n"
            )
            self.__time_process(ser, node, now)

    def test_time_get(self, ser: SerialHelper):
        """Time set test

        Will iterate through all nodes on the bus and attempt to get
        the time. This evaluates the console statement from the
        response message from the nodes on the bus.

        Args:
            ser (SerialHelper): Serial helper instance passed in from
                                starting the pytests (see: conftest.py).
        """
        topology = Topology(ser).get()
        for node in topology:
            formatted_node = format_node_id_to_hex_str(node)
            ser.transmit_str("bm time get " + formatted_node + "\n")
            self.__time_process(ser, node)
