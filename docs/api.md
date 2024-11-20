# API

Minimal API for the integrator is the philosophy behind Bristlemouth.
The API required is as follows.

---

## Network Interface

Bristlemouth requires 10BASE-T1L transceiver in order to communicate with other devices on the network.
Currently the supported drivers for the 10BASE-T1L transceiver are as follows:

- ADIN2111

### ADIN2111

In order to configure the ADIN2111 driver the following is needed,

<!--- Infomration regarding initializing the adin driver here --->

---


## Bristlemouth Stack

The bristlemouth stack is configured with the following,

<!--- Infomration regarding initializing the bm stack here --->

---

## Configuration

Bristlemouth offers device configurations that can be accessed over the network,
or locally on a device.
Configuration parameters are stored in key value pairs,
it can be thought of in the following format:

```
"key" : value
```

Key is a string.
Value can be one of the following types:

- `UINT32` 32 bit unsigned integer
- `INT32` 32 bit signed integer
- `FLOAT` 32 bit floating point value
- `STR` character string
- `BYTES` 8 bit integer buffer

Some examples of how these configuration parameters can be used within a custom application are as follows:

- Set the sampling rate on how often a device takes readings of a sensor
- Enabling and disabling certain features on the device
- Configuring minimum and maximum sensor readings on a device

In order to be able to configure a device over the network or locally on the device,
integrators must implement the required API to write/read to a non-volatile memory location.
This is done by following this guide:
<!--- Put link to Victor's guide here! --->

Configuration values can be stored in three separate partitions:

1. System
2. User
3. Hardware

---

### Local API
The following header file path must be included when calling local configuration API:

```
#include "configuration.h"
```

All of the following API are utilized to set configuration values on the local device.\
Public API supported by the configuration module is as follows:

```{eval-rst}
.. cpp:function:: bool get_config_uint(BmConfigPartition partition, const char *key, \
                                       size_t key_len, uint32_t *value);

  Get a configuration value that is an unsigned 32 bit integer.

  :param partition: Partition to get key from
  :param key: NULL terminated key name
  :param key_len: Length of key name
  :param value: Value of configuration key requested

  :returns: true if configuration key is found and is of the right type, false otherwise
```

```{eval-rst}
.. cpp:function:: bool get_config_int(BmConfigPartition partition, const char *key, \
                                       size_t key_len, int32_t *value);

  Get a configuration value that is a signed 32 bit integer.

  :param partition: Partition to get key from
  :param key: NULL terminated key name
  :param key_len: Length of key name
  :param value: Value of configuration key requested

  :returns: true if configuration key is found and is of the right type, false otherwise
```

```{eval-rst}
.. cpp:function:: bool get_config_float(BmConfigPartition partition, const char *key, \
                                       size_t key_len, float *value);

  Get a configuration value that is a 32 bit float.

  :param partition: Partition to get key from
  :param key: NULL terminated key name
  :param key_len: Length of key name
  :param value: Value of configuration key requested

  :returns: true if configuration key is found and is of the right type, false otherwise
```

```{eval-rst}
.. cpp:function:: bool get_config_string(BmConfigPartition partition, const char *key, \
                                         size_t key_len, char *value, size_t *value_len);

  Get a configuration value that is a string.

  :param partition: Partition to get key from
  :param key: NULL terminated key name
  :param key_len: Length of key name
  :param value: Value of configuration key requested
  :param value_len: Length of the value to obtain in bytes

  :returns: true if configuration key is found and is of the right type, false otherwise
```

```{eval-rst}
.. cpp:function:: bool get_config_buffer(BmConfigPartition partition, const char *key, \
                                         size_t key_len, uint8_t *value, size_t *value_len);

  Get a configuration value that is an 8 bit buffer.

  :param partition: Partition to get key from
  :param key: NULL terminated key name
  :param key_len: Length of key name
  :param value: Value of configuration key requested
  :param value_len: Length of the value to obtain in bytes

  :returns: true if configuration key is found and is of the right type, false otherwise
```

```{eval-rst}
.. cpp:function:: bool get_config_cbor(BmConfigPartition partition, const char *key, \
                                       size_t key_len, uint8_t *value, size_t *value_len);

  Get the cbor encoded buffer for a given key.

  :param partition: Partition to get key from
  :param key: NULL terminated key name
  :param key_len: Length of key name
  :param value: Value of configuration key requested
  :param value_len: Length of the value to obtain in bytes

  :returns: true if configuration key is found, false otherwise
```

