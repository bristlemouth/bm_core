#include "packet.h"
#include "bm_os.h"
#include "ll.h"
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>

#define default_message_timeout_ms 24
#define message_timer_expiry_period_ms 12

typedef struct BcmpRequestElement {
  uint16_t type;
  uint32_t timestamp_ms;
  uint32_t timeout_ms;
  uint32_t seq_num;
  BcmpSequencedRequestCb cb;
} BcmpRequestElement;

struct PacketInfo {
  struct {
    BcmpGetIPAddr src_ip;
    BcmpGetIPAddr dst_ip;
    BcmpGetData data;
    BcmpGetChecksum checksum;
  } cb;
  bool initialized;
  BmSemaphore sequence_list_semaphore;
  BmTimer timer;
  LL sequence_list;
  LL packet_list;
};

static struct PacketInfo PACKET;

/*!
 @brief Check Endianness Of Message Type And Format Little Endian

 @details This must get updated whenever there is a new message added to the spec

 @param buf buffer to format
 @param type message type to format
 */
static void check_endianness(void *buf, BcmpMessageType type) {
  if (!is_little_endian()) {
    switch (type) {
    case BcmpAckMessage:
      break;
    case BcmpHeartbeatMessage: {
      BcmpHeartbeat *hb = (BcmpHeartbeat *)buf;
      swap_64bit(&hb->time_since_boot_us);
      swap_32bit(&hb->liveliness_lease_dur_s);
    } break;
    case BcmpEchoRequestMessage: {
      BcmpEchoRequest *request = (BcmpEchoRequest *)buf;
      swap_64bit(&request->target_node_id);
      swap_16bit(&request->id);
      swap_32bit(&request->seq_num);
      swap_16bit(&request->payload_len);
    } break;
    case BcmpEchoReplyMessage: {
      BcmpEchoReply *reply = (BcmpEchoReply *)buf;
      swap_64bit(&reply->node_id);
      swap_16bit(&reply->id);
      swap_16bit(&reply->seq_num);
      swap_16bit(&reply->payload_len);
    } break;
    case BcmpDeviceInfoRequestMessage: {
      BcmpDeviceInfoRequest *request = (BcmpDeviceInfoRequest *)buf;
      swap_64bit(&request->target_node_id);
    } break;
    case BcmpDeviceInfoReplyMessage: {
      BcmpDeviceInfoReply *reply = (BcmpDeviceInfoReply *)buf;
      swap_64bit(&reply->info.node_id);
      swap_16bit(&reply->info.vendor_id);
      swap_16bit(&reply->info.product_id);
      swap_32bit(&reply->info.git_sha);
    } break;
    case BcmpProtocolCapsRequestMessage: {
      BcmpProtocolCapsRequest *request = (BcmpProtocolCapsRequest *)buf;
      swap_64bit(&request->target_node_id);
      swap_16bit(&request->caps_list_len);
    } break;
    case BcmpProtocolCapsReplyMessage: {
      BcmpProtocolCapsReply *reply = (BcmpProtocolCapsReply *)buf;
      swap_64bit(&reply->node_id);
      swap_16bit(&reply->bcmp_rev);
      swap_16bit(&reply->caps_count);
    } break;
    case BcmpNeighborTableRequestMessage: {
      BcmpNeighborTableRequest *request = (BcmpNeighborTableRequest *)buf;
      swap_64bit(&request->target_node_id);
    } break;
    case BcmpNeighborTableReplyMessage: {
      BcmpNeighborTableReply *reply = (BcmpNeighborTableReply *)buf;
      swap_64bit(&reply->node_id);
      swap_16bit(&reply->neighbor_len);
    } break;
    case BcmpResourceTableRequestMessage: {
      BcmpResourceTableRequest *request = (BcmpResourceTableRequest *)buf;
      swap_64bit(&request->target_node_id);
    } break;
    case BcmpResourceTableReplyMessage: {
      BcmpResourceTableReply *reply = (BcmpResourceTableReply *)buf;
      swap_64bit(&reply->node_id);
      swap_16bit(&reply->num_pubs);
      swap_16bit(&reply->num_subs);
    } break;
    case BcmpNeighborProtoRequestMessage: {
      BcmpNeighborProtoRequest *request = (BcmpNeighborProtoRequest *)buf;
      swap_64bit(&request->target_node_id);
      swap_16bit(&request->option_count);
    } break;
    case BcmpNeighborProtoReplyMessage: {
      BcmpNeighborProtoReply *request = (BcmpNeighborProtoReply *)buf;
      swap_64bit(&request->node_id);
      swap_16bit(&request->option_count);
    } break;
    case BcmpSystemTimeRequestMessage: {
      BcmpSystemTimeRequest *request = (BcmpSystemTimeRequest *)buf;
      swap_64bit(&request->header.target_node_id);
      swap_64bit(&request->header.source_node_id);
    } break;
    case BcmpSystemTimeResponseMessage: {
      BcmpSystemTimeResponse *response = (BcmpSystemTimeResponse *)buf;
      swap_64bit(&response->header.target_node_id);
      swap_64bit(&response->header.source_node_id);
      swap_64bit(&response->utc_time_us);
    } break;
    case BcmpSystemTimeSetMessage: {
      BcmpSystemTimeSet *set = (BcmpSystemTimeSet *)buf;
      swap_64bit(&set->header.target_node_id);
      swap_64bit(&set->header.source_node_id);
      swap_64bit(&set->utc_time_us);
    } break;
    case BcmpNetStateRequestMessage: {
      BcmpNetStateRequest *request = (BcmpNetStateRequest *)buf;
      swap_64bit(&request->target_node_id);
    } break;
    case BcmpNetStateReplyMessage: {
      BcmpNetStateReply *reply = (BcmpNetStateReply *)buf;
      swap_64bit(&reply->node_id);
    } break;
    case BcmpPowerStateRequestMessage: {
      BcmpPowerStateRequest *request = (BcmpPowerStateRequest *)buf;
      swap_64bit(&request->target_node_id);
    } break;
    case BcmpPowerStateReplyMessage: {
      BcmpPowerStateReply *reply = (BcmpPowerStateReply *)buf;
      swap_64bit(&reply->node_id);
    } break;
    case BcmpRebootRequestMessage: {
      BcmpRebootRequest *request = (BcmpRebootRequest *)buf;
      swap_64bit(&request->target_node_id);
    } break;
    case BcmpRebootReplyMessage: {
      BcmpRebootReply *reply = (BcmpRebootReply *)buf;
      swap_64bit(&reply->node_id);
    } break;
    case BcmpNetAssertQuietMessage: {
      BcmpNetAssertQuiet *request = (BcmpNetAssertQuiet *)buf;
      swap_64bit(&request->target_node_id);
    } break;
    case BcmpConfigGetMessage:
    case BcmpConfigValueMessage:
    case BcmpConfigSetMessage:
    case BcmpConfigCommitMessage:
    case BcmpConfigStatusRequestMessage:
    case BcmpConfigStatusResponseMessage:
    case BcmpConfigDeleteRequestMessage:
    case BcmpConfigDeleteResponseMessage:
    case BcmpDFUStartMessage:
    case BcmpDFUPayloadReqMessage:
    case BcmpDFUPayloadMessage:
    case BcmpDFUEndMessage:
    case BcmpDFUAckMessage:
    case BcmpDFUAbortMessage:
    case BcmpDFUHeartbeatMessage:
    case BcmpDFURebootReqMessage:
    case BcmpDFURebootMessage:
    case BcmpDFUBootCompleteMessage:
      break;
    case BcmpHeaderMessage: {
      BcmpHeader *header = (BcmpHeader *)buf;
      swap_16bit(&header->type);
      swap_16bit(&header->checksum);
      swap_32bit(&header->seq_num);
    } break;
    }
  }
}

