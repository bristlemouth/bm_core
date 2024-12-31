from serial_helper import SerialHelper
import re
import sys
import argparse


class Topology:
    """Topology class

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
        self.__ser = ser
        self.__root = None

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
        root_pattern = r"(\(root\))([0-9a-fA-F]{16})"

        # Send bm topo command and wait until the device responds with
        # a regex match
        self.__ser.transmit_str("bm topo\n")
        s = self.__ser.read_until_regex(r"[0-9a-fA-F]{16}.*\n")
        ret = re.findall(pattern, s)
        if ret is not None:
            # Find the root node
            self.__root = int(re.search(root_pattern, s).group(2), 16)
        for i in range(len(ret)):
            ret[i] = int(ret[i], 16)

        return ret

    def root(self) -> int:
        """Get root node ID

        Gets the root node ID of the topology report. If the topology
        report has not been ran, it will run the report and obtain the
        root node ID.

        Returns:
            int: The node ID of the root node
        """
        if self.__root is None:
            self.get()
        return self.__root


if __name__ == "__main__":
    """Command line option if this script is invoked directly

    CLI Options that will print topology information if this script
    is invoked directly
    """
    parser = argparse.ArgumentParser(
        description="Prints a list of the Bristlemouth topology or root"
        "node to the console",
    )
    parser.add_argument(
        "-r", "--root", action="store_true", help="print the root to the console"
    )
    parser.add_argument(
        "-p", "--port", help="serial port to access node", required=True
    )
    parser.add_argument(
        "-b", "--baud", help="baud rate of serial connection", required=True
    )
    args = parser.parse_args()
    ser = SerialHelper(args.port, args.baud)
    topology = Topology(ser)
    if args.root is True:
        print(hex(topology.root()))
    else:
        values = topology.get()
        for i in range(len(values)):
            values[i] = hex(values[i])
        print(values)
