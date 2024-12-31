from topology_helper import Topology
from serial_helper import SerialHelper
from util import format_node_id_to_hex_str, retry_test


class TestPing:
    """Ping test class"""

    TEST_STRING = "test_string"

    def _compare_data(self, ser: SerialHelper):
        pattern = r"\d+ bytes from [0-9a-fA-F]{16} bcmp_seq=\d+ time=\d+ ms payload="
        ser.read_until_regex(pattern)
        read = ser.read_line()
        vals = read.split()
        for i in range(len(vals)):
            vals[i] = int(vals[i], 16)
        vals = bytes(vals).decode("utf-8")
        assert self.TEST_STRING == vals

    @retry_test(max_attempts=3, wait_s=2)
    def test_ping(self, ser: SerialHelper):
        """Test to see if ping command works as expected

        This test runs the ping command and ensures that a proper response
        is received from every other node on the bus.

        Args:
            ser (SerialHelper): Serial helper instance passed in from
                                starting the pytests (see: conftest.py).
        """
        topology = Topology(ser)
        nodes = topology.get()
        for node in nodes:
            if node != topology.root():
                formatted_node = format_node_id_to_hex_str(node)
                ser.transmit_str(
                    "bm ping " + formatted_node + " " + self.TEST_STRING + "\n"
                )
                self._compare_data(ser)

        # Send ping request to all nodes and ensure that all respond
        # this would be the ping request with 0 as the node ID
        count = 0
        ser.transmit_str("bm ping 0 " + self.TEST_STRING + "\n")
        for node in nodes:
            if node != topology.root():
                self._compare_data(ser)
                count += 1
        assert count == len(nodes) - 1
