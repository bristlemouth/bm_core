#include "ll.h"
#include "mock_packet.h"

struct PacketMeta {
  uint32_t count;
  uint32_t tracker;
  LL ll;
};

static struct PacketMeta PACKET_META = {0};

DEFINE_FFF_GLOBALS;

DEFINE_FAKE_VALUE_FUNC(BmErr, packet_init, BcmpGetIPAddr, BcmpGetIPAddr,
                       BcmpGetData, BcmpGetChecksum);
DEFINE_FAKE_VALUE_FUNC(uint16_t, packet_checksum, void *, uint32_t);
DEFINE_FAKE_VALUE_FUNC(BmErr, process_received_message, void *, uint32_t);
DEFINE_FAKE_VALUE_FUNC(BmErr, serialize, void *, void *, uint32_t,
                       BcmpMessageType, uint32_t, BcmpSequencedRequestCb);
DEFINE_FAKE_VALUE_FUNC(BmErr, packet_remove, BcmpMessageType);

/*!
 @brief Add Packet Item To Packet Processor/Serializer

 @details This will fail if packet_add_fail is configured

 @param cfg statically allocated configuration of packet
 @param type type of packet to add, this will be ll item ID

 @return BmOK on success
 @return BmError on failure
*/
BmErr packet_add(BcmpPacketCfg *cfg, BcmpMessageType type) {
  BmErr err = BmEINVAL;
  LLItem *item = NULL;

  PACKET_META.tracker++;
  if (PACKET_META.count && PACKET_META.tracker >= PACKET_META.count) {
    err = BmENOMEM;
  } else if (cfg) {
    item = ll_create_item(item, cfg, sizeof(BcmpPacketCfg), type);
    err = item ? ll_item_add(&PACKET_META.ll, item) : err;
  }

  return err;
}

/*!
 @brief Set Packet Add To Fail In Count Iterations

 @details Set count to zero for packet_add to never fail

 @param count number of times packet add will succeed before being failure
*/
void packet_add_fail(uint32_t count) { PACKET_META.count = count; }

/*!
 @brief Invoke A Processing Callback Of A Bcmp Message Type

 @param type message type to invoke callback on
*/
BmErr packet_process_invoke(BcmpMessageType type, BcmpProcessData data) {
  BmErr err = BmENODATA;
  BcmpPacketCfg *cfg = NULL;

  err = ll_get_item(&PACKET_META.ll, type, (void **)&cfg);
  if (err == BmOK && cfg->process) {
    err = cfg->process(data);
  }

  return err;
}

/*!
 @brief Cleanup Items Configured/Added To Packet
*/
void packet_cleanup(void) {
  LLItem *item = PACKET_META.ll.head;
  LLItem *prev = item;
  while (item != NULL) {
    item = item->next;
    ll_delete_item(prev);
  }
  PACKET_META.ll.head = NULL;
  PACKET_META.ll.tail = NULL;
  PACKET_META.ll.cursor = NULL;
  PACKET_META.tracker = 0;
  PACKET_META.count = 0;
}
