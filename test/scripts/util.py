from enum import Enum
import sys
from typing import Union
import serial
import time
from functools import wraps


class RunOrder(Enum):
    NEIGHBORS_TEST_RUN_ORDER = 1
    TOPOLOGY_TEST_RUN_ORDER = 2


def format_node_id_to_hex_str(node: int) -> str:
    """Topology format node to hex string

    This returns a formatted string that can be used to pass into
    a terminal command to a Bristlemouth node's console.

    Args:
        node (int): A node that is obtained from the get method,
                    or another node found elsewhere (ex. neighbor).
    """
    return hex(node)[2:]


def print_progress_bar(s: str, current: Union[int, float], total: Union[int, float]):
    """Print a progress bar to the console

    Prints a pretty progress bar to the console formmatted as so:

        String To Print Here [##                     ]10%

    Args:
        s (str): String to print befor the progress bar.
        current (Union[int, float]): Current value.
        total (Union[int, float]): Total value.
    """
    progress = int((current / total) * 100.0)
    sys.stdout.write(
        f"\r{s}: [{'#' * progress}{' ' * (100 - progress)}]" f"{progress}%"
    )
    sys.stdout.flush()


def retry_test(max_attempts: int = 5, wait_s: float = 5.0):
    """Retry pytest for specified number of attempts

    Decorator to retry pytest tests for specific number of attempts.
    If a test fails, it will retry the the test after waiting
    a specified number of seconds. If the test fails after retried
    number of attempts, then it will be reported accordingly.

    Args:
        max_attempts (int): Number of maximum attempts the test can
                            be ran.
        wait_s (int): Time in seconds to wait in between test runs
                      if a test is to fail.
    """

    def decorator(test_fun):
        @wraps(test_fun)
        def wrapper(*args, **kwargs):
            retry_count = 1

            while retry_count < max_attempts:
                try:
                    return test_fun(*args, **kwargs)

                except AssertionError as assert_error:
                    assert_message = assert_error.__str__().split("\n")[0]
                    print(
                        f'Retry error: "{test_fun.__name__}" '
                        f"--> {assert_message}."
                        f"[{retry_count}/{max_attempts - 1}]"
                        f"Retrying new execution in {wait_s} second(s)"
                    )
                    time.sleep(wait_s)
                    retry_count += 1
                except serial.SerialException as serial_error:
                    serial_message = serial_error.__str__().split("\n")[0]
                    print(
                        f'Retry error: "{test_fun.__name__}" '
                        f"--> {serial_message}."
                        f"[{retry_count}/{max_attempts - 1}]"
                        f" Retrying new execition in {wait_s} second(s)"
                    )
                    time.sleep(wait_s)
                    retry_count += 1

            # Preserve original traceback in case assertion Failed
            return test_fun(*args, **kwargs)

        return wrapper

    return decorator
