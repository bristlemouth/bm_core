import serial


class SerialHelper:
    """Serial module helper class

    This class is responsible for providing a simple interface over the
    pyserial library.
    """

    def __init__(
        self, port: str = None, baudrate: int = 115200, timeout_s: float = 5.0
    ):
        """SerialHelper constructor

        Will setup the SerialHelper class, does not need a port, but
        will not open a up to a port if one is not provided.

        Args:
            port (str): String name of path to serial port to open
                        ex: '/dev/ttyACM0'.
            baudrate (int): Baudrate of port.
            timeout_s (int): Timeout in seconds to open/write to/read
                             from a port.
        """
        self.port = port
        self.baud = baudrate
        self._inst = None
        if port != None:
            self.open(port, baudrate, timeout_s)

    def open(self, port: str, baudrate: int = 115200, timeout_s: float = 5.0):
        """Open a serial sort

        Opens a serial port and assigns it to an instance that is used
        to read from/write to.

        Args:
            port (str): String name of path to serial port to open
                        ex: '/dev/ttyACM0'.
            baudrate (int): Baudrate of port.
            timeout_s (int): Timeout in seconds to open/write to/read
                             from a port.
        """
        self.port = port
        self.baud = baudrate
        try:
            self._inst = serial.Serial(
                port=self.port, timeout=timeout_s, baudrate=self.baud
            )
            self.flush()
        except serial.SerialException:
            raise Exception("Failed to open serial port...")
            self._inst = None

    def close(self):
        """Close serial port

        Closes the serial port that was previously opened in this
        instance.
        """
        self._inst.close()

    def flush(self):
        """Flush serial port

        Flush serial port that was previously opened in this instance.
        This will flush the input and output buffers.
        """
        self._inst.reset_input_buffer()
        self._inst.reset_output_buffer()

    def transmit_data(self, data: bytearray):
        """Transmit data

        Transmits data to open serial port in bytearray.

        Args:
            data (bytearray): data to transmit to opened port.
        """
        try:
            self._inst.write(data)
        except:
            raise Exception(
                "Could not write to serial port, \
                            check if it is open..."
            )

    def transmit_str(self, data: str):
        """Transmit string

        Transmits data in string format to open serial port.

        Args:
            data (str): string to transmit to opened port.
        """
        self.transmit_data(data.encode("utf-8"))

    def read_until(self, seq: str = "\n") -> str:
        """Read from serial port until sequence is found.

        Read lines until a certain sequence is found from the serial
        port. Will continue to read until sequence is found or timeout
        occurs.

        Args:
            seq (str): String to search for from serial port.

        Returns:
            str: The line read from the port as well as the new-line
                 character.
        """
        try:
            return self._inst.read_until(seq.encode("utf-8")).decode("utf-8")
        except:
            raise Exception(
                "Could not read from serial port, \
                            check if it is open..."
            )

    def read_line(self) -> str:
        """Read line from serial port

        Read lines terminated with new-line character from serial port.

        Returns:
            str: The line read from the port as well as the new-line
                 character.
        """
        return self.read_until()
