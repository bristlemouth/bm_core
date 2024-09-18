#include <stdbool.h>
#include <stdint.h>

typedef struct {
  uint16_t type;
  uint16_t checksum;
  uint8_t flags;
  uint8_t reserved;
  uint32_t seq_num;
  uint8_t frag_total;
  uint8_t frag_id;
  uint8_t next_header;
} __attribute__((packed)) BCMPHeader;

typedef struct {
  // Microseconds since system has last reset/powered on.
  uint64_t time_since_boot_us;

  // How long to consider this node alive. 0 can mean indefinite, which could be useful for certain applications.
  uint32_t liveliness_lease_dur_s;
} __attribute__((packed)) BCMPHeartbeat;

typedef struct {
  // Node ID of the target node for which the request is being made. (Zeroed = all nodes)
  uint64_t target_node_id;

  // A "unique" ID correlating to a stream of ping requests/replies, usually tied to the process or task ID of the requester
  uint16_t id;

  // Monotonically increasing sequence number relating to the stream identified by id
  uint16_t seq_num;

  // Length of optional payload
  uint16_t payload_len;

  // Optional payload. If used, the reply to this request must send a matching payload to be considered valid
  uint8_t payload[0];
} __attribute__((packed)) BCMPEchoRequest;

typedef struct {
  // Node ID of the responding node
  uint64_t node_id;

  // ID matching that of the request that triggered this reply
  uint16_t id;

  // Sequence number matching that of the request that triggered this reply
  uint16_t seq_num;

  // Length of optional payload
  uint16_t payload_len;

  // Optional payload. Must match with original request payload to be considered valid.
  uint8_t payload[0];
} __attribute__((packed)) BCMPEchoReply;

typedef struct {
  // Node ID of the target node for which the request is being made. (Zeroed = all nodes)
  uint64_t target_node_id;
} __attribute__((packed)) BCMPDeviceInfoRequest;

typedef struct {
  // Node ID of the responding node
  uint64_t node_id;

  // Vendor ID of the hardware module implementing the BM Node functions
  uint16_t vendor_id;

  // Product ID for the hardware module implementing the BM Node functions
  uint16_t product_id;

  // Factory-flashed unique serial number
  uint8_t serial_num[16];

  // Last 4 bytes of git SHA
  uint32_t git_sha;

  // Major Version
  uint8_t ver_major;

  // Minor Version
  uint8_t ver_minor;

  // Revision/Patch Version
  uint8_t ver_rev;

  // Version of the product hardware (0 for don't care)
  uint8_t ver_hw;
} __attribute__((packed)) BCMPDeviceInfo;

typedef struct {
  BCMPDeviceInfo info;

  // Length of the full version string
  uint8_t ver_str_len;

  // Length of device name
  uint8_t dev_name_len;

  // ver_str is immediately followed by dev_name_len
  char strings[0];
} __attribute__((packed)) BCMPDeviceInfoReply;

typedef struct {
  // Node ID of the target node for which the request is being made. (Zeroed = all nodes)
  uint64_t target_node_id;

  // Length of requested caps list
  uint16_t caps_list_len;

  // List of Capability IDs requested from target node(s)
  uint16_t caps_list[0];
} __attribute__((packed)) BCMPProtocolCapsRequest;

typedef struct {
  // Node ID of the responding node
  uint64_t node_id;

  // BCMP revision. Helper - doesn't necessarily provide actionable information about supported features. See attached TLVs
  uint16_t bcmp_rev;

  // Number of capability TLVs to expect in the remainder of this message
  uint16_t caps_count;

  // Sequence of capability TLVs (type, length, value). Points to bcmp_cap_tlv* elements. Each TLV value has a defined structure (defined separately)
  // TODO - Switch to bcmp_cap_tlv caps[0]; when implemented
  uint8_t caps[0];
} __attribute__((packed)) BCMPProtocolCapsReply;

typedef struct {
  // Node ID of the target node for which the request is being made. (Zeroed = all nodes)
  uint64_t target_node_id;
} __attribute__((packed)) BCMPNeighborTableRequest;

typedef struct {
  // Port state up/down
  bool state;

  // What type of port is this?
  // Maps to bm_port_type_e
  uint8_t type;
} __attribute__((packed)) BCMPPortInfo;

typedef struct {
  // Node ID of the responding node
  uint64_t node_id;

  // Length of port list
  uint8_t port_len;

  // How many neighbors are there
  uint16_t neighbor_len;

  // List of local BM ports and basic status information about them
  BCMPPortInfo port_list[0];

  // Followed by neighbor list
  // bcmp_neighbor_info_t
} __attribute__((packed)) BCMPNeighborTableReply;