/*!
 @brief Add Message To Sequenced Linked List

 @param message message to add

 @return true if message is added successfully
 @return false if message is not added successfully
 */
static bool sequence_list_add_message(BcmpRequestElement message,
                                      uint32_t seq_num) {
  bool ret = false;
  LLItem *item = NULL;
  if (bm_semaphore_take(PACKET.sequence_list_semaphore,
                        default_message_timeout_ms) == BmOK) {
    item = ll_create_item(item, &message, sizeof(BcmpRequestElement), seq_num);
    if (item) {
      ll_item_add(&PACKET.sequence_list, item);
      ret = true;
    }
    bm_semaphore_give(PACKET.sequence_list_semaphore);
  }
  return ret;
}

/*!
 @brief Remove Message From Sequenced Linked List

 @param message message to be removed

 @return true if message is removed successfully
 @return false if message is not removed successfully
 */
static bool sequence_list_remove_message(uint32_t seq_num) {
  bool ret = false;
  if (ll_remove(&PACKET.sequence_list, seq_num) == BmOK) {
    ret = true;
  }
  return ret;
}

/*!
 @brief Create An Item To Be Added To Sequenced Linked List

 @param seq_num sequence number of item to be added
 @param type type of message the sequence item is
 @param timeout_ms the amount of time the item can exist in the list
 @param cb callback to be used when a reply is handled

 @return new item that has been added
 @return NULL if item is not able to be created
 */
