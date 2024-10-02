# BCMP

BCMP (Bristlemouth Control Messaging Protocol),
which is similar to ICMP,
implements neighbor and resource discovery for the Bristlemouth stack.
The section of code that handles this has the following structure:

```{image} bcmp.png
:align: center
:width: 1000
:class: no-scaled-link
```

## Modules
The following is an explanation of how these modules function:

- BCMP
  - Responsible for initializing all of the BCMP message modules
- Reply/Request Message Modules
  - Messages as defined in [Network Discovery and Formation](https://bristlemouth.notion.site/Networking-Overview-17bce3252c9a42fdb232c06b5e00d4cd?pvs=97#121e42806c3d4c7dbd296be3c87d7131)
  in the networking overview found in the Bristlemouth Wiki
  - Configures the necessary information that must be passed down to the packet module,
  this includes:
    - Size of the message type structure
    - If this message is a sequenced reply
    - If this message is a sequenced request
    - A callback function for incoming messages that match this message type
  - The available messages structures can be found under `include/messages.h`
- Packet Module
  - Used to process (incoming) and serialize (outgoing) messages
  - All data sent over Bristlemouth is sent in little endian format
  (contrary to network byte order which is big endian),
  this module is also responsible for ensuring the data sent and received is in the correct endianness
    - Outgoing messages will always be little-endian
    - Incoming messages will be little-endian until the process callback is handled,
    then the message will be in the endianness of the system
  - Logic that handles when messages require sequenced replies and requests
  - IP stack related functions are passed into the initialization of this module,
  they are respectively described as follows:
    - `src_ip`
      - Callback function to obtain a pointer to the source IP address of the message
    - `dst_ip`
      - Callback function to obtain a pointer to the destination IP address of the message
    - `data`
      - Callback function to obtain a pointer to the payload of the message
    - `checksum`
      - Callback function to calculate a 16bit checksum of the message
  - An example of how to initialize this module utilizing LWIP:
  (lwip_packet_snippet)=
  ```c
  #include "packet.h"
  #include "pbuf.h"
  #include "ip_addr.h"
  #include "inet_chksum.h"
  #include "netif.h"
  #include <stddef.h>

  typedef struct {
    struct pbuf *pbuf;
    const ip_addr *src;
    const ip_addr *dst;
  } NetworkData;

  static void *lwip_get_data(void *payload) {
    NetworkData *ret = (NetworkData *)payload;
    return (void *)ret->pbuf->payload;
  }

  static void *lwip_get_src_ip(void *payload) {
    NetworkData *ret = (NetworkData *)payload;
    return (void *)ret->src;
  }

  static void *lwip_get_dst_ip(void *payload) {
    NetworkData *ret = (NetworkData *)payload;
    return (void *)ret->dst;
  }

  static uint16_t lwip_get_checksum(void *payload, uint32_t size) {
    uint16_t ret = UINT16_MAX;
    NetworkData *data = (NetworkData *)payload;

    ret = ip6_chksum_pseudo(data->pbuf,
                            IpProtoBcmp,
                            size,
                            (ip_addr *)lwip_get_src_ip(payload),
                            (ip_addr *)lwip_get_dst_ip(payload));

    return ret;
  }

  void example_lwip_init(void) {
    if (packet_init(lwip_get_src_ip, lwip_get_dst_ip,
                      lwip_get_data, lwip_get_checksum) == BmOK) {
      // Initialize other things maybe related to transmissions and receive handling that properly abstracts lwip
      // These things might pertain to how data is passed through to these other interfaces
      // to ensure that NetworkData is used across all transmit/receive functionality
      ...
    }
  }
  ```
- IP Stack Abstraction
  - Responsible for setting up the packet module (via `packet_init`,
  see code snippet [above](#lwip_packet_snippet))
  - For BCMP, since it is built within the network layer as akin to ICMP,
  the utilized API are as which must be implemented by the IP stack are as follows:
    - `void bm_ip_rx_cleanup(void *payload)`
    - `void *bm_ip_tx_new(const void *dst, uint32_t size)`
    - `BmErr bm_ip_tx_copy(void *payload, const void *data, uint32_t size, uint32_t offset)`
    - `BmErr bm_ip_tx_perform(void *payload, const void *dst)`
    - `void bm_ip_tx_cleanup(void *payload)`
  - Providing transmission abstract calls for allocating a new item,
  copying raw data to that item,
  performing a transmission of that item
  cleaning up that item
  - Providing receiving abstract calls for cleaning up an item after it was processed,
  and internally providing a callback for the IP stack

## Messages
The following are a list of supported messages
and what they are responsible for:

- Heartbeat
  - Responsible for telling neighbors (devices directly connected to) that the node is active
  - Incoming messages are received and information is requested to know more about neighbors
    - Information is only requested once a new neighbor is seen (or has been re-booted)
    - Monitors last heartbeat a neighbor has sent
- Device Information
  - Request device information from another node
    - This includes:
      - Node ID
      - Vendor ID
      - Product ID
      - Git SHA
      - Hardware Revision
      - Device Name
      - Firmware Version Information
        - Semantic Versioning
        - Custom String
  - Replying to device information requests with the above device information
  - This message is useful for understanding what other types of nodes are available on the network
- Ping
  - Send a ping request to a target node id
  - Reply to a ping request from the targeted node id
- Resource Discovery
  % TODO: place links to middleware documentation here
  - Request a node's current publishing and subscription topics,
  please see middleware for more information on this
  - Reply to a request with the published/subscribed topics
  - This can be helpful for understanding what types messages that a node might be interested in obtaining more information from
- Neighbor Table
  - Request neighbor table information from a node
    - This includes each neighbors':
      - Node ID
      - Port they are connected to
      - If they are online
  - Reply to a request for neighbor table information from a node
  - This is useful for applications such as discovering the topology of a whole Bristlemouth network
  - Please see the [Neighbor Discovery](https://bristlemouth.notion.site/Networking-Overview-17bce3252c9a42fdb232c06b5e00d4cd?pvs=97#87a06e2a1bfa4cfda5f709316e3b659e)
  section of the Networking Overview Documentation on Bristlemouth for a more in depth explanation