typedef struct {
  // Neighbor's node_id
  uint64_t node_id;

  // Which port is this neighbor on
  uint8_t port;

  // Is this neighbor online or not?
  uint8_t online;
} __attribute__((packed)) BCMPNeighborInfo;

typedef struct {
  // Node ID of the target node for which the request is being made. (Zeroed = all nodes)
  uint64_t target_node_id;
} __attribute__((packed)) BCMPResourceTableRequest;

typedef struct {
  // Length of resource name
  uint16_t resource_len;
  // Name of resource
  char resource[0];
} __attribute__((packed)) BCMPResource;

typedef struct {
  // Node ID of the responding node
  uint64_t node_id;

  // Number of published topics
  uint16_t num_pubs;

  // Number of subscribed topics
  uint16_t num_subs;

  // List containing information for all resource interests known about this node.
  // The list is comprised of bcmp_resource_t structures that are ordered such that
  // the published resources are listed first, followed by the subscribed resources.
  // The list can be traversed using 0 to (num_pubs - 1) to access the published resources,
  // and num_pubs to (num_pubs + num_subs - 1) to access the subscribed resources.
  uint8_t resource_list[0];
} __attribute__((packed)) BCMPResourceTableReply;

typedef struct {
  // Node ID of the target node for which the request is being made. (Zeroed = all nodes)
  uint64_t target_node_id;

  // Number of option TLVs to expect in the remainder of this message
  uint16_t option_count;

  // TODO - switch to bcmp_proto_opt_tlv options[0]; when defined
  uint8_t options[0];
} __attribute__((packed)) BCMPNeighborProtoRequest;

typedef struct {
  // Node ID of the responding node
  uint64_t node_id;

  // Number of option TLVs to expect in the remainder of this message
  uint16_t option_count;

  // TODO - switch to bcmp_proto_opt_tlv option_list[0]; when defined
  uint8_t option_list[0];
} __attribute__((packed)) BCMPNeighborProtoReply;

typedef struct {
  // Node ID of the target node for which the request is being made.
  uint64_t target_node_id;
  // Node ID of the source node
  uint64_t source_node_id;
  // message payload
  uint8_t payload[0];
} __attribute__((packed)) BCMPSystemTimeHeader;

typedef struct {
  BCMPSystemTimeHeader header;
} __attribute__((packed)) BCMPSystemTimeRequest;

typedef struct {
  BCMPSystemTimeHeader header;
  // time
  uint64_t utc_time_us;
} __attribute__((packed)) BCMPSystemTimeResponse;

typedef struct {
  BCMPSystemTimeHeader header;
  // time
  uint64_t utc_time_us;
} __attribute__((packed)) BCMPSystemTimeSet;

typedef struct {
  // Node ID of the target node for which the request is being made. (Zeroed = all nodes)
  uint64_t target_node_id;
} __attribute__((packed)) BCMPNetStateRequest;

typedef struct {
  // Node ID of the responding node
  uint64_t node_id;

  // TODO - add node info here
} __attribute__((packed)) BCMPNetStateReply;

typedef struct {
  // Node ID of the target node for which the request is being made. (Zeroed = all nodes)
  uint64_t target_node_id;
} __attribute__((packed)) BCMPPowerStateRequest;

typedef struct {
  // Node ID of the responding node
  uint64_t node_id;

  // Length of port list
  uint8_t port_len;

  // List containing power state and information for each port.
  // TODO - switch to  bcmp_port_power port_list[0];
  uint8_t port_list[0];
} __attribute__((packed)) BCMPPowerStateReply;

typedef struct {
  // Node ID of the target node for which the request is being made. (Zeroed = all nodes)
  uint64_t target_node_id;

  // Bypass preventative measures to trigger a reboot (use with caution)
  uint8_t force;
} __attribute__((packed)) BCMPRebootRequest;

typedef struct {
  // Node ID of the responding node
  uint64_t node_id;

  // Request accepted or rejected
  uint8_t ack_nack;

} __attribute__((packed)) BCMPRebootReply;

typedef struct {
  // Node ID of the target node for which the request is being made. (Zeroed = all nodes)
  uint64_t target_node_id;
} __attribute__((packed)) BCMPNetAssertQuiet;

