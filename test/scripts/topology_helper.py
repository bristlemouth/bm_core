from serial_helper import SerialHelper
import re
import sys


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
    """Command line option is this script is invoked directly

    CLI Options that will print topology information is this script
    is invoked directly
    """
    help = False
    root = False
    port = None
    port_arg = False
    baud = None
    baud_arg = False

    for arg in sys.argv:
        if arg == "--help" or arg == "-h":
            help = True
            break
        elif arg == "--root" or arg == "-r":
            root = True
        elif arg == "--port" or arg == "-p":
            port_arg = True
        elif port_arg is True:
            port = arg
            port_arg = False
        elif arg == "--baud" or arg == "-b":
            baud_arg = True
        elif baud_arg is True:
            baud = int(arg)
            baud_arg = False

    if help is True:
        print(
            "usage: topology_helper.py [-h] [-r] -p {port}\n\n"
            "prints a list of the topology or "
            "root node to the console\n\n"
            "required arguments:\n"
            "-p, --port {port} serial port to access node\n"
            "-b, --baud {baud} baud rate of serial connection\n"
            "\n"
            "optional arguments:\n"
            "-h, --help show this help message and exit\n"
            "-r, --root print the root to the console"
        )
    else:
        ser = SerialHelper(port, baud)
        topology = Topology(ser)
        if root is True:
            print(hex(topology.root()))
        else:
            values = topology.get()
            for i in range(len(values)):
                values[i] = hex(values[i])
            print(values)
