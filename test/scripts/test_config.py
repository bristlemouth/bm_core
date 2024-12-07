import re
from serial_helper import SerialHelper
from topology_helper import Topology
from typing import Callable, Any
from time import sleep
from util import format_node_id_to_hex_str


class TestConfig:
    """Configuration HIL test class"""

    CONFIG_TEST_KEY_UINT = "test_config_name_uint"
    CONFIG_TEST_KEY_INT = "test_config_name_int"
    CONFIG_TEST_KEY_FLOAT = "test_config_name_float"
    CONFIG_TEST_KEY_STRING = "test_config_name_string"

    CONFIG_TEST_STR = "test_me!"
    CONFIG_TEST_UINT = 32
    CONFIG_TEST_INT = -64
    CONFIG_TEST_FLOAT = 10000.4592

    TYPE_VALUE = {
        "u": [CONFIG_TEST_UINT, CONFIG_TEST_KEY_UINT],
        "i": [CONFIG_TEST_INT, CONFIG_TEST_KEY_INT],
        "f": [CONFIG_TEST_FLOAT, CONFIG_TEST_KEY_FLOAT],
        "s": [CONFIG_TEST_STR, CONFIG_TEST_KEY_STRING],
    }

    COMMIT_SLEEP_TIME_S = 10.0

    def __process_value_message_handler(self, ser: SerialHelper, read: str, value: Any):
        read = ser.read_line().replace("\n", "")
        value_type = type(value)
        if value_type is not float:
            assert value_type(read) == value
        else:
            assert value_type(read) > value * 0.95 and value_type(read) < value * 1.05

    def __handle_read_message(
        self,
        ser: SerialHelper,
        func: Callable[[str, str, Any, str], None],
        _type: type,
        value: list[any, str],
        node: int,
        rx_msg: str = None,
        pattern: str = None,
        handle: Callable[[SerialHelper, str, Any], None] = None,
    ):

        formatted_node = format_node_id_to_hex_str(node)
        ser.transmit_str(func(formatted_node, _type, value[0], value[1]))
        read = ser.read_until(rx_msg)
        assert rx_msg in read
        if pattern is not None:
            read = ser.read_until_regex(pattern)
            assert read is not None
            if handle is not None:
                handle(ser, read, value[0])

    def __send_config_message(
        self,
        ser: SerialHelper,
        func: Callable[[str, str, Any, str], None],
        rx_msg: str = None,
        pattern: str = None,
        itr: bool = True,
        handle: Callable[[SerialHelper, str, Any], None] = None,
    ):
        """Generic function to send a config message

        Will send a config message to all nodes on the bus, besides
        the node currently being tested. If the message shall iterate
        through the dictionary TYPE_VALUE, then commands from the
        node will execute messages on all items in the dictonary.

        Args:
            ser
        """
        topology = Topology(ser)
        nodes = topology.get()

        for node in nodes:
            if node != topology.root():
                if itr:
                    for _type, value in self.TYPE_VALUE.items():
                        self.__handle_read_message(
                            ser,
                            func,
                            _type,
                            value,
                            node,
                            rx_msg,
                            pattern,
                            handle,
                        )
                else:
                    value = [None, None]
                    _type = None
                    self.__handle_read_message(
                        ser,
                        func,
                        _type,
                        value,
                        node,
                        rx_msg,
                        pattern,
                        handle,
                    )

    def test_config_set(self, ser: SerialHelper):
        pattern = r"Node Id: [0-9a-fA-F]{16} Value:"
        rx_msg = "Succesfully sent config set msg\n"

        def message(x, y, z, a):
            return "bm cfg set " + x + " s " + y + " " + a + " " + str(z) + "\n"

        self.__send_config_message(
            ser, message, rx_msg, pattern, True, self.__process_value_message_handler
        )

    def test_config_get(self, ser: SerialHelper):
        pattern = r"Node Id: [0-9a-fA-F]{16} Value:"
        rx_msg = "Succesfully sent config get msg\n"

        def message(x, y, z, a):
            return "bm cfg get " + x + " s " + a + "\n"

        self.__send_config_message(
            ser, message, rx_msg, pattern, True, self.__process_value_message_handler
        )

    def test_config_commit(self, ser: SerialHelper):
        pattern = None
        rx_msg = "Succesfully config commit send\n"

        def message(x, y, z, a):
            return "bm cfg commit " + x + " s\n"

        # Set configs, commit configs and then read configs
        self.test_config_set(ser)
        self.__send_config_message(ser, message, rx_msg, pattern, False)
        sleep(self.COMMIT_SLEEP_TIME_S)

        # Open up port again before reading in case the node is
        # disabled (ex: the bridge shuts off the power to the network)
        ser.close()
        ser.open(ser.port, ser.baud, ser.timeout_s)
        self.test_config_get(ser)

    def test_config_status(self, ser: SerialHelper):
        pattern = r"Response msg -- Node Id: [0-9a-fA-F]{16}, Partition: \d+, Commit Status: \d+\nNum Keys: (\d+)\n"
        rx_msg = "Successful status request send\n"

        def message(x, y, z, a):
            return "bm cfg status " + x + " s\n"

        def compare(ser: SerialHelper, read: str, value: Any):
            """Compares status message keys to known set keys

            Checks all keys on the node and determines if the keys
            exist on the
            """
            count = int(re.search(pattern, read).group(1))
            found = 0
            for i in range(count):
                read_key = ser.read_line()
                for value in self.TYPE_VALUE.values():
                    key = value[1]
                    if key in read_key:
                        found += 1
            assert len(self.TYPE_VALUE) == found

        self.test_config_set(ser)
        self.__send_config_message(ser, message, rx_msg, pattern, False, compare)

    def test_config_delete(self, ser: SerialHelper):
        pattern = None
        rx_msg = "Successfully sent del key request\n"

        def message(x, y, z, a):
            return "bm cfg del " + x + " s " + a + "\n"

        self.__send_config_message(ser, message, rx_msg, pattern, True)

        # Currently there is no way to check if a node deleted a key
        # other than calling bm cfg get <node_id> <partition> <key>
        # and ensuring there is no response from the node in
        # question, which is a manual process