// TODO move this to DFU specific stuff
// DFU stuff goes below
//typedef struct {
//  bm_dfu_frame_header_t header;
//  bm_dfu_event_img_info_t info;
//} __attribute__((packed)) bcmp_dfu_start_t;
//
//typedef struct {
//  bm_dfu_frame_header_t header;
//  bm_dfu_event_chunk_request_t chunk_req;
//} __attribute__((packed)) bcmp_dfu_payload_req_t;
//
//typedef struct {
//  bm_dfu_frame_header_t header;
//  bm_dfu_event_image_chunk_t chunk;
//} __attribute__((packed)) bcmp_dfu_payload_t;
//
//typedef struct {
//  bm_dfu_frame_header_t header;
//  bm_dfu_event_result_t result;
//} __attribute__((packed)) bcmp_dfu_end_t;
//
//typedef struct {
//  bm_dfu_frame_header_t header;
//  bm_dfu_event_result_t ack;
//} __attribute__((packed)) bcmp_dfu_ack_t;
//
//typedef struct {
//  bm_dfu_frame_header_t header;
//  bm_dfu_event_result_t err;
//} __attribute__((packed)) bcmp_dfu_abort_t;
//
//typedef struct {
//  bm_dfu_frame_header_t header;
//  bm_dfu_event_address_t addr;
//} __attribute__((packed)) bcmp_dfu_heartbeat_t;
//
//typedef struct {
//  bm_dfu_frame_header_t header;
//  bm_dfu_event_address_t addr;
//} __attribute__((packed)) bcmp_dfu_reboot_req_t;
//
//typedef struct {
//  bm_dfu_frame_header_t header;
//  bm_dfu_event_address_t addr;
//} __attribute__((packed)) bcmp_dfu_reboot_t;
//
//typedef struct {
//  bm_dfu_frame_header_t header;
//  bm_dfu_event_address_t addr;
//} __attribute__((packed)) bcmp_dfu_boot_complete_t;

typedef enum {
  BCMPAckMessage = 0x00,
  BCMPHeartbeatMessage = 0x01,

  BCMPEchoRequestMessage = 0x02,
  BCMPEchoReplyMessage = 0x03,
  BCMPDeviceInfoRequestMessage = 0x04,
  BCMPDeviceInfoReplyMessage = 0x05,
  BCMPProtocolCapsRequestMessage = 0x06,
  BCMPProtocolCapsReplyMessage = 0x07,
  BCMPNeighborTableRequestMessage = 0x08,
  BCMPNeighborTableReplyMessage = 0x09,
  BCMPResourceTableRequestMessage = 0x0A,
  BCMPResourceTableReplyMessage = 0x0B,
  BCMPNeighborProtoRequestMessage = 0x0C,
  BCMPNeighborProtoReplyMessage = 0x0D,

  BCMPSystemTimeRequestMessage = 0x10,
  BCMPSystemTimeResponseMessage = 0x11,
  BCMPSystemTimeSetMessage = 0x12,

  BCMPNetStateRequestMessage = 0xB0,
  BCMPNetStateReplyMessage = 0xB1,
  BCMPPowerStateRequestMessage = 0xB2,
  BCMPPowerStateReplyMessage = 0xB3,

  BCMPRebootRequestMessage = 0xC0,
  BCMPRebootReplyMessage = 0xC1,
  BCMPNetAssertQuietMessage = 0xC2,

  BCMPConfigGetMessage = 0xA0,
  BCMPConfigValueMessage = 0xA1,
  BCMPConfigSetMessage = 0xA2,
  BCMPConfigCommitMessage = 0xA3,
  BCMPConfigStatusRequestMessage = 0xA4,
  BCMPConfigStatusResponseMessage = 0xA5,
  BCMPConfigDeleteRequestMessage = 0xA6,
  BCMPConfigDeleteResponseMessage = 0xA7,

  BCMPDFUStartMessage = 0xD0,
  BCMPDFUPayloadReqMessage = 0xD1,
  BCMPDFUPayloadMessage = 0xD2,
  BCMPDFUEndMessage = 0xD3,
  BCMPDFUAckMessage = 0xD4,
  BCMPDFUAbortMessage = 0xD5,
  BCMPDFUHeartbeatMessage = 0xD6,
  BCMPDFURebootReqMessage = 0xD7,
  BCMPDFURebootMessage = 0xD8,
  BCMPDFUBootCompleteMessage = 0xD9,
  BCMPDFULastMessageMessage = BCMPDFUBootCompleteMessage,

  BCMPHeaderMessage = 0xFFFF
} BCMPMessageType;
