#include "bcmp.h"

#include "heartbeat.h"

BmErr bcmp_init(void) {
  BmErr err = BmOK;
  err = bcmp_heartbeat_init();
  return err;
}

//TODO: place callbacks in their respective module
//static struct SerialLUT PACKET_LUT[] = {
//    {
//        BcmpHeartbeatMessage,
//        sizeof(BcmpHeartbeat),
//        false,
//        NULL,
//        process_heartbeat,
//    },
//    {
//        BcmpEchoRequestMessage,
//        sizeof(BcmpEchoRequest),
//        false,
//        NULL,
//        NULL,
//    },
//    {
//        BcmpEchoReplyMessage,
//        sizeof(BcmpEchoReply),
//        false,
//        NULL,
//        NULL,
//    },
//    {
//        BcmpDeviceInfoRequestMessage,
//        sizeof(BcmpDeviceInfoRequest),
//        false,
//        NULL,
//        NULL,
//    },
//    {
//        BcmpDeviceInfoReplyMessage,
//        sizeof(BcmpDeviceInfoReply),
//        false,
//        NULL,
//        NULL,
//    },
//    {
//        BcmpProtocolCapsRequestMessage,
//        sizeof(BcmpProtocolCapsRequest),
//        false,
//        NULL,
//        NULL,
//    },
//    {
//        BcmpProtocolCapsReplyMessage,
//        sizeof(BcmpProtocolCapsReply),
//        false,
//        NULL,
//        NULL,
//    },
//    {
//        BcmpNeighborTableRequestMessage,
//        sizeof(BcmpNeighborTableRequest),
//        false,
//        NULL,
//        NULL,
//    },
//    {
//        BcmpNeighborTableReplyMessage,
//        sizeof(BcmpNeighborTableReply),
//        false,
//        NULL,
//        NULL,
//    },
//    {
//        BcmpResourceTableRequestMessage,
//        sizeof(BcmpResourceTableRequest),
//        false,
//        NULL,
//        NULL,
//    },
//    {
//        BcmpResourceTableReplyMessage,
//        sizeof(BcmpResourceTableReply),
//        false,
//        NULL,
//        NULL,
//    },
//    {
//        BcmpNeighborProtoRequestMessage,
//        sizeof(BcmpNeighborProtoRequest),
//        false,
//        NULL,
//        NULL,
//    },
//    {
//        BcmpNeighborTableReplyMessage,
//        sizeof(BcmpNeighborTableReply),
//        false,
//        NULL,
//        NULL,
//    },
//    {
//        BcmpSystemTimeRequestMessage,
//        sizeof(BcmpSystemTimeRequest),
//        false,
//        NULL,
//        NULL,
//    },
//    {
//        BcmpSystemTimeResponseMessage,
//        sizeof(BcmpSystemTimeResponse),
//        false,
//        NULL,
//        NULL,
//    },
//    {
//        BcmpSystemTimeSetMessage,
//        sizeof(BcmpSystemTimeSet),
//        false,
//        NULL,
//        NULL,
//    },
//    {
//        BcmpNetStateRequestMessage,
//        sizeof(BcmpNetStateRequest),
//        false,
//        NULL,
//        NULL,
//    },
//    {
//        BcmpNetStateReplyMessage,
//        sizeof(BcmpNetStateReply),
//        false,
//        NULL,
//        NULL,
//    },
//    {
//        BcmpPowerStateRequestMessage,
//        sizeof(BcmpPowerStateRequest),
//        false,
//        NULL,
//        NULL,
//    },
//    {
//        BcmpPowerStateReplyMessage,
//        sizeof(BcmpPowerStateReply),
//        false,
//        NULL,
//        NULL,
//    },
//    {
//        BcmpRebootRequestMessage,
//        sizeof(BcmpRebootRequest),
//        false,
//        NULL,
//        NULL,
//    },
//    {
//        BcmpRebootReplyMessage,
//        sizeof(BcmpRebootReply),
//        false,
//        NULL,
//        NULL,
//    },
//    {
//        BcmpNetAssertQuietMessage,
//        sizeof(BcmpNetAssertQuiet),
//        false,
//        NULL,
//        NULL,
//    },
//    // TODO: Set the size and callbacks of down belo
//    {
//        BcmpConfigGetMessage,
//        1,
//        false,
//        NULL,
//        NULL,
//    },
//    {
//        BcmpConfigValueMessage,
//        1,
//        true,
//        NULL,
//        NULL,
//    },
//    {
//        BcmpConfigSetMessage,
//        1,
//        false,
//        NULL,
//        NULL,
//    },
//    {
//        BcmpConfigCommitMessage,
//        1,
//        false,
//        NULL,
//        NULL,
//    },
//    {
//        BcmpConfigStatusRequestMessage,
//        1,
//        false,
//        NULL,
//        NULL,
//    },
//    {
//        BcmpConfigStatusResponseMessage,
//        1,
//        true,
//        NULL,
//        NULL,
//    },
//    {
//        BcmpConfigDeleteRequestMessage,
//        1,
//        false,
//        NULL,
//        NULL,
//    },
//    {
//        BcmpConfigDeleteResponseMessage,
//        1,
//        true,
//        NULL,
//        NULL,
//    },
//    {
//        BcmpDFUStartMessage,
//        1,
//        false,
//        NULL,
//        NULL,
//    },
//    {
//        BcmpDFUPayloadReqMessage,
//        1,
//        false,
//        NULL,
//        NULL,
//    },
//    {
//        BcmpDFUPayloadMessage,
//        1,
//        false,
//        NULL,
//        NULL,
//    },
//    {
//        BcmpDFUEndMessage,
//        1,
//        false,
//        NULL,
//        NULL,
//    },
//    {
//        BcmpDFUAckMessage,
//        1,
//        false,
//        NULL,
//        NULL,
//    },
//    {
//        BcmpDFUAbortMessage,
//        1,
//        false,
//        NULL,
//        NULL,
//    },
//    {
//        BcmpDFUHeartbeatMessage,
//        1,
//        false,
//        NULL,
//        NULL,
//    },
//    {
//        BcmpDFURebootReqMessage,
//        1,
//        false,
//        NULL,
//        NULL,
//    },
//    {
//        BcmpDFURebootMessage,
//        1,
//        false,
//        NULL,
//        NULL,
//    },
//    {
//        BcmpDFUBootCompleteMessage,
//        1,
//        false,
//        NULL,
//        NULL,
//    },
//};