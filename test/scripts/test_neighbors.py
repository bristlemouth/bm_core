import pytest
from serial_helper import SerialHelper
from neighbors_helper import Neighbors

class TestNeighbors:
    """Neighbors HIL test class"""
    def test_neighbors_get(self, ser: SerialHelper):
        """ Test to see if neighbors exist

        Attempts to obtain the neighbors to the node
        and asserts that they do exist.

        Args:
            ser (SerialHelper): Serial helper instance passed in from
                                starting the pytests (see: conftest.py)
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
            assert isinstance(neighbor_info[0], int)
            assert neighbor_info[1] == port
