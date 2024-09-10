#include "packet.h"
#include "bm_rtos.h"
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>

#define default_message_timeout_ms 24
#define message_timer_expiry_period_ms 12

typedef BmErr (*BCMPParseCb)(BCMPParserData data);
typedef BmErr (*BCMPSequencedRequestCb)(uint8_t *payload);

typedef struct BCMPRequestElement {
  uint32_t seq_num;
  uint16_t type;
  uint32_t timestamp_ms;
  uint32_t timeout_ms;
  BCMPSequencedRequestCb cb;
  struct BCMPRequestElement *next;
} BCMPRequestElement;

struct SerialLUT {
  BCMPMessageType type;
  uint32_t size;
  bool sequenced_reply;
  BCMPSequencedRequestCb sequenced_request;
  BCMPParseCb parse;
};

struct PacketInfo {
  struct {
    BCMPGetIPAddr src_ip;
    BCMPGetIPAddr dst_ip;
    BCMPGetData data;
    BCMPGetChecksum checksum;
  } cb;
  bool initialized;
  BmSemaphore sequence_list_semaphore;
  BmTimer timer;
  BCMPRequestElement *sequence_list;
};

static struct PacketInfo PACKET;

//TODO: place in parser cbs and sequence cbs here
static struct SerialLUT PACKET_LUT[] = {
    {
        BCMPHeartbeatMessage,
        sizeof(BCMPHeartbeat),
        false,
        NULL,
        NULL,
    },
    {
        BCMPEchoRequestMessage,
        sizeof(BCMPEchoRequest),
        false,
        NULL,
        NULL,
    },
    {
        BCMPEchoReplyMessage,
        sizeof(BCMPEchoReply),
        false,
        NULL,
        NULL,
    },
    {
        BCMPDeviceInfoRequestMessage,
        sizeof(BCMPDeviceInfoRequest),
        false,
        NULL,
        NULL,
    },
    {
        BCMPDeviceInfoReplyMessage,
        sizeof(BCMPDeviceInfoReply),
        false,
        NULL,
        NULL,
    },
    {
        BCMPProtocolCapsRequestMessage,
        sizeof(BCMPProtocolCapsRequest),
        false,
        NULL,
        NULL,
    },
    {
        BCMPProtocolCapsReplyMessage,
        sizeof(BCMPProtocolCapsReply),
        false,
        NULL,
        NULL,
    },
    {
        BCMPNeighborTableRequestMessage,
        sizeof(BCMPNeighborTableRequest),
        false,
        NULL,
        NULL,
    },
    {
        BCMPNeighborTableReplyMessage,
        sizeof(BCMPNeighborTableReply),
        false,
        NULL,
        NULL,
    },
    {
        BCMPResourceTableRequestMessage,
        sizeof(BCMPResourceTableRequest),
        false,
        NULL,
        NULL,
    },
    {
        BCMPResourceTableReplyMessage,
        sizeof(BCMPResourceTableReply),
        false,
        NULL,
        NULL,
    },
    {
        BCMPNeighborProtoRequestMessage,
        sizeof(BCMPNeighborProtoRequest),
        false,
        NULL,
        NULL,
    },
    {
        BCMPNeighborTableReplyMessage,
        sizeof(BCMPNeighborTableReply),
        false,
        NULL,
        NULL,
    },
    {
        BCMPSystemTimeRequestMessage,
        sizeof(BCMPSystemTimeRequest),
        false,
        NULL,
        NULL,
    },
    {
        BCMPSystemTimeResponseMessage,
        sizeof(BCMPSystemTimeResponse),
        false,
        NULL,
        NULL,
    },
    {
        BCMPSystemTimeSetMessage,
        sizeof(BCMPSystemTimeSet),
        false,
        NULL,
        NULL,
    },
    {
        BCMPNetStateRequestMessage,
        sizeof(BCMPNetStateRequest),
        false,
        NULL,
        NULL,
    },
    {
        BCMPNetStateReplyMessage,
        sizeof(BCMPNetStateReply),
        false,
        NULL,
        NULL,
    },
    {
        BCMPPowerStateRequestMessage,
        sizeof(BCMPPowerStateRequest),
        false,
        NULL,
        NULL,
    },
    {
        BCMPPowerStateReplyMessage,
        sizeof(BCMPPowerStateReply),
        false,
        NULL,
        NULL,
    },
    {
        BCMPRebootRequestMessage,
        sizeof(BCMPRebootRequest),
        false,
        NULL,
        NULL,
    },
    {
        BCMPRebootReplyMessage,
        sizeof(BCMPRebootReply),
        false,
        NULL,
        NULL,
    },
    {
        BCMPNetAssertQuietMessage,
        sizeof(BCMPNetAssertQuiet),
        false,
        NULL,
        NULL,
    },
    // TODO: Set the size and callbacks of down belo
    {
        BCMPConfigGetMessage,
        1,
        false,
        NULL,
        NULL,
    },
    {
        BCMPConfigValueMessage,
        1,
        true,
        NULL,
        NULL,
    },
    {
        BCMPConfigSetMessage,
        1,
        false,
        NULL,
        NULL,
    },
    {
        BCMPConfigCommitMessage,
        1,
        false,
        NULL,
        NULL,
    },
    {
        BCMPConfigStatusRequestMessage,
        1,
        false,
        NULL,
        NULL,
    },
    {
        BCMPConfigStatusResponseMessage,
        1,
        true,
        NULL,
        NULL,
    },
    {
        BCMPConfigDeleteRequestMessage,
        1,
        false,
        NULL,
        NULL,
    },
    {
        BCMPConfigDeleteResponseMessage,
        1,
        true,
        NULL,
        NULL,
    },
    {
        BCMPDFUStartMessage,
        1,
        false,
        NULL,
        NULL,
    },
    {
        BCMPDFUPayloadReqMessage,
        1,
        false,
        NULL,
        NULL,
    },
    {
        BCMPDFUPayloadMessage,
        1,
        false,
        NULL,
        NULL,
    },
    {
        BCMPDFUEndMessage,
        1,
        false,
        NULL,
        NULL,
    },
    {
        BCMPDFUAckMessage,
        1,
        false,
        NULL,
        NULL,
    },
    {
        BCMPDFUAbortMessage,
        1,
        false,
        NULL,
        NULL,
    },
    {
        BCMPDFUHeartbeatMessage,
        1,
        false,
        NULL,
        NULL,
    },
    {
        BCMPDFURebootReqMessage,
        1,
        false,
        NULL,
        NULL,
    },
    {
        BCMPDFURebootMessage,
        1,
        false,
        NULL,
        NULL,
    },
    {
        BCMPDFUBootCompleteMessage,
        1,
        false,
        NULL,
        NULL,
    },
};

