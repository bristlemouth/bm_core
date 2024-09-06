#include "packet.h"
#include "util.h"
#include <stddef.h>
#include <string.h>

struct SizeLUT {
  uint32_t size;
  void (*cb)(void *);
};

static struct SizeLUT SERIAL_LUT[] = {
    [BCMP_PARSER_8BIT] = {sizeof(uint8_t), NULL},
    [BCMP_PARSER_16BIT] = {sizeof(uint16_t), swap_16bit},
    [BCMP_PARSER_32BIT] = {sizeof(uint32_t), swap_32bit},
    [BCMP_PARSER_64BIT] = {sizeof(uint64_t), swap_64bit},
};

static struct Parser {
  LL ll;
  uint32_t c;
} PARSER;

/*!
 @brief Handler For Parser Traverse Callback Function

 @param buf configuration of the message item
 @param arg data to be processed by the message

 @return BmOK on success
 @return BmError on failure
 */
static BMErr parser_handler(void *buf, void *arg) {
  BMErr err = BmEINVAL;
  BCMPMessageParserCfg *cfg = NULL;
  BCMPParserData data;

  if (arg && buf) {
    err = BmOK;
    cfg = (BCMPMessageParserCfg *)buf;
    data = *(BCMPParserData *)arg;
    for (uint32_t i = 0; i < cfg->msg.count; i++) {
      if (data.header->type == cfg->msg.types[i]) {
        if (cfg->cb && cfg->cb(data) != BmOK) {
          //TODO log something clever here
        } else {
          break;
        }
      }
    }
  }

  return err;
}

/*!
 @brief Initialize Packet Parser

 @return BmOK
 */
BMErr bcmp_parser_init(void) {
  memset(&PARSER, 0, sizeof(struct Parser));
  return BmOK;
}

/*!
 @brief Add Item To Be Parsed When Handling Incoming Messages

 @details This will be a linked list item that is serviced in
          the update function

 @param item parser item to add to linked list

 @return BmOK on success
 @return BmError on failure
 */
BMErr bcmp_parser_add(BCMPParserItem *item) {
  BMErr err = BmEINVAL;
  LLItem *object = &item->object;

  if (item) {
    object->data = &item->cfg;
    object->id = PARSER.c++;
    err = ll_item_add(&PARSER.ll, object);
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
BMErr bcmp_parse_update(struct pbuf *pbuf, ip_addr_t *src, ip_addr_t *dst) {
  BMErr err = BmEINVAL;
  BCMPParserData data;

  data.header = (BCMPHeader *)pbuf->payload;
  data.payload = (uint8_t *)(pbuf->payload + sizeof(BCMPHeader));
  data.pbuf = pbuf;
  data.ingress_port = ((src->addr[1] >> 8) & 0xFF);
  bcmp_serialize_data(err, data.header, bcmp_header);

  err = ll_traverse(&PARSER.ll, parser_handler, &data);

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
BMErr bcmp_serialize(void *buf, BCMPSerializeInfo info) {
  BMErr err = BmEINVAL;
  uint32_t i = 0;
  struct SizeLUT element;

  if (buf && info.lengths && info.size) {
    err = BmOK;
    if (is_big_endian()) {
      while (info.size) {
        element = SERIAL_LUT[info.lengths[i++]];
        if (element.cb) {
          element.cb(buf);
        }
        buf += element.size;
        info.size -= element.size;
      }
    }
  }

  return err;
}
