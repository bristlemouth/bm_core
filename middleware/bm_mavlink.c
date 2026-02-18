#include "bm_mavlink.h"
#include "bm_config.h"
#include "bm_ip.h"
#include "bm_os.h"
#include "middleware.h"

#define mavlink_port 14540

static const BmIpAddr link_local_mavlink_addr = {{
    0xFF,
    0x02,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x03,
}};

typedef struct {
  BmMavLinkRxEntry *rx_lut;
  uint16_t rx_lut_len;
  uint8_t sys_id;
  uint8_t comp_id;
} BmMavlinkCtx;

static BmMavlinkCtx ctx = {0};

static void mavlink_rx_cb(uint64_t node_id, void *buf, uint32_t size) {
  uint8_t *mavlink_buf = (uint8_t *)bm_udp_get_payload(buf);
  mavlink_status_t status;
  mavlink_message_t msg;
  int chan = MAVLINK_COMM_0;
  uint8_t message_found = 0;

  for (uint32_t i = 0; i < size; i++) {
    message_found =
        mavlink_parse_char(chan, ((uint8_t *)mavlink_buf)[i], &msg, &status);
    if (message_found) {
      bm_debug("Received MAVLink Message ID: %d\n", msg.msgid);
      break;
    }
  }

  if (!message_found) {
    // Reset the parser state
    mavlink_reset_channel_status(chan);
    return;
  }

  for (uint32_t i = 0; i < ctx.rx_lut_len; i++) {
    if (msg.msgid != ctx.rx_lut[i].msg_id) {
      continue;
    }

    if (ctx.rx_lut[i].rx_cb) {
      ctx.rx_lut[i].rx_cb(node_id, &msg, &status);
    }
    break;
  }
}

/*!
 @brief Initialize Bristlemouth MAVLink Middleware

 @details Adds new middleware by binding new message transmission
          and reception to IP address link_local_mavlink_addr and
          UDP port mavlink_port. rx_lut is expected to be statically
          defined, because this module references the originally created
          variable.

 @see mavlink_port
 @see link_local_mavlink_addr

 @param sys_id System ID assigned to this Bristlmouth node (MAVLink component)
 @param comp_id Component ID assigned to Bristlmouth node (MAVLink component)
 @param rx_lut Array of receive callbacks and correlated message IDs
 @param rx_lut_len Number of elements in the rx_lut

 @return BmOK on success
         BmEINVAL on invalid input arguments
         BmErr on failure
 */
BmErr bm_mavlink_init(uint8_t sys_id, uint8_t comp_id, BmMavLinkRxEntry *rx_lut,
                      uint16_t rx_lut_len) {
  if (!sys_id || !comp_id || !rx_lut || !rx_lut_len) {
    return BmEINVAL;
  }

  ctx.rx_lut = rx_lut;
  ctx.rx_lut_len = rx_lut_len;
  ctx.sys_id = sys_id;
  ctx.comp_id = comp_id;

  return bm_middleware_add_application(mavlink_port, link_local_mavlink_addr,
                                       mavlink_rx_cb);
}

/*!
 @brief Transmits MAVLink Message Over Bristlemouth

 @details Takes a packed MAVLink message, serializes it and sends it on the
          Bristlemouth network. Packing is done but utilizing the
          mavlink_msg_{xxx}_pack functions available in the MAVLink
          library's header files.

 @param msg Packed message to send on Bristlemouth

 @return BmOK on success
         BmErr on failure
 */
BmErr bm_mavlink_transmit(const mavlink_message_t *msg) {
  if (!msg) {
    return BmEINVAL;
  }

  void *udp_buf = bm_udp_new(MAVLINK_MAX_PACKET_LEN);
  if (!udp_buf) {
    return BmENOMEM;
  }

  uint8_t *mavlink_buf = bm_udp_get_payload(udp_buf);
  uint16_t mavlink_buf_len = mavlink_msg_to_send_buffer(mavlink_buf, msg);
  // Shrink buffer so MAVLINK_MAX_PACKET_LEN is not sent onto the line
  bm_ip_buf_shrink(udp_buf, mavlink_buf_len);
  BmErr err = bm_middleware_net_tx(mavlink_port, udp_buf, mavlink_buf_len);
  bm_udp_cleanup(udp_buf);

  return err;
}

/*!
 @brief Get System ID Assigned To This Bristlemouth Node (MAVLink Component)

 @return System ID
 */
uint8_t bm_mavlink_get_sys_id(void) { return ctx.sys_id; }

/*!
 @brief Get Component ID Assigned To This Bristlemouth Node (MAVLink Component)

 @return Component ID
 */
uint8_t bm_mavlink_get_comp_id(void) { return ctx.comp_id; }
