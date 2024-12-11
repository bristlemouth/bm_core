from enum import Enum


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
