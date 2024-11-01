#ifndef __MESSAGES_H__
#define __MESSAGES_H__

#include <stdbool.h>
#include <stdint.h>

#include "dfu_message_structs.h"

typedef struct {
  uint16_t type;
  uint16_t checksum;
  uint8_t flags;
  uint8_t reserved;
  uint32_t seq_num;
  uint8_t frag_total;
  uint8_t frag_id;
  uint8_t next_header;
} __attribute__((packed)) BcmpHeader;

typedef struct {
  // Microseconds since system has last reset/powered on.
  uint64_t time_since_boot_us;

  // How long to consider this node alive. 0 can mean indefinite, which could be useful for certain applications.
  uint32_t liveliness_lease_dur_s;
} __attribute__((packed)) BcmpHeartbeat;

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
} __attribute__((packed)) BcmpEchoRequest;

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
} __attribute__((packed)) BcmpEchoReply;

typedef struct {
  // Node ID of the target node for which the request is being made. (Zeroed = all nodes)
  uint64_t target_node_id;
} __attribute__((packed)) BcmpDeviceInfoRequest;

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
} __attribute__((packed)) BcmpDeviceInfo;

typedef struct {
  BcmpDeviceInfo info;

  // Length of the full version string
  uint8_t ver_str_len;

  // Length of device name
  uint8_t dev_name_len;

  // ver_str is immediately followed by dev_name_len
  char strings[0];
} __attribute__((packed)) BcmpDeviceInfoReply;

typedef struct {
  // Node ID of the target node for which the request is being made. (Zeroed = all nodes)
  uint64_t target_node_id;

  // Length of requested caps list
  uint16_t caps_list_len;

  // List of Capability IDs requested from target node(s)
  uint16_t caps_list[0];
} __attribute__((packed)) BcmpProtocolCapsRequest;

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
} __attribute__((packed)) BcmpProtocolCapsReply;

typedef struct {
  // Node ID of the target node for which the request is being made. (Zeroed = all nodes)
  uint64_t target_node_id;
} __attribute__((packed)) BcmpNeighborTableRequest;

typedef struct {
  // Port state up/down
  bool state;

  // What type of port is this?
  // Maps to bm_port_type_e
  uint8_t type;
} __attribute__((packed)) BcmpPortInfo;

typedef struct {
  // Node ID of the responding node
  uint64_t node_id;

  // Length of port list
  uint8_t port_len;

  // How many neighbors are there
  uint16_t neighbor_len;

  // List of local BM ports and basic status information about them
  BcmpPortInfo port_list[0];

  // Followed by neighbor list
  // bcmp_neighbor_info_t
} __attribute__((packed)) BcmpNeighborTableReply;

typedef struct {
  // Neighbor's node_id
  uint64_t node_id;

  // Which port is this neighbor on
  uint8_t port;

  // Is this neighbor online or not?
  uint8_t online;
} __attribute__((packed)) BcmpNeighborInfo;

typedef struct {
  // Node ID of the target node for which the request is being made. (Zeroed = all nodes)
  uint64_t target_node_id;
} __attribute__((packed)) BcmpResourceTableRequest;

typedef struct {
  // Length of resource name
  uint16_t resource_len;
  // Name of resource
  char resource[0];
} __attribute__((packed)) BcmpResource;

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
} __attribute__((packed)) BcmpResourceTableReply;

typedef struct {
  // Node ID of the target node for which the request is being made. (Zeroed = all nodes)
  uint64_t target_node_id;

  // Number of option TLVs to expect in the remainder of this message
  uint16_t option_count;

  // TODO - switch to bcmp_proto_opt_tlv options[0]; when defined
  uint8_t options[0];
} __attribute__((packed)) BcmpNeighborProtoRequest;

typedef struct {
  // Node ID of the responding node
  uint64_t node_id;

  // Number of option TLVs to expect in the remainder of this message
  uint16_t option_count;

  // TODO - switch to bcmp_proto_opt_tlv option_list[0]; when defined
  uint8_t option_list[0];
} __attribute__((packed)) BcmpNeighborProtoReply;

typedef struct {
  // Node ID of the target node for which the request is being made.
  uint64_t target_node_id;
  // Node ID of the source node
  uint64_t source_node_id;
  // message payload
  uint8_t payload[0];
} __attribute__((packed)) BcmpSystemTimeHeader;

typedef struct {
  BcmpSystemTimeHeader header;
} __attribute__((packed)) BcmpSystemTimeRequest;

