from serial_helper import SerialHelper

SER = None

def pytest_addoption(parser):
    parser.addoption("--port",
                     action="store",
                     default=None,
                     help='Serial port to open for HIL test')
    parser.addoption("--baud",
                     action="store",
                     default=115200,
                     help='Serial port baud rate for HIL test')

def pytest_generate_tests(metafunc):
    """Begin session functionality

    Called at the beginning of the test suit, is responsible for
    opening the serial port and passing its instance to all tests.
    """
    port = metafunc.config.option.port
    baud = metafunc.config.option.baud
    if 'ser' in metafunc.fixturenames and port is not None:
            SER = SerialHelper(port, baud, 0.5)
            metafunc.parametrize('ser', [SER])

def pytest_sessionfinish(session, exitstatus):
    """Finish session functionality

    Called after whole test run finished, right before returning the
    exit status to the system. This is responsible for closing the
    serial port opened.
    """
    if SER is not None:
        SER.close()
