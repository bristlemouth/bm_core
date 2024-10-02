#include "mock_packet.h"

DEFINE_FFF_GLOBALS;

DEFINE_FAKE_VALUE_FUNC(BmErr, packet_init, BcmpGetIPAddr, BcmpGetIPAddr,
                       BcmpGetData, BcmpGetChecksum);
DEFINE_FAKE_VALUE_FUNC(BmErr, packet_add, BcmpPacketCfg *, BcmpMessageType);
DEFINE_FAKE_VALUE_FUNC(uint16_t, packet_checksum, void *, uint32_t);
DEFINE_FAKE_VALUE_FUNC(BmErr, process_received_message, void *, uint32_t);
DEFINE_FAKE_VALUE_FUNC(BmErr, serialize, void *, void *, uint32_t,
                       BcmpMessageType, uint32_t, BcmpSequencedRequestCb);
DEFINE_FAKE_VALUE_FUNC(BmErr, packet_remove, BcmpMessageType);
