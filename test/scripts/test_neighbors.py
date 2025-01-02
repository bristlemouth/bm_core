import pytest
from serial_helper import SerialHelper
from neighbors_helper import Neighbors
from util import RunOrder, retry_test


class TestNeighbors:
    """Neighbors HIL test class"""

    @pytest.mark.order(RunOrder.NEIGHBORS_TEST_RUN_ORDER.value)
    @retry_test(max_attempts=5, wait_s=2)
    def test_neighbors_get(self, ser: SerialHelper):
        """Test to see if neighbors exist

        Attempts to obtain the neighbors to the node
        and asserts that they do exist.
        This test should run first before any other tests.

        Args:
            ser (SerialHelper): Serial helper instance passed in from
                                starting the pytests (see: conftest.py).
        """
        neighbors = Neighbors(ser)
        neighbors_info = list()
        neighbors_info = neighbors.get()

        # Ensure that there are actual neighbors
        assert len(neighbors_info) > 0
        for neighbor_info in neighbors_info:

            # Ensure that a neighbor node ID and the port found exists
            assert isinstance(neighbor_info[0], int)
            assert isinstance(neighbor_info[1], int)
