import pytest
from serial_helper import SerialHelper
from neighbors_helper import Neighbors
from util import RunOrder


class TestNeighbors:
    """Neighbors HIL test class"""

    @pytest.mark.order(RunOrder.NEIGHBORS_TEST_RUN_ORDER)
    def test_neighbors_get(self, ser: SerialHelper):
        """Test to see if neighbors exist

        Attempts to obtain the neighbors to the node
        and asserts that they do exist.
        This test should run first before any other tests.

        Args:
            ser (SerialHelper): Serial helper instance passed in from
                                starting the pytests (see: conftest.py).
        """
        port = 0
        neighbors = Neighbors(ser)
        neighbors_info = list()
        neighbors_info = neighbors.get()

        # Ensure that there are actual neighbors
        assert len(neighbors_info) > 0
        for neighbor_info in neighbors_info:
            port += 1

            # Ensure that a neighbor node ID exists and the port found
            # matches the port expected number
            if not isinstance(neighbor_info[0], int) or neighbor_info[1] != port:
                pytest.fail("Failed neighbor test and cannot move on")