static BcmpRequestElement new_sequence_list_item(BcmpMessageType type,
                                                 uint32_t timeout_ms,
                                                 uint32_t seq_num,
                                                 BcmpSequencedRequestCb cb) {
  BcmpRequestElement element;
  element.type = type;
  element.timestamp_ms = bm_ticks_to_ms(bm_get_tick_count());
  element.timeout_ms = timeout_ms;
  element.cb = cb;
  element.seq_num = seq_num;
  return element;
}

static BmErr timer_traverse_cb(void *data, void *arg) {
  BmErr err = BmEINVAL;
  BcmpRequestElement *element = NULL;
  (void)arg;

  if (data) {
    err = BmOK;
    element = (BcmpRequestElement *)data;
    if (bm_ticks_to_ms(bm_get_tick_count()) - element->timestamp_ms >
        element->timeout_ms) {
      if (element->cb) {
        element->cb(NULL);
      }
      err = ll_remove(&PACKET.sequence_list, element->seq_num);
    }
  }

  return err;
}

/*!
 @brief BM Timer Callback For Linked List Item

#details Iterates through linked list and determines if the sequence item has
         expired and removes it from linked list

 @param tmr timer instance (unused)
 */
static void sequence_list_timer_callback(BmTimer tmr) {
  (void)tmr;
  if (bm_semaphore_take(PACKET.sequence_list_semaphore,
                        default_message_timeout_ms) == BmOK) {
    ll_traverse(&PACKET.sequence_list, timer_traverse_cb, NULL);
    bm_semaphore_give(PACKET.sequence_list_semaphore);
  }
}

/*!
 @brief Find Sequenced Item In Linked List

 @param seq_num number if item to find in the linked list

 @return item if it is found in linked list
 @return NULL if item is not able to found
 */
static BcmpRequestElement *sequence_list_find_message(uint32_t seq_num) {
  BmErr err = BmEINPROGRESS;
  BcmpRequestElement *element = NULL;
  if (bm_semaphore_take(PACKET.sequence_list_semaphore,
                        default_message_timeout_ms) == BmOK) {
    err = ll_get_item(&PACKET.sequence_list, seq_num, (void *)&element);
    if (err == BmOK && element) {
      printf("Bcmp message with seq_num %d\n", seq_num);
    }
    bm_semaphore_give(PACKET.sequence_list_semaphore);
  }
  return element;
}

/*!
 @brief Initialize Packet Module

 @param src_ip callback to obtain source IP address from raw payload
 @param dst_ip callback to obtain destination IP address from raw payload
 @param data callback to obtain data from raw payload
 @param checksum callback to calculate checksum from raw payload

 @return BmOK on success
 @return BmError on failure
 */
BmErr packet_init(BcmpGetIPAddr src_ip, BcmpGetIPAddr dst_ip, BcmpGetData data,
                  BcmpGetChecksum checksum) {
  BmErr err = BmEINVAL;
  if (src_ip && dst_ip && data && checksum) {
    PACKET.cb.src_ip = src_ip;
    PACKET.cb.dst_ip = dst_ip;
    PACKET.cb.data = data;
    PACKET.cb.checksum = checksum;
    PACKET.initialized = true;

    // Create timer and semaphore for sequenced packet handling
    err = BmENOMEM;
    PACKET.sequence_list_semaphore = bm_semaphore_create();
    if (PACKET.sequence_list_semaphore) {
      PACKET.timer = bm_timer_create(sequence_list_timer_callback,
                                     "bcmp_message_expiration",
                                     message_timer_expiry_period_ms, NULL);
      err = PACKET.timer != NULL ? bm_timer_start(PACKET.timer, 10) : err;
    }
  }
  return err;
}

/*!
 @brief Add Packet Item To Packet Processor/Serializer

 @param cfg statically allocated configuration of packet
 @param type type of packet to add, this will be ll item ID

 @return BmOK on success
 @return BmError on failure
 */
BmErr packet_add(BcmpPacketCfg *cfg, BcmpMessageType type) {
  BmErr err = BmEINVAL;
  LLItem *item = NULL;

  if (cfg) {
    item = ll_create_item(item, cfg, sizeof(BcmpPacketCfg), type);
    err = item ? ll_item_add(&PACKET.packet_list, item) : err;
  }

  return err;
}

/*!
 @brief Remove Packet Item From Packet Processor/Serializer

 @param type type of packet to remove, this will be ll item ID

 @return BmOK on success
 @return BmError on failure
 */
