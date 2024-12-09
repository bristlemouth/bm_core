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
        """Process received value messages

        This function is used for set and get config messages, as the
        other nodes will always respond with a value message. It
        determines if the received message is equivalent to (or close
        to equivalent to) what the expected value is.

        Args:
            ser (SerialHelper): Serial helper instance.
            read (str): String read from regex in
                        __handle_read_message.
            value (Any): Value of any type to compare agains
                         what was read. These values are the
                         ones found in the TYPE_VALUE variable.
        """
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
        _type: str,
        value: list[Any, str],
        node: int,
        wait_msg: str = None,
        pattern: str = None,
        handle: Callable[[SerialHelper, str, Any], None] = None,
    ):
        """Transmits and handles console messages

        This function will transmit the message to be tested then
        evaluate the console output to ensure that the message is
        is sent properly and will utilize a regex pattern to ensure
        the response message is handled properly and the console
        message matches that regex pattern.

        Args:
            ser (SerialHelper): Serial helper instance.
            func (Callable[[str, str, Any, str], None]): Function called
                                                         to format the
                                                         tx message
                                                         string
            _type (str): type of message as seen as the keys in the
                         TYPE_VALUE dictionary.
            value (list[Any, str]): List of values from TYPE_VALUE
                                    dictionary.
            wait_msg (str): Console output to wait for on the network to
                            ensure that a message is sent properly.
            pattern (str): Regex pattern to wait for the console to
                           print in order to determine if the response
                           received matches
            handle (Callable[[SerialHelper, str, Any], None]): If pattern
                                                               matches,
                                                               this func-
                                                               tion will
                                                               be ran.
        """

        formatted_node = format_node_id_to_hex_str(node)
        ser.transmit_str(func(formatted_node, _type, value[0], value[1]))
        read = ser.read_until(wait_msg)
        assert wait_msg in read
        if pattern is not None:
            read = ser.read_until_regex(pattern)
            assert read is not None
            if handle is not None:
                handle(ser, read, value[0])

    def __send_config_message(
        self,
        ser: SerialHelper,
        func: Callable[[str, str, Any, str], None],
        wait_msg: str = None,
        pattern: str = None,
        itr: bool = True,
        handle: Callable[[SerialHelper, str, Any], None] = None,
        delay: float = None,
    ):
        """Generic function to send a config message

        Will send a config message to all nodes on the network, besides
        the node currently being tested. If the message shall iterate
        through the dictionary TYPE_VALUE, then commands from the
        node will execute messages on all items in the dictonary.

        Args:
            ser (SerialHelper): Serial helper instance.
            func (Callable[[str, str, Any, str], None]): Function called
                                                         to format the
                                                         tx message
                                                         string
            wait_msg (str): Console output to wait for on the network to
                            ensure that a message is sent properly.
            pattern (str): Regex pattern to wait for the console to
                           print in order to determine if the response
                           received matches
            itr (bool): Whether or not the message needs to iterate
                        through all of the TYPE_VALUE array.
            handle (Callable[[SerialHelper, str, Any], None]): If pattern
                                                               matches,
                                                               this func-
                                                               tion will
                                                               be ran.
            delay_s (float): Will be set if a delay is necessary between
                             sending messages on the network.
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
                            wait_msg,
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
                        wait_msg,
                        pattern,
                        handle,
                    )
                if delay is not None:
                    # Open up port again before reading in case the node is
                    # disabled (ex: the bridge shuts off the power to the
                    # network)
                    sleep(delay)
                    ser.close()
                    ser.open(ser.port, ser.baud, ser.timeout_s)

    def test_config_set(self, ser: SerialHelper):
        """Send set command from node

        Sends the set command from the node to all other nodes on the
        network, this iterates through all of the TYPE_VALUE dictionary for
        each node and ensures that the messages are sent properly and
        the value message is received properly.

        Args:
            ser (SerialHelper): Serial helper instance passed in from
                                starting the pytests (see: conftest.py).
        """
        pattern = r"Node Id: [0-9a-fA-F]{16} Value:"
        wait_msg = "Succesfully sent config set msg\n"

        def message(x, y, z, a):
            return "bm cfg set " + x + " s " + y + " " + a + " " + str(z) + "\n"

        self.__send_config_message(
            ser, message, wait_msg, pattern, True, self.__process_value_message_handler
        )

    def test_config_get(self, ser: SerialHelper):
        """Send get command from node

        Sends the get command from the node to all other nodes on the
        network, this iterates through all of the TYPE_VALUE dictionary for
        each node and ensures that the messages are sent properly and
        the value message is received properly.

        Args:
            ser (SerialHelper): Serial helper instance passed in from
                                starting the pytests (see: conftest.py).
        """
        pattern = r"Node Id: [0-9a-fA-F]{16} Value:"
        wait_msg = "Succesfully sent config get msg\n"

        def message(x, y, z, a):
            return "bm cfg get " + x + " s " + a + "\n"

        self.__send_config_message(
            ser, message, wait_msg, pattern, True, self.__process_value_message_handler
        )

    def test_config_commit(self, ser: SerialHelper):
        """Send commit message to all nodes

        This test sets all the config values in the TYPE_VALUE
        dictionary to all other nodes on the network, then sends
        a commit message to all nodes on the network. Once the
        commit message is successfully sent, a 10 second delay
        is set between commit messages to each node as there
        may be a chance that a node will shut down the power
        to the whole bus (ex: the bridge). After, the values are then
        read from all of the nodes on the bus and compared to the
        values in the TYPE_VALUE dictionary to ensure that the
        data has been properly set.

        Args:
            ser (SerialHelper): Serial helper instance passed in from
                                starting the pytests (see: conftest.py).
        """
        pattern = None
        wait_msg = "Succesfully config commit send\n"

        def message(x, y, z, a):
            return "bm cfg commit " + x + " s\n"

        # Set configs, commit configs and then read configs
        self.test_config_set(ser)
        self.__send_config_message(
            ser, message, wait_msg, pattern, False, None, self.COMMIT_SLEEP_TIME_S
        )

        self.test_config_get(ser)

    def test_config_status(self, ser: SerialHelper):
        """Send status message to all nodes

        This tests the status message on all other nodes besides the
        root node on the bus. Values are set from the TYPE_VALUE
        dictionary and then the status message is sent for each node.
        The response received is evaluated and the output of all
        configuration keys is evaluated to the key of the configuration
        key-pair values set. The response message is printed to the
        console as:
            Response msg -- Node Id: {node_id}
            Num Keys: {num_keys}
            key_1
            key_2
            key_3
            ...

        Args:
            ser (SerialHelper): Serial helper instance passed in from
                                starting the pytests (see: conftest.py).
        """
        pattern = r"Response msg -- Node Id: [0-9a-fA-F]{16}, Partition: \d+, Commit Status: \d+\nNum Keys: (\d+)\n"
        wait_msg = "Successful status request send\n"

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
        self.__send_config_message(ser, message, wait_msg, pattern, False, compare)

    def test_config_delete(self, ser: SerialHelper):
        """Send delete message to all nodes

        This test deletes all the keys in the TYPE_VALUE dictionary and
        ensures that the message is sent properly.

        Args:
            ser (SerialHelper): Serial helper instance passed in from
                                starting the pytests (see: conftest.py).
        """
        pattern = None
        wait_msg = "Successfully sent del key request\n"

        def message(x, y, z, a):
            return "bm cfg del " + x + " s " + a + "\n"

        # Currently there is no way to check if a node deleted a key
        # other than calling bm cfg get <node_id> <partition> <key>
        # and ensuring there is no response from the node in
        # question, which is a manual process
        self.__send_config_message(ser, message, wait_msg, pattern, True)
