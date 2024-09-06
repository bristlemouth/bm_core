#include "packet.h"
#include <stddef.h>

/*!
 @brief Update Function To Parse And Handle Incoming Message

 @details This is to be ran on incoming messages, the function
          serializes the header from the received message and then
          reports the payload to the associated parser item. The parser
          is then responsible for serializing the payload.

 @param pbuf lwip pbuf incoming message
 @param src ip address of source message
 @param dst ip address of destination message

 @return BmOK on success
 @return BmError on failure
 */
BMErr parse(BCMPParseIngest ingest, BCMPParserItem *items, uint32_t size) {
  BMErr err = BmEINVAL;
  BCMPParserData data;

  data.header = (BCMPHeader *)ingest.pbuf->payload;
  data.payload = (uint8_t *)(ingest.pbuf->payload + sizeof(BCMPHeader));
  data.pbuf = ingest.pbuf;
  data.src = ingest.src;
  data.dst = ingest.dst;
  data.ingress_port = ((ingest.src->addr[1] >> 8) & 0xFF);
  err = serialize(data.header, BCMP_HEADER);

  for (uint32_t i = 0; i < size; i++) {
    if (data.header->type == items[i].type) {
      if (items[i].cb && items[i].cb(data) != BmOK) {
        //TODO log something clever here
      } else {
        break;
      }
    }
  }

  return err;
}

/*!
 @brief Serialize A BCMP Message In Little Endian Format

 @details Serializes an incoming/outgoing message into an associated structure
          see messages.h for potential structures. This allows incoming and
          outgoing messages to be in a consistent endianess.

 @param buf buffer to be serialed
 @param info layout of the buffer to be serialized, this would be the format
             of the incoming or outgoing structure

 @return BmOK on success
 @return BmError on failure
 */
