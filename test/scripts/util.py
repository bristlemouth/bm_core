from enum import Enum
import sys
from typing import Union


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
