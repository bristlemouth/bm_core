from serial_helper import SerialHelper
import pytest

SER = None


def pytest_addoption(parser):
    """Define command line arguments

    Sets command line options for pytest.
    """
    parser.addoption(
        "--port", action="store", default=None, help="Serial port to open for HIL test"
    )
    parser.addoption(
        "--baud",
        action="store",
        default=115200,
        help="Serial port baud rate for HIL test",
    )
    parser.addoption(
        "--file",
        action="store",
        default=None,
        help="DFU binary application file for loading over bristlemouth",
    )
    parser.addoption(
        "--node_id",
        action="store",
        default=None,
        help="Node ID of device to receive DFU, if passed in hex, prefix with 0x",
    )


@pytest.fixture(scope="function", autouse=True)
def setup_teardown():
    """Setup and teardown before every function

    Called at the beginning of every test function, is responsible for
    opening the serial port upon starting the test and closing the port
    at the end of the test.
    """
    global SER
    SER.open(SER.port, SER.baud, SER.timeout_s)
    yield
    SER.close()


def pytest_generate_tests(metafunc):
    """Begin session functionality

    Called at the beginning of the test suite, is responsible for
    opening the serial port and passing its instance to all tests.
    This function runs once for every test being ran.
    """
    global SER
    port = metafunc.config.option.port
    baud = metafunc.config.option.baud
    file = metafunc.config.option.file
    node_id = metafunc.config.option.node_id
    if "ser" in metafunc.fixturenames and port is not None:
        # This function should always pass only one instance of SER
        # as it runs for every test being ran, we only want to create
        # SER one time
        if SER is None:
            SER = SerialHelper(port, baud, 5.0)
        metafunc.parametrize("ser", [SER])
    if "file" in metafunc.fixturenames:
        metafunc.parametrize("file", [file])
    if "node_id" in metafunc.fixturenames:
        metafunc.parametrize("node_id", [node_id])


def pytest_sessionfinish(session, exitstatus):
    """Finish session functionality

    Called after whole test run finished, right before returning the
    exit status to the system. This is responsible for closing the
    serial port opened.
    """
    global SER
    if SER is not None:
        SER.close()
