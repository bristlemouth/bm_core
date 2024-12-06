from serial_helper import SerialHelper
import re


class Topology:
    """Topology classs

    Class utilized to obtain information about the topology as seen by
    the node.
    """

    def __init__(self, ser: SerialHelper = None):
        """Topology constructor

        Setup the Topology class instance.

        Args:
            ser (SerialHelper): Serial helper instance to perform read/
                                write commands on the node
        """
        self._ser = ser

    def get(self) -> list[int]:
        """Get topology report

        Get the topology as seen by the node being tested on.

        Returns:
            list[int]: A list of integers that reports the nodes seen
                       on the network.
        """
        ret = list()
        s = ""
        pattern = r"[0-9a-fA-F]{16}"

        # Send bm topo command and wait until the device responds with
        # a regex match
        self._ser.transmit_str("bm topo\n")
        s = self._ser.read_until_regex(r"[0-9a-fA-F]{16}.*\n")
        ret = re.findall(pattern, s)
        for i in range(len(ret)):
            ret[i] = int(ret[i], 16)

        return ret
