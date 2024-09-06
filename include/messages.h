#ifndef __BM_MESSAGES_H__
#define __BM_MESSAGES_H__

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
#define bcmp_header_layout                                                              \
  BCMP_PARSER_16BIT, BCMP_PARSER_16BIT, BCMP_PARSER_8BIT, BCMP_PARSER_8BIT,             \
  BCMP_PARSER_32BIT, BCMP_PARSER_8BIT, BCMP_PARSER_8BIT, BCMP_PARSER_8BIT,
#define bcmp_header_params(l) {l, sizeof(BCMPHeader)}

typedef struct {
  // Microseconds since system has last reset/powered on.
  uint64_t time_since_boot_us;

  // How long to consider this node alive. 0 can mean indefinite, which could be useful for certain applications.
  uint32_t liveliness_lease_dur_s;
} __attribute__((packed)) BCMPHeartbeat;
#define bcmp_heartbeat_layout BCMP_PARSER_64BIT, BCMP_PARSER_32BIT,
#define bcmp_heartbeat_params {{bcmp_heartbeat_layout}, sizeof(BCMPHeartbeat)}

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
#define bcmp_echo_request_layout                                                        \
  BCMP_PARSER_64BIT, BCMP_PARSER_16BIT, BCMP_PARSER_16BIT, BCMP_PARSER_16BIT,
#define bcmp_echo_request_params                                                     \
  {{bcmp_echo_request_layout}, sizeof(BCMPEchoRequest)}

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
#define bcmp_echo_reply_layout                                                          \
  BCMP_PARSER_64BIT, BCMP_PARSER_16BIT, BCMP_PARSER_16BIT, BCMP_PARSER_16BIT,
#define bcmp_echo_reply_params                                                       \
  {{bcmp_echo_reply_layout}, sizeof(BCMPEchoReply)}

typedef struct {
  // Node ID of the target node for which the request is being made. (Zeroed = all nodes)
  uint64_t target_node_id;
} __attribute__((packed)) BCMPDeviceInfoRequest;
#define bcmp_device_info_request_layout BCMP_PARSER_64BIT,
#define bcmp_device_info_request_params                                                        \
  {{bcmp_device_info_request_layout}, sizeof(BCMPDeviceInfoRequest)}

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
#define bcmp_device_info_layout                                                                \
  BCMP_PARSER_64BIT, BCMP_PARSER_16BIT, BCMP_PARSER_16BIT, BCMP_PARSER_8BIT, BCMP_PARSER_8BIT, \
      BCMP_PARSER_8BIT, BCMP_PARSER_8BIT, BCMP_PARSER_8BIT, BCMP_PARSER_8BIT,                  \
      BCMP_PARSER_8BIT, BCMP_PARSER_8BIT, BCMP_PARSER_8BIT, BCMP_PARSER_8BIT,                  \
      BCMP_PARSER_8BIT, BCMP_PARSER_8BIT, BCMP_PARSER_8BIT, BCMP_PARSER_8BIT,                  \
      BCMP_PARSER_8BIT, BCMP_PARSER_8BIT, BCMP_PARSER_32BIT, BCMP_PARSER_8BIT,                 \
      BCMP_PARSER_8BIT, BCMP_PARSER_8BIT, BCMP_PARSER_8BIT,
#define bcmp_device_info_params {{bcmp_device_info_layout}, sizeof(BCMPDeviceInfo)}

typedef struct {
  BCMPDeviceInfo info;

  // Length of the full version string
  uint8_t ver_str_len;

  // Length of device name
  uint8_t dev_name_len;

  // ver_str is immediately followed by dev_name_len
  char strings[0];
} __attribute__((packed)) BCMPDeviceInfoReply;
#define bcmp_device_info_reply_layout                                                          \
  bcmp_device_info_layout, BCMP_PARSER_8BIT, BCMP_PARSER_8BIT,
#define bcmp_device_info_reply_params                                                          \
  {{bcmp_device_info_reply_layout}, sizeof(BCMPDeviceInfoReply)}