typedef struct {
  BcmpSystemTimeHeader header;
  // time
  uint64_t utc_time_us;
} __attribute__((packed)) BcmpSystemTimeResponse;

typedef struct {
  BcmpSystemTimeHeader header;
  // time
  uint64_t utc_time_us;
} __attribute__((packed)) BcmpSystemTimeSet;

typedef struct {
  // Node ID of the target node for which the request is being made. (Zeroed = all nodes)
  uint64_t target_node_id;
} __attribute__((packed)) BcmpNetStateRequest;

typedef struct {
  // Node ID of the responding node
  uint64_t node_id;

  // TODO - add node info here
} __attribute__((packed)) BcmpNetStateReply;

typedef struct {
  // Node ID of the target node for which the request is being made. (Zeroed = all nodes)
  uint64_t target_node_id;
} __attribute__((packed)) BcmpPowerStateRequest;

typedef struct {
  // Node ID of the responding node
  uint64_t node_id;

  // Length of port list
  uint8_t port_len;

  // List containing power state and information for each port.
  // TODO - switch to  bcmp_port_power port_list[0];
  uint8_t port_list[0];
} __attribute__((packed)) BcmpPowerStateReply;

typedef struct {
  // Node ID of the target node for which the request is being made. (Zeroed = all nodes)
  uint64_t target_node_id;

  // Bypass preventative measures to trigger a reboot (use with caution)
  uint8_t force;
} __attribute__((packed)) BcmpRebootRequest;

typedef struct {
  // Node ID of the responding node
  uint64_t node_id;

  // Request accepted or rejected
  uint8_t ack_nack;

} __attribute__((packed)) BcmpRebootReply;

typedef struct {
  // Node ID of the target node for which the request is being made. (Zeroed = all nodes)
  uint64_t target_node_id;
} __attribute__((packed)) BcmpNetAssertQuiet;

// TODO move this to DFU specific stuff
// DFU stuff goes below
typedef struct {
 BmDfuFrameHeader header;
 BmDfuEventImgInfo info;
} __attribute__((packed)) BcmpDfuStart;

typedef struct {
 BmDfuFrameHeader header;
 BmDfuEventChunkRequest chunk_req;
} __attribute__((packed)) BcmpDfuPayloadReq;

typedef struct {
 BmDfuFrameHeader header;
 BmDfuEventImageChunk chunk;
} __attribute__((packed)) BcmpDfuPayload;

typedef struct {
 BmDfuFrameHeader header;
 BmDfuEventResult result;
} __attribute__((packed)) BcmpDfuEnd;

typedef struct {
 BmDfuFrameHeader header;
 BmDfuEventResult ack;
} __attribute__((packed)) BcmpDfuAck;

typedef struct {
 BmDfuFrameHeader header;
 BmDfuEventResult err;
} __attribute__((packed)) BcmpDfuAbort;

typedef struct {
 BmDfuFrameHeader header;
 BmDfuEventAddress addr;
} __attribute__((packed)) BcmpDfuHeartbeat;

typedef struct {
 BmDfuFrameHeader header;
 BmDfuEventAddress addr;
} __attribute__((packed)) BcmpDfuRebootReq;

typedef struct {
 BmDfuFrameHeader header;
 BmDfuEventAddress addr;
} __attribute__((packed)) BcmpDfuReboot;

typedef struct {
 BmDfuFrameHeader header;
 BmDfuEventAddress addr;
} __attribute__((packed)) BcmpDfuBootComplete;

/////////////////////////////
/* CONFIGURATION*/
/////////////////////////////
typedef enum {
  BM_CFG_PARTITION_USER,
  BM_CFG_PARTITION_SYSTEM,
  BM_CFG_PARTITION_HARDWARE,
  BM_CFG_PARTITION_COUNT
} BmConfigPartition;

typedef struct {
  // Node ID of the target node for which the request is being made.
  uint64_t target_node_id;
  // Node ID of the source node
  uint64_t source_node_id;
  // message payload
  uint8_t payload[0];
} __attribute__((packed)) BmConfigHeader;

typedef struct {
  BmConfigHeader header;
  // Partition id
  BmConfigPartition partition;
  // String length of the key (without terminator)
  uint8_t key_length;
  // Key string
  char key[0];
} __attribute__((packed)) BmConfigGet;

typedef struct {
  BmConfigHeader header;
  // Partition id
  BmConfigPartition partition;
  // Length of cbor buffer
  uint32_t data_length;
  // cbor buffer
  uint8_t data[0];
} __attribute__((packed)) BmConfigValue;

