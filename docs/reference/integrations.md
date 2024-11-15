# Integrations

The integrations library is used to implement API that is particular to specific system.
For example,
this may be a way to transmit data down the Bristlemouth network to a node that has cellular/satellite connection,
which could then transmit that data to a backend,
or,
commanding a device down the network to activate an actuator.
The API available in this library will be a created by contributers of the community.

The following sections explain the integrations that are currently available in Bristlemouth:

## Spotter

The spotter integration is used to communicate to Sofar Ocean's spotter buoy technology.
This integration offers 2 functions,
the following are their names and use cases:

```{eval-rst}
.. cpp:function:: BmErr spotter_log(uint64_t target_node_id, const char *file_name, uint8_t print_time, const char *format, ...)

  Publish string data to be stored/logged onto a file on the spotter's SD card.
  This function can also print directly to the spotter's console if the filename parameter is witheld.

  :param target_node_id: The node ID to send this to (0 = all nodes)
  :param file_name: file name to print to (this appends to a file),
                    omit if intention is print to the console
  :param print_time: whether or not the RTC timestamp shall be printed to the file/console
  :param format: printf styled format string

  :returns: BmOK on success, other value on failure
```

```{eval-rst}
.. cpp:function:: BmErr spotter_tx_data(const void *data, uint16_t data_len, BmSerialNetworkType type)

  Transmit data via the Spotter's satellite or cellular connection to the Sofar backend.
  Currently, there are 2 notable limitations on this function.

  - First, only network type `BmNetworkTypeCellularIriFallback`
    shows up in the Sofar Ocean API.
  - Second, the `data_len` has a realistic max of 311 bytes because
    Iridium messages are limited to 340 bytes, and Spotter adds a
    29-byte header.

  :param data: Data to transmit
  :param data_len: Length of the data to transmit
  :param type: Network type to send over. Must be BmNetworkTypeCellularIriFallback.

  :returns: BmOK on success, other value on failure
```

## File Operations

The file operations integration is used to access data to/from a file on another node.
This is beneficial,
because not all nodes on a network may have a file system that can store persistent data,
or may not be in an easily accessible location to retrieve persistently stored data.
The integration offers the following API:

```{eval-rst}
.. cpp:function:: BmErr bm_file_append(uint64_t target_node_id, const char *file_name, const uint8_t *buf, uint16_t len)

  Append data to a file on another node's file system.

  :param target_node_id: The node ID to send this to (0 = all nodes)
  :param file_name: file name to append data to
  :param buf: buffer to append to file
  :param len: length of buffer to append to file

  :returns: BmOK on success, other value on failure
```

## Topology

The topology integration is used to obtain all of the nodes on a network.
This is useful as it can inform an application of the node IDs on a network,
as well as the information of the neighbors on each node,
which port the the neighbors are on as well as the state of those ports.
Topology is obtained by requesting the neighbor information,
done through the BCMP neighbors messaging,
from local nodes
and iterating down the network to all other nodes based on their neighbor information.
A separate thread is utilized to handle a topology event.

```{eval-rst}
.. cpp:function:: BmErr bcmp_topology_start(const BcmpTopoCb callback)

  Start a topology event.
  A callback is invoked once the topology event has finished
  and all of the node's neighbor information has been aquired on the network.

  :param callback: Callback which is invoked after topology event has finished.

  :returns: BmOK on success, other value on failure
```