typedef struct {
  // Node ID of the target node for which the request is being made. (Zeroed = all nodes)
  uint64_t target_node_id;

  // Length of requested caps list
  uint16_t caps_list_len;

  // List of Capability IDs requested from target node(s)
  uint16_t caps_list[0];
} __attribute__((packed)) BCMPProtocolCapsRequest;
#define bcmp_protocol_caps_request_layout BCMP_PARSER_64BIT, BCMP_PARSER_16BIT,
#define bcmp_protocol_caps_request_params                                                      \
  {{bcmp_protocol_caps_request_layout}, sizeof(BCMPProtocolCapsRequest)}

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
#define bcmp_protocol_caps_reply_layout BCMP_PARSER_64BIT, BCMP_PARSER_16BIT, BCMP_PARSER_16BIT,
#define bcmp_protocol_caps_reply_params                                                        \
  {{bcmp_protocol_caps_reply_layout}, sizeof(BCMPProtocolCapsReply)}

typedef struct {
  // Node ID of the target node for which the request is being made. (Zeroed = all nodes)
  uint64_t target_node_id;
} __attribute__((packed)) BCMPNeighborTableRequest;
#define bcmp_neighbor_table_request_layout BCMP_PARSER_64BIT,
#define bcmp_neighbor_table_request_params                                                     \
  {{bcmp_neighbor_table_request_layout}, sizeof(BCMPNeighborTableRequest)}

typedef struct {
  // Port state up/down
  bool state;

  // What type of port is this?
  // Maps to bm_port_type_e
  uint8_t type;
} __attribute__((packed)) BCMPPortInfo;
#define bcmp_port_info_layout BCMP_PARSER_8BIT, BCMP_PARSER_8BIT,
#define bcmp_port_info_params {{bcmp_port_info_layout}, sizeof(BCMPPortInfo)}

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
#define bcmp_neighbor_table_reply_layout BCMP_PARSER_64BIT, BCMP_PARSER_8BIT, BCMP_PARSER_16BIT,
#define bcmp_neighbor_table_reply_params                                                       \
  {{bcmp_neighbor_table_reply_layout}, sizeof(BCMPNeighborTableReply)}

typedef struct {
  // Neighbor's node_id
  uint64_t node_id;

  // Which port is this neighbor on
  uint8_t port;

  // Is this neighbor online or not?
  uint8_t online;
} __attribute__((packed)) BCMPNeighborInfo;
#define bcmp_neighbor_info_layout BCMP_PARSER_64BIT, BCMP_PARSER_8BIT, BCMP_PARSER_16BIT,
#define bcmp_neighbor_info_params {{bcmp_neighbor_info_layout}, sizeof(BCMPNeighborInfo)}

typedef struct {
  // Node ID of the target node for which the request is being made. (Zeroed = all nodes)
  uint64_t target_node_id;
} __attribute__((packed)) BCMPResourceTableRequest;
#define bcmp_resource_table_request_layout BCMP_PARSER_64BIT,
#define bcmp_resource_table_request_params                                                     \
  {{bcmp_resource_table_request_layout}, sizeof(BCMPResourceTableRequest)}

typedef struct {
  // Length of resource name
  uint16_t resource_len;
  // Name of resource
  char resource[0];
} __attribute__((packed)) BCMPResource;
#define bcmp_resource_layout BCMP_PARSER_16BIT,
#define bcmp_resource_params {{bcmp_resource_layout}, sizeof(BCMPResource)}

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
#define bcmp_resource_table_reply_layout                                                       \
  BCMP_PARSER_64BIT, BCMP_PARSER_16BIT, BCMP_PARSER_16BIT,
#define bcmp_resource_table_reply_params                                                       \
  {{bcmp_resource_table_reply_layout}, sizeof(BCMPResourceTableReply)}

typedef struct {
  // Node ID of the target node for which the request is being made. (Zeroed = all nodes)
  uint64_t target_node_id;

  // Number of option TLVs to expect in the remainder of this message
  uint16_t option_count;

  // TODO - switch to bcmp_proto_opt_tlv options[0]; when defined
  uint8_t options[0];
} __attribute__((packed)) BCMPNeighborProtoRequest;
#define bcmp_neighbor_proto_request_layout BCMP_PARSER_64BIT, BCMP_PARSER_16BIT,
#define bcmp_neighbor_proto_request_params                                                     \
  {{bcmp_neighbor_proto_request_layout}, sizeof(BCMPNeighborProtoRequest)}