BMErr serialize(void *buf, BCMPMessageType type) {
  BMErr err = BmEINVAL;
  uint32_t i = 0;

  if (buf) {
    err = BmOK;
    if (is_big_endian()) {
      switch (type) {
      case BCMP_ACK:
        break;
      case BCMP_HEARTBEAT: {
        BCMPHeartbeat *hb = (BCMPHeartbeat *)buf;
        swap_64bit(&hb->time_since_boot_us);
        swap_32bit(&hb->liveliness_lease_dur_s);
      } break;
      case BCMP_ECHO_REQUEST: {
        BCMPEchoRequest *request = (BCMPEchoRequest *)buf;
        swap_64bit(&request->target_node_id);
        swap_16bit(&request->id);
        swap_32bit(&request->seq_num);
        swap_16bit(&request->payload_len);
      } break;
      case BCMP_ECHO_REPLY: {
        BCMPEchoReply *reply = (BCMPEchoReply *)buf;
        swap_64bit(&reply->node_id);
        swap_16bit(&reply->id);
        swap_16bit(&reply->seq_num);
        swap_16bit(&reply->payload_len);
      } break;
      case BCMP_DEVICE_INFO_REQUEST: {
        BCMPDeviceInfoRequest *request = (BCMPDeviceInfoRequest *)buf;
        swap_64bit(&request->target_node_id);
      } break;
      case BCMP_DEVICE_INFO_REPLY: {
        BCMPDeviceInfoReply *reply = (BCMPDeviceInfoReply *)buf;
        swap_64bit(&reply->info.node_id);
        swap_16bit(&reply->info.vendor_id);
        swap_16bit(&reply->info.product_id);
        swap_32bit(&reply->info.git_sha);
      } break;
      case BCMP_PROTOCOL_CAPS_REQUEST: {
        BCMPProtocolCapsRequest *request = (BCMPProtocolCapsRequest *)buf;
        swap_64bit(&request->target_node_id);
        swap_16bit(&request->caps_list_len);
      } break;
      case BCMP_PROTOCOL_CAPS_REPLY: {
        BCMPProtocolCapsReply *reply = (BCMPProtocolCapsReply *)buf;
        swap_64bit(&reply->node_id);
        swap_16bit(&reply->bcmp_rev);
        swap_16bit(&reply->caps_count);
      } break;
      case BCMP_NEIGHBOR_TABLE_REQUEST: {
        BCMPNeighborTableRequest *request = (BCMPNeighborTableRequest *)buf;
        swap_64bit(&request->target_node_id);
      } break;
      case BCMP_NEIGHBOR_TABLE_REPLY: {
        BCMPNeighborTableReply *reply = (BCMPNeighborTableReply *)reply;
        swap_64bit(&reply->node_id);
        swap_16bit(&reply->neighbor_len);
      } break;
      case BCMP_RESOURCE_TABLE_REQUEST: {
        BCMPResourceTableRequest *request = (BCMPResourceTableRequest *)buf;
        swap_64bit(&request->target_node_id);
      } break;
      case BCMP_RESOURCE_TABLE_REPLY: {
        BCMPResourceTableReply *reply = (BCMPResourceTableReply *)buf;
        swap_64bit(&reply->node_id);
        swap_16bit(&reply->num_pubs);
        swap_16bit(&reply->num_subs);
      } break;
      case BCMP_NEIGHBOR_PROTO_REQUEST: {
        BCMPNeighborProtoRequest *request = (BCMPNeighborProtoRequest *)buf;
        swap_64bit(&request->target_node_id);
        swap_16bit(&request->option_count);
      } break;
      case BCMP_NEIGHBOR_PROTO_REPLY: {
        BCMPNeighborProtoReply *request = (BCMPNeighborProtoReply *)buf;
        swap_64bit(&request->node_id);
        swap_16bit(&request->option_count);
      } break;
      case BCMP_SYSTEM_TIME_REQUEST: {
        BCMPSystemTimeRequest *request = (BCMPSystemTimeRequest *)buf;
        swap_64bit(&request->header.target_node_id);
        swap_64bit(&request->header.source_node_id);
      } break;
      case BCMP_SYSTEM_TIME_RESPONSE: {
        BCMPSystemTimeResponse *response = (BCMPSystemTimeResponse *)buf;
        swap_64bit(&response->header.target_node_id);
        swap_64bit(&response->header.source_node_id);
        swap_64bit(&response->utc_time_us);
      } break;
      case BCMP_SYSTEM_TIME_SET: {
        BCMPSystemTimeSet *set = (BCMPSystemTimeSet *)buf;
        swap_64bit(&set->header.target_node_id);
        swap_64bit(&set->header.source_node_id);
        swap_64bit(&set->utc_time_us);
      } break;
      case BCMP_NET_STAT_REQUEST: {
        BCMPNetstatRequest *request = (BCMPNetstatRequest *)buf;
        swap_64bit(&request->target_node_id);
      } break;
      case BCMP_NET_STAT_REPLY: {
        BCMPNetstatReply *reply = (BCMPNetstatReply *)buf;
        swap_64bit(&reply->node_id);
      } break;
      case BCMP_POWER_STAT_REQUEST: {
        BCMPPowerStateRequest *request = (BCMPPowerStateRequest *)buf;
        swap_64bit(&request->target_node_id);
      } break;
      case BCMP_POWER_STAT_REPLY: {
        BCMPPowerStateReply *reply = (BCMPPowerStateReply *)buf;
        swap_64bit(&reply->node_id);
      } break;
      case BCMP_REBOOT_REQUEST: {
        BCMPRebootRequest *request = (BCMPRebootRequest *)buf;
        swap_64bit(&request->target_node_id);
      } break;
      case BCMP_REBOOT_REPLY: {
        BCMPRebootReply *reply = (BCMPRebootReply *)buf;
        swap_64bit(&reply->node_id);
      } break;
      case BCMP_NET_ASSERT_QUIET: {
        BCMPNetAssertQuiet *request = (BCMPNetAssertQuiet *)buf;
      } break;
      case BCMP_CONFIG_GET:
      case BCMP_CONFIG_VALUE:
      case BCMP_CONFIG_SET:
      case BCMP_CONFIG_COMMIT:
      case BCMP_CONFIG_STATUS_REQUEST:
      case BCMP_CONFIG_STATUS_RESPONSE:
      case BCMP_CONFIG_DELETE_REQUEST:
      case BCMP_CONFIG_DELETE_RESPONSE:
      case BCMP_DFU_START:
      case BCMP_DFU_PAYLOAD_REQ:
      case BCMP_DFU_PAYLOAD:
      case BCMP_DFU_END:
      case BCMP_DFU_ACK:
      case BCMP_DFU_ABORT:
      case BCMP_DFU_HEARTBEAT:
      case BCMP_DFU_REBOOT_REQ:
      case BCMP_DFU_REBOOT:
      case BCMP_DFU_BOOT_COMPLETE:
      case BCMP_HEADER:
        break;
      }
    }
  }

  return err;
}