typedef struct {
  BmConfigHeader header;
  // Partition id
  BmConfigPartition partition;
  // String length of the key (without terminator)
  uint8_t key_length;
  // Length of cbor encoded data buffer
  uint32_t data_length;
  // cbor encoded data
  uint8_t keyAndData[0];
} __attribute__((packed)) BmConfigSet;

typedef struct {
  BmConfigHeader header;
  // Partition id
  BmConfigPartition partition;
} __attribute__((packed)) BmConfigCommit;

typedef struct {
  BmConfigHeader header;
  // Partition id
  BmConfigPartition partition;
} __attribute__((packed)) BmConfigStatusRequest;

typedef struct {
  // String length of the key (without terminator)
  uint8_t key_length;
  // Key string
  char key[0];
} __attribute__((packed)) BmConfigStatusKeyData;

typedef struct {
  BmConfigHeader header;
  // Partition id
  BmConfigPartition partition;
  // True if there are changes to be committed, false otherwise.
  bool committed;
  // Number of keys
  uint8_t num_keys;
  // Key Data
  uint8_t keyData[0];
} __attribute__((packed)) BmConfigStatusResponse;

typedef struct {
  BmConfigHeader header;
  // Partition id
  BmConfigPartition partition;
  // String length of the key (without terminator)
  uint8_t key_length;
  // Key string
  char key[0];
} __attribute__((packed)) BmConfigDeleteKeyRequest;

typedef struct {
  BmConfigHeader header;
  // success
  bool success;
  // Partition id
  BmConfigPartition partition;
  // String length of the key (without terminator)
  uint8_t key_length;
  // Key string
  char key[0];
} __attribute__((packed)) BmConfigDeleteKeyResponse;


typedef enum {
  BcmpAckMessage = 0x00,
  BcmpHeartbeatMessage = 0x01,

  BcmpEchoRequestMessage = 0x02,
  BcmpEchoReplyMessage = 0x03,
  BcmpDeviceInfoRequestMessage = 0x04,
  BcmpDeviceInfoReplyMessage = 0x05,
  BcmpProtocolCapsRequestMessage = 0x06,
  BcmpProtocolCapsReplyMessage = 0x07,
  BcmpNeighborTableRequestMessage = 0x08,
  BcmpNeighborTableReplyMessage = 0x09,
  BcmpResourceTableRequestMessage = 0x0A,
  BcmpResourceTableReplyMessage = 0x0B,
  BcmpNeighborProtoRequestMessage = 0x0C,
  BcmpNeighborProtoReplyMessage = 0x0D,

  BcmpSystemTimeRequestMessage = 0x10,
  BcmpSystemTimeResponseMessage = 0x11,
  BcmpSystemTimeSetMessage = 0x12,

  BcmpNetStateRequestMessage = 0xB0,
  BcmpNetStateReplyMessage = 0xB1,
  BcmpPowerStateRequestMessage = 0xB2,
  BcmpPowerStateReplyMessage = 0xB3,

  BcmpRebootRequestMessage = 0xC0,
  BcmpRebootReplyMessage = 0xC1,
  BcmpNetAssertQuietMessage = 0xC2,

  BcmpConfigGetMessage = 0xA0,
  BcmpConfigValueMessage = 0xA1,
  BcmpConfigSetMessage = 0xA2,
  BcmpConfigCommitMessage = 0xA3,
  BcmpConfigStatusRequestMessage = 0xA4,
  BcmpConfigStatusResponseMessage = 0xA5,
  BcmpConfigDeleteRequestMessage = 0xA6,
  BcmpConfigDeleteResponseMessage = 0xA7,

  BcmpDFUStartMessage = 0xD0,
  BcmpDFUPayloadReqMessage = 0xD1,
  BcmpDFUPayloadMessage = 0xD2,
  BcmpDFUEndMessage = 0xD3,
  BcmpDFUAckMessage = 0xD4,
  BcmpDFUAbortMessage = 0xD5,
  BcmpDFUHeartbeatMessage = 0xD6,
  BcmpDFURebootReqMessage = 0xD7,
  BcmpDFURebootMessage = 0xD8,
  BcmpDFUBootCompleteMessage = 0xD9,
  BcmpDFULastMessageMessage = BcmpDFUBootCompleteMessage,

  BcmpHeaderMessage = 0xFFFF
} BcmpMessageType;

#endif