typedef struct {
  // Node ID of the responding node
  uint64_t node_id;

  // Number of option TLVs to expect in the remainder of this message
  uint16_t option_count;

  // TODO - switch to bcmp_proto_opt_tlv option_list[0]; when defined
  uint8_t option_list[0];
} __attribute__((packed)) BCMPNeighborProtoReply;
#define bcmp_neighbor_proto_reply_layout BCMP_PARSER_64BIT, BCMP_PARSER_16BIT,
#define bcmp_neighbor_proto_reply_params                                                       \
  {{bcmp_neighbor_proto_reply_layout}, sizeof(BCMPNeighborProtoReply)}

typedef struct {
  // Node ID of the target node for which the request is being made.
  uint64_t target_node_id;
  // Node ID of the source node
  uint64_t source_node_id;
  // message payload
  uint8_t payload[0];
} __attribute__((packed)) BCMPSystemTimeHeader;
#define bcmp_system_time_header_layout BCMP_PARSER_64BIT, BCMP_PARSER_64BIT,
#define bcmp_system_time_header_params                                                         \
  {{bcmp_system_time_header_layout}, sizeof(BCMPSystemTimeHeader)}

typedef struct {
  BCMPSystemTimeHeader header;
} __attribute__((packed)) BCMPSystemTimeRequest;
#define bcmp_system_time_request_layout bcmp_system_time_header_layout,
#define bcmp_system_time_request_params                                                        \
  {{bcmp_system_time_request_layout}, sizeof(BCMPSystemTimeRequest)}

typedef struct {
  BCMPSystemTimeHeader header;
  // time
  uint64_t utc_time_us;
} __attribute__((packed)) BCMPSystemTimeResponse;
#define bcmp_system_time_response_layout bcmp_system_time_header_layout, BCMP_PARSER_64BIT,
#define bcmp_system_time_response_params                                                       \
  {{bcmp_system_time_response_layout}, sizeof(BCMPSystemTimeResponse)}

typedef struct {
  BCMPSystemTimeHeader header;
  // time
  uint64_t utc_time_us;
} __attribute__((packed)) BCMPSystemTimeLayout;
#define bcmp_system_time_set_layout bcmp_system_time_header_layout, BCMP_PARSER_64BIT,
#define bcmp_system_time_set_params                                                            \
  {{bcmp_system_time_set_layout}, sizeof(BCMPSystemTimeLayout)}

typedef struct {
  // Node ID of the target node for which the request is being made. (Zeroed = all nodes)
  uint64_t target_node_id;
} __attribute__((packed)) BCMPNetstatRequest;
#define bcmp_netstat_request_layout BCMP_PARSER_64BIT,
#define bcmp_netstat_request_params {{bcmp_netstat_request_layout}, sizeof(BCMPNetstatRequest)}

typedef struct {
  // Node ID of the responding node
  uint64_t node_id;

  // TODO - add node info here
} __attribute__((packed)) BCMPNetstatReply;
#define bcmp_netstat_reply_layout BCMP_PARSER_64BIT,
#define bcmp_netstat_reply_params {{bcmp_netstat_reply_layout}, sizeof(BCMPNetstatReply)}

typedef struct {
  // Node ID of the target node for which the request is being made. (Zeroed = all nodes)
  uint64_t target_node_id;
} __attribute__((packed)) BCMPPowerStateRequest;
#define bcmp_power_state_request_layout BCMP_PARSER_64BIT,
#define bcmp_power_state_request_params                                                        \
  {{bcmp_power_state_request_layout}, sizeof(BCMPPowerStateRequest)}

typedef struct {
  // Node ID of the responding node
  uint64_t node_id;

  // Length of port list
  uint8_t port_len;

  // List containing power state and information for each port.
  // TODO - switch to  bcmp_port_power port_list[0];
  uint8_t port_list[0];
} __attribute__((packed)) BCMPPowerStateReply;
#define bcmp_power_state_reply_layout BCMP_PARSER_64BIT, BCMP_PARSER_8BIT,
#define bcmp_power_state_reply_params                                                          \
  {{bcmp_power_state_reply_layout}, sizeof(BCMPPowerStateReply)}