```{eval-rst}
.. cpp:function:: bool set_config_uint(BmConfigPartition partition, const char *key, \
                                       size_t key_len, uint32_t value);

  Set a configuration value that is an unsigned 32 bit integer.

  :param partition: Partition to set key to
  :param key: NULL terminated key name
  :param key_len: Length of key name
  :param value: Value of configuration key requested

  :returns: true if configuration key is found and is set properly, false otherwise
```

```{eval-rst}
.. cpp:function:: bool set_config_int(BmConfigPartition partition, const char *key, \
                                       size_t key_len, int32_t value);

  Set a configuration value that is a signed 32 bit integer.

  :param partition: Partition to set key to
  :param key: NULL terminated key name
  :param key_len: Length of key name
  :param value: Value of configuration key requested

  :returns: true if configuration key is found and is set properly, false otherwise
```

```{eval-rst}
.. cpp:function:: bool set_config_float(BmConfigPartition partition, const char *key, \
                                       size_t key_len, float value);

  Set a configuration value that is a 32 bit float.

  :param partition: Partition to set key to
  :param key: NULL terminated key name
  :param key_len: Length of key name
  :param value: Value of configuration key requested

  :returns: true if configuration key is found and is set properly, false otherwise
```

```{eval-rst}
.. cpp:function:: bool set_config_string(BmConfigPartition partition, const char *key, \
                                         size_t key_len, char *value, size_t *value_len);

  Set a configuration value that is a string.

  :param partition: Partition to set key to
  :param key: NULL terminated key name
  :param key_len: Length of key name
  :param value: Value of configuration key requested
  :param value_len: Length of the value to obtain in bytes

  :returns: true if configuration key is found and is set properly, false otherwise
```

```{eval-rst}
.. cpp:function:: bool set_config_buffer(BmConfigPartition partition, const char *key, \
                                         size_t key_len, uint8_t *value, size_t *value_len);

  Set a configuration value that is an 8 bit buffer.

  :param partition: Partition to set key to
  :param key: NULL terminated key name
  :param key_len: Length of key name
  :param value: Value of configuration key requested
  :param value_len: Length of the value to obtain in bytes

  :returns: true if configuration key is found and is set properly, false otherwise
```

```{eval-rst}
.. cpp:function:: bool set_config_cbor(BmConfigPartition partition, const char *key, \
                                       size_t key_len, uint8_t *value, size_t *value_len);

  Set the cbor encoded buffer for a given key,
  the value must be encoded before passing it into this API with the appropriate cbor API:

  - cbor_encode_uint

  - cbor_encode_int

  - cbor_encode_float

  - cbor_encode_text_string

  - cbor_encode_byte_string

  or by obtaining already encoded data in some other way

  :param partition: Partition to set key to
  :param key: NULL terminated key name
  :param key_len: Length of key name
  :param value: Value of configuration key requested
  :param value_len: Length of the value to obtain in bytes

  :returns: true if configuration key is found and is set properly, false otherwise
```

```{eval-rst}
.. cpp:function:: bool remove_key(BmConfigPartition partition, const char *key, size_t key_len)

  Remove a key pair value from a selected partition.

  :param partition: Partition to remove key from
  :param key: NULL terminated key name
  :param key_len: Length of key name

  :returns: true if configuration key is found and is removed properly, false otherwise
```

```{eval-rst}
.. cpp:function:: bool save_config(BmConfigPartition partition, bool restart);

  Save updated values to a partition,
  this also can restart the device if requested

  :param partition: Partition to save
  :param restart: Whether to restart the device after saving or not

  :returns: true if partition was saved, false otherwise
```

```{eval-rst}
.. cpp:function:: bool needs_commit(BmConfigPartition partition);

  Determines whether or not configuration values have been updated in RAM,
  but have not been saved to NVM.

  :param partition: Partition to check if it needs to be saved

  :returns: true if values need to be saved, false otherwise
```

```{eval-rst}
.. cpp:function:: const ConfigKey* get_stored_keys(BmConfigPartition partition, \
                                                   uint8_t *num_stored_keys);

  Get the stored key pair values from the specified partition.
  Also obtains the number of stored keys in partition.

  :param partition: Partition to obtain the key pair values from
  :param num_stored_keys: Pointer that is assigned the number of keys in the specified partition

  :returns: The key pair values of the requested partition
```

### Local Enumerations

