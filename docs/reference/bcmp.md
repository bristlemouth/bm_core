# BCMP

BCMP (Bristlemouth Control Messaging Protocol),
which is similar to ICMP,
implements neighbor and resource discovery for the Bristlemouth stack.
The section of code that handles this has the following hierarchy:

```{image} bcmp.png
:align: center
:width: 550
:class: no-scaled-link
```

## Modules
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
  ```c
  #include "packet.h"
  #include "pbuf.h"
  #include "ip_addr.h"
  #include "inet_chksum.h"
  #include "netif.h"
  #include <stddef.h>

  typedef struct {
    struct pbuf *pbuf;
    ip_addr *src;
    ip_addr *dst;
  } NetworkData;

  static void *lwip_get_data(void *payload) {
    NetworkData *ret = (NetworkData *)payload;
    return ret->pbuf->payload;
  }

  static void *lwip_get_src_ip(void *payload) {
    NetworkData *ret = (NetworkData *)payload;
    return ret->src;
  }

  static void *lwip_get_dst_ip(void *payload) {
    NetworkData *ret = (NetworkData *)payload;
    return ret->dst;
  }

  static uint16_t lwip_get_checksum(void *payload, uint32_t size) {
    uint16_t ret = UINT16_MAX;
    NetworkData *data = (NetworkData *)payload;

    ret = ip6_chksum_pseudo(data->pbuf,
                            IP_PROTO_BCMP,
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
