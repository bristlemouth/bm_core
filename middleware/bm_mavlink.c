#include "bm_mavlink.h"
#include "bm_config.h"
#include "bm_ip.h"
#include "bm_os.h"
#include "l2.h"
#include "middleware.h"

#define mavlink_port 14540
#define mavlink_heartbeat_period_ms 1000

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
  BmTimer heartbeat_timer;
  uint16_t rx_lut_len;
  uint8_t sys_id;
  uint8_t comp_id;
  uint8_t type;
  uint8_t state;
} BmMavlinkCtx;

static BmMavlinkCtx ctx = {0};

static bool mavlink_routing_cb(uint8_t ingress_port, uint16_t *egress_mask,
                               BmIpAddr *src) {
  (void)src;
  // TODO: Implement actual MAVLink routing here, for now forward to other ports
  // and handle every incoming message
  uint8_t port_count = bm_l2_get_port_count();
  for (uint8_t i = 0; i < port_count; i++) {
    bool port_state = bm_l2_get_port_state(i);
    if (port_state && i != (ingress_port - 1)) {
      *egress_mask |= 1 << i;
    }
  }

  return true;
}

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

static void mavlink_heartbeat_cb(BmTimer timer) {
  (void)timer;

  mavlink_message_t msg;
  MAV_AUTOPILOT autopilot = MAV_AUTOPILOT_INVALID;
  uint8_t base_mode = 0;
  uint32_t custom_mode = 0;

  mavlink_msg_heartbeat_pack(ctx.sys_id, ctx.comp_id, &msg, ctx.type, autopilot,
                             base_mode, custom_mode, ctx.state);
  bm_mavlink_transmit(&msg);
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

 @param info MAVLink component information
 @param init_state Initial state of the MAVLink component 
 @param rx_lut Array of receive callbacks and correlated message IDs
 @param rx_lut_len Number of elements in the rx_lut

 @return BmOK on success
         BmEINVAL on invalid input arguments
         BmENOMEM if heartbeat timer fails to be created
         BmErr on failure
 */
BmErr bm_mavlink_init(BmMavLinkInfo info, MAV_STATE init_state,
                      BmMavLinkRxEntry *rx_lut, uint16_t rx_lut_len) {
  if (!info.sys_id || !info.comp_id || !rx_lut || !rx_lut_len) {
    return BmEINVAL;
  }

  ctx.rx_lut = rx_lut;
  ctx.rx_lut_len = rx_lut_len;
  ctx.sys_id = info.sys_id;
  ctx.comp_id = info.comp_id;
  ctx.type = info.type;
  ctx.state = init_state;

  ctx.heartbeat_timer =
      bm_timer_create("mavlink_heartbeat", mavlink_heartbeat_period_ms, true,
                      NULL, mavlink_heartbeat_cb);

  if (!ctx.heartbeat_timer) {
    return BmENOMEM;
  }

  BmErr err = bm_timer_start(ctx.heartbeat_timer, 0);
  if (err != BmOK) {
    return err;
  }

  return bm_middleware_add_application(mavlink_port, link_local_mavlink_addr,
                                       mavlink_rx_cb, mavlink_routing_cb);
}

/*!
 @brief Set The MAVLink State Of The Bristlemouth Node (MAVLink Component)

 @details When initialized with bm_mavlink_init, the state will be set
          to the init_state argument. This is intended to be used after
          bm_mavlink_init.

 @param state State to set the MAVLink device to
 */
void bm_mavlink_set_state(MAV_STATE state) { ctx.state = state; }

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
