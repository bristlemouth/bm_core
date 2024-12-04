from serial_helper import SerialHelper
import re
from typing import Tuple


class Neighbors:
    """Neighbors class

    Class utilized to obtain information about neighboring nodes
    """

    def __init__(self, ser: SerialHelper = None):
        """Neighbors constructor

        Setup the Neighbors class instance.

        Args:
            ser (SerialHelper): Serial helper instance to perform read/
                                write commands on the node
        """
        self._ser = ser

    def get(self) -> list[Tuple[int, int]]:
        """Get node neighbors

        Obtain the node's neighbors' node ID and port association.

        Returns:
            list[Tuple[int, int]]: A list of tuples per neighbor. The
                                   tuple is formatted as
                                   (node_id, port).
        """
        node_id = 0
        port = 0
        ret = list()
        pattern = r"(\S{16}) \| *(\d) *\| *(\S*)"

        # Transmit neighbors command to CLI and read until specific
        # string with endline termination...
        self._ser.transmit_str("bm neighbors\n")
        self._ser.read_until("Time since last heartbeat (s)\n")
        while True:
            line = self._ser.read_line()
            match = re.search(pattern, line)
            if match:
                node_id = int(match.group(1), 16)
                port = int(match.group(2))
                ret.append((node_id, port))
            else:
                break
        return ret
