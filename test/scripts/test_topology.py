import pytest
from serial_helper import SerialHelper
from topology_helper import Topology
from neighbors_helper import Neighbors
from util import RunOrder, retry_test


class TestTopology:
    """Topology HIL test class"""

    @pytest.mark.order(RunOrder.TOPOLOGY_TEST_RUN_ORDER.value)
    @retry_test(max_attempts=5, wait_s=2)
    def test_topology_get(self, ser: SerialHelper):
        """Test to see if the topology report is correct

        This test runs to see if the topology can properly be printed
        from the command line argument, bm topo. The topology helper
        ensures that the string is formatted in a specific format
        reported from the console. With this reported string, the
        tests then checks to see if there are more nodes on the network
        than the number of neighbors. If there are we succeed.

        Args:
            ser (SerialHelper): Serial helper instance passed in from
                                starting the pytests (see: conftest.py).
        """
        neighbors = Neighbors(ser).get()
        nodes = Topology(ser).get()

        # Ensure the number of nodes seen are greater than the
        # number of neighbors plus the current node
        min_nodes = len(neighbors) + 1
        assert len(nodes) >= min_nodes