BmErr packet_remove(BcmpMessageType type) {
  return ll_remove(&PACKET.packet_list, type);
}

/*!
 @brief Update Function To Parse And Process And Handle Incoming Message

 @details This is to be ran on incoming messages, the function
          serializes the header from the received message and then
          reports the payload to the associated packet processor item.
          The packet processor is then responsible for serializing the payload.

 @param payload incoming data to be parsed and processed

 @return BmOK on success
 @return BmError on failure
 */
BmErr process_received_message(void *payload) {
  BmErr err = BmEINVAL;
  BcmpProcessData data;
  BcmpSequencedRequestCb cb = NULL;
  BcmpRequestElement *request_message = NULL;
  BcmpPacketCfg *cfg = NULL;
  void *buf = NULL;

  if (payload && PACKET.initialized) {
    buf = PACKET.cb.data(payload);
    data.header = (BcmpHeader *)buf;
    data.payload = (uint8_t *)(buf + sizeof(BcmpHeader));
    data.src = PACKET.cb.src_ip(payload);
    data.dst = PACKET.cb.dst_ip(payload);
    data.ingress_port = (((uint32_t *)(data.src))[1] >> 8) & 0xFF;
    check_endianness(data.header, BcmpHeaderMessage);

    // Process parsed message type
    if ((err = ll_get_item(&PACKET.packet_list, data.header->type,
                           (void *)&cfg)) == BmOK &&
        cfg) {
      check_endianness(data.payload, data.header->type);

      // Check if this message is a reply to a message we sent
      if (cfg->sequenced_reply && !cfg->sequenced_request) {
        request_message = sequence_list_find_message(data.header->seq_num);
        if (request_message) {
          printf(
              "BCMP - Received reply to our request message with seq_num %d\n",
              data.header->seq_num);
          if (bm_semaphore_take(PACKET.sequence_list_semaphore,
                                default_message_timeout_ms) == BmOK) {
            cb = request_message->cb;
            sequence_list_remove_message(data.header->seq_num);
            bm_semaphore_give(PACKET.sequence_list_semaphore);
          }
        }
      }

      if (cb) {
        // If message is a reply, utilize associated cb
        err = cb(data.payload);
      } else {
        // Utilize parsing callback
        if (cfg->process && (err = cfg->process(data)) != BmOK) {
          printf("Error processing parsed cb: 0x%X\n", err);
        }
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

 @param payload buffer to be formatted and serialized
 @param data passed in formatted data structure cast to a void pointer
 @param type type of message to serialize
 @param seq_num number of sequence reply
 @param cb callback of message that is to be sequenced upon a sequenced reply

 @return BmOK on success
 @return BmError on failure
 */
BmErr serialize(void *payload, void *data, BcmpMessageType type,
                uint32_t seq_num, BcmpSequencedRequestCb cb) {
  static uint32_t message_count = 0;
  BmErr err = BmEINVAL;
  BcmpHeader *header = NULL;
  BcmpPacketCfg *cfg = NULL;
  BcmpRequestElement request_message;

  if (payload && data && PACKET.initialized) {
    // Check endianness of type and place into little endian form
    check_endianness(data, type);

    // Determine size of message type and if there is a sequenced reply/request
    if ((err = ll_get_item(&PACKET.packet_list, type, (void *)&cfg)) == BmOK &&
        cfg) {

      header = (BcmpHeader *)PACKET.cb.data(payload);
      header->flags = 0;       // Unused
      header->reserved = 0;    // Unused
      header->frag_total = 0;  // Unused
      header->frag_id = 0;     // Unused
      header->next_header = 0; // Unused
      header->type = type;
      if (cfg->sequenced_reply) {
        // if we are replying to a message, use the sequence number from the received message
        header->seq_num = seq_num;
      } else if (cfg->sequenced_request) {
        // If we are sending a new request, use our own sequence number
        header->seq_num = message_count++;
        request_message = new_sequence_list_item(
            header->type, default_message_timeout_ms, header->seq_num, cb);
        sequence_list_add_message(request_message, header->seq_num);
        printf("BCMP - Serializing message with seq_num %d\n", header->seq_num);
      } else {
        // If the message doesn't use sequence numbers, set it to 0
        header->seq_num = 0;
      }

      // Format header in little endian format and append data onto payload
      check_endianness(header, BcmpHeaderMessage);
      memcpy(((uint8_t *)header) + sizeof(BcmpHeader), data, cfg->size);

      header->checksum =
          PACKET.cb.checksum(payload, cfg->size + sizeof(BcmpHeader));
    }
  }

  return err;
}
