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
    if "ser" in metafunc.fixturenames and port is not None:
        # This function should always pass only one instance of SER
        # as it runs for every test being ran, we only want to create
        # SER one time
        if SER is None:
            SER = SerialHelper(port, baud, 0.5)
        metafunc.parametrize("ser", [SER])


def pytest_sessionfinish(session, exitstatus):
    """Finish session functionality

    Called after whole test run finished, right before returning the
    exit status to the system. This is responsible for closing the
    serial port opened.
    """
    global SER
    SER.close()
