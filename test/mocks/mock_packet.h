#include "fff.h"
#include "packet.h"
#include "util.h"

DECLARE_FAKE_VALUE_FUNC(BmErr, packet_init, BcmpGetIPAddr, BcmpGetIPAddr,
                        BcmpGetData, BcmpGetChecksum);
DECLARE_FAKE_VALUE_FUNC(BmErr, packet_add, BcmpPacketCfg *, BcmpMessageType);
DECLARE_FAKE_VALUE_FUNC(uint16_t, packet_checksum, void *, uint32_t);
DECLARE_FAKE_VALUE_FUNC(BmErr, process_received_message, void *, uint32_t);
DECLARE_FAKE_VALUE_FUNC(BmErr, serialize, void *, void *, uint32_t,
                        BcmpMessageType, uint32_t, BcmpSequencedRequestCb);
DECLARE_FAKE_VALUE_FUNC(BmErr, packet_remove, BcmpMessageType);
void packet_add_fail(uint32_t count);
BmErr packet_process_invoke(BcmpMessageType type, BcmpProcessData data);
void packet_cleanup(void);
