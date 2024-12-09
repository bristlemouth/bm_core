from topology_helper import Topology
from serial_helper import SerialHelper
from util import format_node_id_to_hex_str


class TestInfo:
    """Info test class"""

    def test_info(self, ser: SerialHelper):
        """Test the BCMP info message

        This test the info request/reply messages. It checks the
        console print messages and attempts to match it to a regex
        pattern. If the pattern matches, the reply from the other
        nodes were sent successfully.

        Args:
            ser (SerialHelper): Serial helper instance passed in from
                                starting the pytests (see: conftest.py).
        """
        pattern = (
            r"Neighbor information:\n"
            r"Node ID: \w+\n"
            r"VID: \w{4} PID: \w{4}\n"
            r"Serial number \w{16}\n"
            r"GIT SHA: \w+\nVersion: \w+.\w+.\w+\n"
            r"HW Version: \d+\nVersionStr: \S+\n"
            r"Device Name: \S+\n"
        )
        topology = Topology(ser)
        nodes = topology.get()

        for node in nodes:
            if node != topology.root():
                tx_s = "bm info " + format_node_id_to_hex_str(node) + "\n"
                ser.transmit_str(tx_s)
                read = ser.read_until_regex(pattern)
                assert read is not None