```{eval-rst}
.. cpp:enum:: ConfigDataTypes

  Configuration data types.

  .. cpp:enumerator:: UINT32
  .. cpp:enumerator:: INT32
  .. cpp:enumerator:: FLOAT
  .. cpp:enumerator:: STR
  .. cpp:enumerator:: BYTES

```

### Local Type Definitions

```{eval-rst}
.. cpp:struct:: ConfigKey

  Configuration key that will be stored in NVM. \
  This structure's data is packed.

  .. cpp:member:: char key_buf[MAX_KEY_LEN_BYTES]

    Buffer to hold the key

  .. cpp:member:: size_t key_len

    Length of the specified key

  .. cpp:member:: ConfigDataTypes value_type

    Type of value that the key holds
```

---

### Network API
Configuration messages are sent over the Bristlemouth Control Messaging Protocol (BCMP).
Please see the [following](https://bristlemouth.notion.site/Networking-Overview-17bce3252c9a42fdb232c06b5e00d4cd?pvs=97#121e42806c3d4c7dbd296be3c87d7131)
guide for further information on BCMP.

The following header file path must be included when calling BCMP configuration API:

```
#include "messages/config.h"
```

Public API supported by the BCMP configuration module is as follows:


```{eval-rst}
.. cpp:function:: bool bcmp_config_get(uint64_t target_node_id, BmConfigPartition partition, \
                                       size_t key_len, const char *key, BmErr *err, \
                                       BmErr (*reply_cb)(uint8_t *))

  Get a configuration value from a target node,
  in a selected partition by key name.

  :param target_node_id: Target node id to obtain config value
  :param partition: Partition to obtain configuration from
  :param key_len: Length of the key name
  :param key: Pointer to the key name string
  :param err: Error value passed in as a pointer
  :param reply_cb: Callback when a reply is received over the bus, can be NULL

  :returns: true if get message is sent properly, false otherwise
```

```{eval-rst}
.. cpp:function:: bool bcmp_config_set(uint64_t target_node_id, BmConfigPartition partition, \
                                       size_t key_len, const char *key, BmErr *err, \
                                       BmErr (*reply_cb)(uint8_t *))

  Set a configuration value on a target node,
  in a selected partition by key name.

  :param target_node_id: Target node id to set config value
  :param partition: Partition to set configuration
  :param key_len: Length of the key name
  :param key: Pointer to the key name string
  :param err: Error value passed in as a pointer
  :param reply_cb: Callback when a reply is received over the bus, can be NULL

  :returns: true if set message is sent properly, false otherwise
```

```{eval-rst}
.. cpp:function:: bool bcmp_config_commit(uint64_t target_node_id, BmConfigPartition partition, \
                                          BmErr *err)

  Commit a configuration partition on a target node.
  This saves any updated configuration values and
  resets the device.

  :param target_node_id: Target node id to commit the configuration partition to
  :param partition: Partition to commit
  :param err: Error value passed in as a pointer

  :returns: true if commit message is sent properly, false otherwise
```

```{eval-rst}
.. cpp:function:: bool bcmp_config_status_request(uint64_t target_node_id, \
                                                  BmConfigPartition partition, BmErr *err, \
                                                  BmErr (*reply_cb)(uint8_t *))

  Get all of the key names from a target node's partition.

  :param target_node_id: Target node id to obtaion the key names from
  :param partition: Configuration partition to obtain the key names from
  :param err: Error value passed in as a pointer
  :param reply_cb: Callback when a reply is received over the bus, can be NULL

  :returns: true if status message is sent properly, false otherwise
```

```{eval-rst}
.. cpp:function:: bool bcmp_config_del_key(uint64_t target_node_id, BmConfigPartition partition, \
                                           size_t key_len, const char *key, \
                                           BmErr (*reply_cb)(uint8_t *))

  Delete a key from a target node's configuration partition.

  :param target_node_id: Target node id to delete config value
  :param partition: Partition to delete configuration value from
  :param key_len: Length of the key name
  :param key: Pointer to the key name string
  :param reply_cb: Callback when a reply is received over the bus, can be NULL

  :returns: true if delete message is sent properly, false otherwise
```

## Time
Bristlemouth offers a time API that can be used to obtain or set the current time on a device.

Time messages are sent over the Bristlemouth Control Messaging Protocol (BCMP).

The following header file path must be included when calling BCMP time API:

```
#include "messages/time.h"
```

Public API supported by the BCMP time module is as follows:

```{eval-rst}
.. cpp:function:: BmErr bcmp_time_get_time(uint64_t target_node_id)

  Get the current time from a target node.

  :param target_node_id: Target node id to obtain the current time from

  :returns: BmOK if the get message was sent successfully, BmErr otherwise
```

```{eval-rst}
.. cpp:function:: BmErr bcmp_time_set_time(uint64_t target_node_id, uint64_t utc_us)

  Set the current time on a target node.

  :param target_node_id: Target node id to set the current time on
  :param utc_us: Time to set on the target node represented in microseconds since epoch

  :returns: BmOK if the set message was sent successfully, BmErr otherwise
```

## PubSub

Bristlemouth offers the ability to subscribe and publish to topics on the network.
A topic is a unique string that nodes on the network communicate over.
Pubsub is apart of the [Middleware Protocol](https://bristlemouth.notion.site/Middleware-Protocol-0f2bdf9abaca49a488fbe52e6a92cf96)
on the Bristlemouth stack.
Middleware is built on top of UDP
and the publish subscribe capabilities are associated with port 4321.

Topics are subscribed to,
and then added to the resource list in `bcmp/resource_discovery.c`,
which can be used by other nodes to determine the subscribed to topics of interest.
The following is an example of how to subscribe to a topic of interest:

```
#define sub_topic "sub/example"

static void example_sub_cb(uint64_t node_id, const char *topic,
                           uint16_t topic_len, const uint8_t *data,
                           uint16_t data_len, uint8_t type, uint8_t version)
{
    ...
}

int main(void) {

  ...

  bm_sub(sub_topic, example_sub_cb);

  ...

}
```

Multiple callbacks are able to be tied to the same subscription,
allowing multiple applications to utilize the same topic.
API is also available to unsubscribe to topics.

In order to use the API required by the pubsub module,
the following header must be included:

```
#include "pubsub.h"
```

### API

```{eval-rst}
.. cpp:function:: bool bm_sub(const char *topic, const BmPubSubCb callback)

  Subscribe to a specific string topic with callback

  :param *topic: topic string to subscribe to
  :param callback: callback function to call when data is received on this topic

  :returns: BmOk if able to properly subscribe to topic, BmErr otherwise
```

```{eval-rst}
.. cpp:function:: bool bm_sub_wl(const char *topic, uint16_t topic_len, \
                                 const BmPubSubCb callback)

  Subscribe to a specific string topic with callback (while providing the topic
  length)

  :param *topic: topic string to subscribe to
  :param topic_len: length of topic string
  :param callback: callback function to call when data is received on this topic

  :returns: BmOk if able to properly subscribe to topic, BmErr otherwise
```

```{eval-rst}
.. cpp:function:: bool bm_unsub(const char *topic, const BmPubSubCb callback)

  Unsubscribe to a specific string topic with callback

  :param *topic: topic string to unsubscribe from
  :param callback: callback function to call when data is received on this topic

  :returns: BmOk if able to properly unsubscribe from topic, BmErr otherwise
```

```{eval-rst}
.. cpp:function:: bool bm_unsubwl(const char *topic, uint16_t topic_len,
                                  const BmPubSubCb callback)

  Unsubscribe to a specific string topic with callback (while providing the topic
  length)

  :param *topic: topic string to unsubscribe from
  :param topic_len: length of topic string
  :param callback: callback function to call when data is received on this topic

  :returns: BmOk if able to properly unsubscribe from topic, BmErr otherwise
```

```{eval-rst}
.. cpp:function:: bool bm_pub(const char *topic, const void *data, \
                              uint16_t len, const uint8_t type, \
                              uint8_t version)

  Publish data to specific topic

  :param *topic: topic string to publish to
  :param data: data to publish
  :param len: length of data to publish
  :param type: type of data to publish, this is defined by the integrator (not currently used)
  :param version: version of the data to publish, this is defined by integrator (not currently used)

  :returns: BmOk if able to properly publish to topic, BmErr otherwise
```

```{eval-rst}
.. cpp:function:: bool bm_pubwl(const char *topic, uint16_t topic_len, \
                                const void *data, uint16_t len, \
                                const uint8_t type, uint8_t version)

  Publish data to specific topic (while providing the topic length)

  :param *topic: topic string to publish to
  :param topic_len: length of topic string
  :param data: data to publish
  :param len: length of data to publish
  :param type: type of data to publish, this is defined by the integrator (not currently used)
  :param version: version of the data to publish, this is defined by integrator (not currently used)

  :returns: BmOk if able to properly publish to topic, BmErr otherwise
```
