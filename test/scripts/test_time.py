import time
import re
from serial_helper import SerialHelper
from topology_helper import Topology
from datetime import datetime, timezone
from util import format_node_id_to_hex_str


class TestTime:
    """Time test class"""

    def __time_process(self, ser: SerialHelper, node: int, now: int = None):
        """Process time response messages

        This obtains the response from the a time get/set message.

        Args:
            ser (SerialHelper): Serial helper instance passed.
            node (int): Node ID to process read message.
            now (int): Optional argument, the current time that was
                       set from a set message. This is utc time in
                       microseconds.
        """
        pattern = r"([0-9a-fA-F]{16}) to (\d{1,})/(\d{1,})/(\d{1,}) (\d{2}):(\d{2}):(\d{2}).(\d{3})"
        ser.read_until("Response time node ID: ")
        data = ser.read_line()
        match = re.match(pattern, data)
        if match is not None:
            # Ensure the match is complete and the node is a response
            # from the expected node
            assert len(match.groups()) == 8
            assert int(match.group(1), 16) == node

            # Assign the console message to a date time variable and
            # ensure it is what we set the value as
            if now is not None:
                dt = datetime(
                    int(match.group(2)),
                    int(match.group(3)),
                    int(match.group(4)),
                    int(match.group(5)),
                    int(match.group(6)),
                    int(match.group(7)),
                )
                calculated_utc = dt.replace(tzinfo=timezone.utc).timestamp()
                assert int(calculated_utc) == (now / 1000000)
        else:
            assert 0

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
        topology = Topology(ser)
        nodes = topology.get()
        for node in nodes:
            if node != topology.root():
                formatted_node = format_node_id_to_hex_str(node)

                # Time is set in microseconds...
                now = int(time.time()) * 1000000
                ser.transmit_str(
                    "bm time set " + formatted_node + " " + str(now) + "\n"
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
        topology = Topology(ser)
        nodes = topology.get()
        for node in nodes:
            if node != topology.root():
                formatted_node = format_node_id_to_hex_str(node)
                ser.transmit_str("bm time get " + formatted_node + "\n")
                self.__time_process(ser, node)