/*!
 @brief Check Endianness Of Message Type And Format Little Endian

 @details This must get updated whenever there is a new message added to the spec

 @param buf buffer to format
 @param type message type to format
 */
static void check_endianness(void *buf, BCMPMessageType type) {
  if (!is_little_endian()) {
    switch (type) {
    case BCMPAckMessage:
      break;
    case BCMPHeartbeatMessage: {
      BCMPHeartbeat *hb = (BCMPHeartbeat *)buf;
      swap_64bit(&hb->time_since_boot_us);
      swap_32bit(&hb->liveliness_lease_dur_s);
    } break;
    case BCMPEchoRequestMessage: {
      BCMPEchoRequest *request = (BCMPEchoRequest *)buf;
      swap_64bit(&request->target_node_id);
      swap_16bit(&request->id);
      swap_32bit(&request->seq_num);
      swap_16bit(&request->payload_len);
    } break;
    case BCMPEchoReplyMessage: {
      BCMPEchoReply *reply = (BCMPEchoReply *)buf;
      swap_64bit(&reply->node_id);
      swap_16bit(&reply->id);
      swap_16bit(&reply->seq_num);
      swap_16bit(&reply->payload_len);
    } break;
    case BCMPDeviceInfoRequestMessage: {
      BCMPDeviceInfoRequest *request = (BCMPDeviceInfoRequest *)buf;
      swap_64bit(&request->target_node_id);
    } break;
    case BCMPDeviceInfoReplyMessage: {
      BCMPDeviceInfoReply *reply = (BCMPDeviceInfoReply *)buf;
      swap_64bit(&reply->info.node_id);
      swap_16bit(&reply->info.vendor_id);
      swap_16bit(&reply->info.product_id);
      swap_32bit(&reply->info.git_sha);
    } break;
    case BCMPProtocolCapsRequestMessage: {
      BCMPProtocolCapsRequest *request = (BCMPProtocolCapsRequest *)buf;
      swap_64bit(&request->target_node_id);
      swap_16bit(&request->caps_list_len);
    } break;
    case BCMPProtocolCapsReplyMessage: {
      BCMPProtocolCapsReply *reply = (BCMPProtocolCapsReply *)buf;
      swap_64bit(&reply->node_id);
      swap_16bit(&reply->bcmp_rev);
      swap_16bit(&reply->caps_count);
    } break;
    case BCMPNeighborTableRequestMessage: {
      BCMPNeighborTableRequest *request = (BCMPNeighborTableRequest *)buf;
      swap_64bit(&request->target_node_id);
    } break;
    case BCMPNeighborTableReplyMessage: {
      BCMPNeighborTableReply *reply = (BCMPNeighborTableReply *)reply;
      swap_64bit(&reply->node_id);
      swap_16bit(&reply->neighbor_len);
    } break;
    case BCMPResourceTableRequestMessage: {
      BCMPResourceTableRequest *request = (BCMPResourceTableRequest *)buf;
      swap_64bit(&request->target_node_id);
    } break;
    case BCMPResourceTableReplyMessage: {
      BCMPResourceTableReply *reply = (BCMPResourceTableReply *)buf;
      swap_64bit(&reply->node_id);
      swap_16bit(&reply->num_pubs);
      swap_16bit(&reply->num_subs);
    } break;
    case BCMPNeighborProtoRequestMessage: {
      BCMPNeighborProtoRequest *request = (BCMPNeighborProtoRequest *)buf;
      swap_64bit(&request->target_node_id);
      swap_16bit(&request->option_count);
    } break;
    case BCMPNeighborProtoReplyMessage: {
      BCMPNeighborProtoReply *request = (BCMPNeighborProtoReply *)buf;
      swap_64bit(&request->node_id);
      swap_16bit(&request->option_count);
    } break;
    case BCMPSystemTimeRequestMessage: {
      BCMPSystemTimeRequest *request = (BCMPSystemTimeRequest *)buf;
      swap_64bit(&request->header.target_node_id);
      swap_64bit(&request->header.source_node_id);
    } break;
    case BCMPSystemTimeResponseMessage: {
      BCMPSystemTimeResponse *response = (BCMPSystemTimeResponse *)buf;
      swap_64bit(&response->header.target_node_id);
      swap_64bit(&response->header.source_node_id);
      swap_64bit(&response->utc_time_us);
    } break;
    case BCMPSystemTimeSetMessage: {
      BCMPSystemTimeSet *set = (BCMPSystemTimeSet *)buf;
      swap_64bit(&set->header.target_node_id);
      swap_64bit(&set->header.source_node_id);
      swap_64bit(&set->utc_time_us);
    } break;
    case BCMPNetStateRequestMessage: {
      BCMPNetStateRequest *request = (BCMPNetStateRequest *)buf;
      swap_64bit(&request->target_node_id);
    } break;
    case BCMPNetStateReplyMessage: {
      BCMPNetStateReply *reply = (BCMPNetStateReply *)buf;
      swap_64bit(&reply->node_id);
    } break;
    case BCMPPowerStateRequestMessage: {
      BCMPPowerStateRequest *request = (BCMPPowerStateRequest *)buf;
      swap_64bit(&request->target_node_id);
    } break;
    case BCMPPowerStateReplyMessage: {
      BCMPPowerStateReply *reply = (BCMPPowerStateReply *)buf;
      swap_64bit(&reply->node_id);
    } break;
    case BCMPRebootRequestMessage: {
      BCMPRebootRequest *request = (BCMPRebootRequest *)buf;
      swap_64bit(&request->target_node_id);
    } break;
    case BCMPRebootReplyMessage: {
      BCMPRebootReply *reply = (BCMPRebootReply *)buf;
      swap_64bit(&reply->node_id);
    } break;
    case BCMPNetAssertQuietMessage: {
      BCMPNetAssertQuiet *request = (BCMPNetAssertQuiet *)buf;
    } break;
    case BCMPConfigGetMessage:
    case BCMPConfigValueMessage:
    case BCMPConfigSetMessage:
    case BCMPConfigCommitMessage:
    case BCMPConfigStatusRequestMessage:
    case BCMPConfigStatusResponseMessage:
    case BCMPConfigDeleteRequestMessage:
    case BCMPConfigDeleteResponseMessage:
    case BCMPDFUStartMessage:
    case BCMPDFUPayloadReqMessage:
    case BCMPDFUPayloadMessage:
    case BCMPDFUEndMessage:
    case BCMPDFUAckMessage:
    case BCMPDFUAbortMessage:
    case BCMPDFUHeartbeatMessage:
    case BCMPDFURebootReqMessage:
    case BCMPDFURebootMessage:
    case BCMPDFUBootCompleteMessage:
      break;
    case BCMPHeaderMessage: {
      BCMPHeader *header = (BCMPHeader *)buf;
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
static bool sequence_list_add_message(BCMPRequestElement *message) {
  bool rval = false;
  if (message &&
      bm_semaphore_take(PACKET.sequence_list_semaphore, default_message_timeout_ms) == BmOK) {
    if (PACKET.sequence_list == NULL) {
      PACKET.sequence_list = message;
    } else {
      BCMPRequestElement *current = PACKET.sequence_list;
      while (current->next != NULL) {
        current = current->next;
      }
      current->next = message;
    }
    rval = true;
    bm_semaphore_give(PACKET.sequence_list_semaphore);
  }
  return rval;
}

/*!
 @brief Remove Message From Sequenced Linked List

 @param message message to be removed

 @return true if message is removed successfully
 @return false if message is not removed successfully
 */
static bool sequence_list_remove_message(BCMPRequestElement *message) {
  bool rval = false;
  BCMPRequestElement *element_to_delete = NULL;
  if (message) {
    if (PACKET.sequence_list == message) {
      element_to_delete = PACKET.sequence_list;
      PACKET.sequence_list = PACKET.sequence_list->next;
    } else {
      BCMPRequestElement *current = PACKET.sequence_list;
      while (current && current->next != message) {
        current = current->next;
      }
      if (current) {
        element_to_delete = current->next;
        current->next = current->next->next;
      }
    }
    if (element_to_delete) {
      bm_free(element_to_delete);
      rval = true;
    }
  }
  return rval;
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
static BCMPRequestElement *new_sequence_list_item(uint16_t seq_num, BCMPMessageType type,
                                                  uint32_t timeout_ms,
                                                  BCMPSequencedRequestCb cb) {
  BCMPRequestElement *element = (BCMPRequestElement *)bm_malloc(sizeof(BCMPRequestElement));
  if (element != NULL) {
    element->seq_num = seq_num;
    element->type = type;
    element->timestamp_ms = bm_ticks_to_ms(bm_get_tick_count());
    element->timeout_ms = timeout_ms;
    element->cb = cb;
    element->next = NULL;
  }
  return element;
}

/*!
 @brief BM Timer Callback For Linked List Item

#details Iterates through linked list and determines if the sequence item has
         expired and removes it from linked list

 @param tmr timer instance (unused)
 */
static void sequence_list_timer_callback(BmTimer tmr) {
  (void)tmr;
  if (bm_semaphore_take(PACKET.sequence_list_semaphore, default_message_timeout_ms) == BmOK) {
    BCMPRequestElement *current = PACKET.sequence_list;
    while (current) {
      if (bm_ticks_to_ms(bm_get_tick_count()) - current->timestamp_ms > current->timeout_ms) {
        printf("BCMP message with seq_num %d timed out\n", current->seq_num);
        if (current->cb) {
          current->cb(NULL);
        }
        sequence_list_remove_message(current);
        current = PACKET.sequence_list;
        continue;
      }
      current = current->next;
    }
    bm_semaphore_give(PACKET.sequence_list_semaphore);
  }
}

/*!
 @brief Find Sequenced Item In Linked List

 @param seq_num number if item to find in the linked list

 @return item if it is found in linked list
 @return NULL if item is not able to found
 */
static BCMPRequestElement *sequence_list_find_message(uint16_t seq_num) {
  BCMPRequestElement *rval = NULL;
  if (bm_semaphore_take(PACKET.sequence_list_semaphore, default_message_timeout_ms) == BmOK) {
    BCMPRequestElement *current = PACKET.sequence_list;
    while (current) {
      if (current->seq_num == seq_num) {
        printf("BCMP message with seq_num %d found\n", current->seq_num);
        rval = current;
        break;
      }
      current = current->next;
    }
    bm_semaphore_give(PACKET.sequence_list_semaphore);
  }
  return rval;
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
BmErr packet_init(BCMPGetIPAddr src_ip, BCMPGetIPAddr dst_ip, BCMPGetData data,
                  BCMPGetChecksum checksum) {
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
      PACKET.timer = bm_timer_create(sequence_list_timer_callback, "bcmp_message_expiration",
                                     message_timer_expiry_period_ms, NULL);
      err = PACKET.timer != NULL ? bm_timer_start(PACKET.timer, 10) : err;
    }
  }
  return err;
}

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
BmErr parse(void *payload) {
  BmErr err = BmEINVAL;
  BCMPParserData data;
  BCMPSequencedRequestCb cb = NULL;
  BCMPRequestElement *request_message = NULL;
  void *buf = NULL;

  if (payload && PACKET.initialized) {
    buf = PACKET.cb.data(payload);
    data.header = (BCMPHeader *)buf;
    data.payload = (uint8_t *)(buf + sizeof(BCMPHeader));
    data.src = PACKET.cb.src_ip(payload);
    data.dst = PACKET.cb.dst_ip(payload);
    data.ingress_port = (((uint32_t *)(data.src))[1] >> 8) & 0xFF;
    check_endianness(data.header, BCMPHeaderMessage);

    // Handle parsed message type
    for (uint32_t i = 0; i < array_size(PACKET_LUT); i++) {
      if (data.header->type == PACKET_LUT[i].type) {
        check_endianness(data.payload, data.header->type);

        // Check if this message is a reply to a message we sent
        if (PACKET_LUT[i].sequenced_reply && !PACKET_LUT[i].sequenced_request) {
          BCMPRequestElement *request_message =
              sequence_list_find_message(data.header->seq_num);
          if (request_message) {
            printf("BCMP - Received reply to our request message with seq_num %d\n",
                   data.header->seq_num);
            if (bm_semaphore_take(PACKET.sequence_list_semaphore, default_message_timeout_ms) ==
                BmOK) {
              cb = request_message->cb;
              sequence_list_remove_message(request_message);
              bm_semaphore_give(PACKET.sequence_list_semaphore);
            }
          }
        }

        if (cb) {
          // If message is a reply, utilize associated cb
          err = cb(data.payload);
        } else {
          // Utilize LUT parsing callback
          if (PACKET_LUT[i].parse && (err = PACKET_LUT[i].parse(data)) != BmOK) {
            printf("Error completing parse cb: 0x%X\n", err);
          } else {
            break;
          }
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

 @return BmOK on success
 @return BmError on failure
 */
BmErr serialize(void *payload, void *data, BCMPMessageType type, uint32_t seq_num) {
  static uint32_t message_count = 0;
  BmErr err = BmEINVAL;
  BCMPHeader *header = NULL;
  uint32_t size = 0;
  bool sequenced_reply = false;
  BCMPSequencedRequestCb sequenced_request = NULL;
  BCMPRequestElement *request_message = NULL;

  if (payload && data && PACKET.initialized) {
    err = BmOK;

    // Check endianness of type and place into little endian form
    check_endianness(data, type);

    // Determine size of message type and if there is a sequenced reply/request
    for (uint32_t i = 0; i < array_size(PACKET_LUT); i++) {
      if (type == PACKET_LUT[i].type) {
        size = PACKET_LUT[i].size;
        sequenced_reply = PACKET_LUT[i].sequenced_reply;
        sequenced_request = PACKET_LUT[i].sequenced_request;
        break;
      }
    }

    header = PACKET.cb.data(payload);
    header->flags = 0;       // Unused
    header->reserved = 0;    // Unused
    header->frag_total = 0;  // Unused
    header->frag_id = 0;     // Unused
    header->next_header = 0; // Unused
    header->type = type;
    header->checksum = PACKET.cb.checksum(payload, size + sizeof(BCMPHeader));
    if (sequenced_reply) {
      // if we are replying to a message, use the sequence number from the received message
      header->seq_num = seq_num;
    } else if (sequenced_request) {
      // If we are sending a new request, use our own sequence number
      header->seq_num = message_count++;
      request_message = new_sequence_list_item(header->seq_num, header->type,
                                               default_message_timeout_ms, sequenced_request);
      sequence_list_add_message(request_message);
      printf("BCMP - Sending message with seq_num %d\n", header->seq_num);
    } else {
      // If the message doesn't use sequence numbers, set it to 0
      header->seq_num = 0;
    }

    // Format header in little endian format and append data onto payload
    check_endianness(header, BCMPHeaderMessage);
    memcpy(header + sizeof(BCMPHeader), data, size);
  }

  return err;
}