typedef struct {
  // Node ID of the target node for which the request is being made. (Zeroed = all nodes)
  uint64_t target_node_id;

  // Bypass preventative measures to trigger a reboot (use with caution)
  uint8_t force;
} __attribute__((packed)) BCMPRebootRequest;
#define bcmp_reboot_request_layout BCMP_PARSER_64BIT, BCMP_PARSER_8BIT,
#define bcmp_reboot_request_params {{bcmp_reboot_request_layout}, sizeof(BCMPRebootRequest)}

typedef struct {
  // Node ID of the responding node
  uint64_t node_id;

  // Request accepted or rejected
  uint8_t ack_nack;

} __attribute__((packed)) BCMPRebootReply;
#define bcmp_reboot_reply_layout BCMP_PARSER_64BIT, BCMP_PARSER_8BIT,
#define bcmp_reboot_reply_params {{bcmp_reboot_reply_layout}, sizeof(BCMPRebootReply)}

typedef struct {
  // Node ID of the target node for which the request is being made. (Zeroed = all nodes)
  uint64_t target_node_id;
} __attribute__((packed)) BCMPNetAssertQuiet;
#define bcmp_net_assert_quiet_layout BCMP_PARSER_64BIT,
#define bcmp_net_assert_quiet_params                                                           \
  {{bcmp_net_assert_quiet_layout}, sizeof(BCMPNetAssertQuiet)}

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
  BCMP_ACK = 0x00,
  BCMP_HEARTBEAT = 0x01,

  BCMP_ECHO_REQUEST = 0x02,
  BCMP_ECHO_REPLY = 0x03,
  BCMP_DEVICE_INFO_REQUEST = 0x04,
  BCMP_DEVICE_INFO_REPLY = 0x05,
  BCMP_PROTOCOL_CAPS_REQUEST = 0x06,
  BCMP_PROTOCOL_CAPS_REPLY = 0x07,
  BCMP_NEIGHBOR_TABLE_REQUEST = 0x08,
  BCMP_NEIGHBOR_TABLE_REPLY = 0x09,
  BCMP_RESOURCE_TABLE_REQUEST = 0x0A,
  BCMP_RESOURCE_TABLE_REPLY = 0x0B,
  BCMP_NEIGHBOR_PROTO_REQUEST = 0x0C,
  BCMP_NEIGHBOR_PROTO_REPLY = 0x0D,

  BCMP_SYSTEM_TIME_REQUEST = 0x10,
  BCMP_SYSTEM_TIME_RESPONSE = 0x11,
  BCMP_SYSTEM_TIME_SET = 0x12,

  BCMP_NET_STAT_REQUEST = 0xB0,
  BCMP_NET_STAT_REPLY = 0xB1,
  BCMP_POWER_STAT_REQUEST = 0xB2,
  BCMP_POWER_STAT_REPLY = 0xB3,

  BCMP_REBOOT_REQUEST = 0xC0,
  BCMP_REBOOT_REPLY = 0xC1,
  BCMP_NET_ASSERT_QUIET = 0xC2,

  BCMP_CONFIG_GET = 0xA0,
  BCMP_CONFIG_VALUE = 0xA1,
  BCMP_CONFIG_SET = 0xA2,
  BCMP_CONFIG_COMMIT = 0xA3,
  BCMP_CONFIG_STATUS_REQUEST = 0xA4,
  BCMP_CONFIG_STATUS_RESPONSE = 0xA5,
  BCMP_CONFIG_DELETE_REQUEST = 0xA6,
  BCMP_CONFIG_DELETE_RESPONSE = 0xA7,

  BCMP_DFU_START = 0xD0,
  BCMP_DFU_PAYLOAD_REQ = 0xD1,
  BCMP_DFU_PAYLOAD = 0xD2,
  BCMP_DFU_END = 0xD3,
  BCMP_DFU_ACK = 0xD4,
  BCMP_DFU_ABORT = 0xD5,
  BCMP_DFU_HEARTBEAT = 0xD6,
  BCMP_DFU_REBOOT_REQ = 0xD7,
  BCMP_DFU_REBOOT = 0xD8,
  BCMP_DFU_BOOT_COMPLETE = 0xD9,
  BCMP_DFU_LAST_MESSAGE = BCMP_DFU_BOOT_COMPLETE,
} BCMPMessageType;

#endif
