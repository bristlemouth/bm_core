#include "bm_link_heartbeat_monitor.h"
#include "bm_os.h"
#include "messages.h"
#include "timer_callback_handler.h"
#include <string.h>

typedef struct {
  bool up;
  uint32_t last_hb_ticks;
} PortState;

typedef struct {
  BmLinkHeartbeatMonitorCfg cfg;
  PortState ports[BM_LINK_HEARTBEAT_MAX_PORTS];
  BmTimer timer;
  BmSemaphore lock;
  bool initialized;
} Monitor;

static Monitor MONITOR;

static bool is_bcmp_heartbeat(const uint8_t *frame, uint32_t len) {
  if (len < min_bcmp_frame_size) {
    return false;
  }
  // Ethertype: big-endian on the wire.
  uint16_t ethertype = (uint16_t)((frame[ethernet_type_offset] << 8) |
                                  frame[ethernet_type_offset + 1]);
  if (ethertype != ethernet_type_ipv6) {
    return false;
  }
  if (frame[ipv6_next_header_offset] != ip_proto_bcmp) {
    return false;
  }
  // BcmpHeader.type is a uint16_t at the start of the BCMP header. The rest
  // of bcmp/ uses the struct directly without explicit byte-swapping, on the
  // assumption that all BM nodes share endianness. Read in that same
  // convention (host-endian uint16_t).
  uint16_t bcmp_type;
  memcpy(&bcmp_type, frame + bcmp_header_offset, sizeof(bcmp_type));
  return bcmp_type == BcmpHeartbeatMessage;
}

// ----------------------------------------------------------------------------
// Liveness check
// ----------------------------------------------------------------------------

static void liveness_check_handler(void *arg) {
  (void)arg;
  if (!MONITOR.initialized) {
    return;
  }

  // Snapshot which ports just transitioned, then fire callbacks outside the
  // lock to keep lock ordering simple for consumers (their on_link_change
  // typically takes L2's callback-list lock).
  bool fire_down[BM_LINK_HEARTBEAT_MAX_PORTS] = {0};
  uint32_t now = bm_get_tick_count();
  uint32_t timeout_ticks = bm_ms_to_ticks(MONITOR.cfg.timeout_ms);

  bm_semaphore_take(MONITOR.lock, UINT32_MAX);
  for (uint8_t i = 0; i < MONITOR.cfg.num_ports; i++) {
    if (MONITOR.ports[i].up && time_remaining(MONITOR.ports[i].last_hb_ticks,
                                              now, timeout_ticks) == 0) {
      MONITOR.ports[i].up = false;
      fire_down[i] = true;
    }
  }
  bm_semaphore_give(MONITOR.lock);

  for (uint8_t i = 0; i < MONITOR.cfg.num_ports; i++) {
    if (fire_down[i]) {
      MONITOR.cfg.on_link_change(i, false, MONITOR.cfg.ctx);
    }
  }
}

static void liveness_timer_cb(BmTimer timer) {
  (void)timer;
  // Non-blocking: post the real work to the timer_callback_handler task.
  // A 0 ms timeout is intentional — if the handler queue is full, dropping
  // one liveness check is far cheaper than stalling the timer-service task.
  timer_callback_handler_send_cb(liveness_check_handler, NULL, 0);
}

// ----------------------------------------------------------------------------
// Public API
// ----------------------------------------------------------------------------

/*!
  @brief Initialize the singleton monitor and start its check timer.

  @param cfg Configuration; copied internally.

  @return BmOK on success.
  @return BmEINVAL on null pointer, num_ports out of range, or null callback.
  @return BmEALREADY if init was already called (call deinit first).
  @return BmENOMEM if the timer or lock cannot be allocated.
 */
BmErr bm_link_heartbeat_monitor_init(const BmLinkHeartbeatMonitorCfg *cfg) {
  if (!cfg || !cfg->on_link_change) {
    return BmEINVAL;
  }
  if (cfg->num_ports == 0 || cfg->num_ports > BM_LINK_HEARTBEAT_MAX_PORTS) {
    return BmEINVAL;
  }
  if (MONITOR.initialized) {
    return BmEALREADY;
  }

  memset(&MONITOR, 0, sizeof(MONITOR));
  MONITOR.cfg = *cfg;

  uint32_t now = bm_get_tick_count();
  for (uint8_t i = 0; i < MONITOR.cfg.num_ports; i++) {
    MONITOR.ports[i].up = MONITOR.cfg.start_ports_up;
    MONITOR.ports[i].last_hb_ticks = now;
  }

  MONITOR.lock = bm_mutex_create();
  if (!MONITOR.lock) {
    return BmENOMEM;
  }

  MONITOR.timer = bm_timer_create("bm_link_hb", MONITOR.cfg.check_period_ms,
                                  true, /* timer_id */ NULL, liveness_timer_cb);
  if (!MONITOR.timer) {
    bm_semaphore_delete(MONITOR.lock);
    MONITOR.lock = NULL;
    return BmENOMEM;
  }

  MONITOR.initialized = true;

  BmErr err = bm_timer_start(MONITOR.timer, 0);
  if (err != BmOK) {
    MONITOR.initialized = false;
    bm_timer_delete(MONITOR.timer, 0);
    MONITOR.timer = NULL;
    bm_semaphore_delete(MONITOR.lock);
    MONITOR.lock = NULL;
    return err;
  }

  // When ports start up, fire a synchronous up edge for each port so the
  // consumer's view (typically bm_l2's per-port link state) matches the
  // monitor's internal state. Without this, bm_l2 would gate heartbeat TX
  // on its default-down state and the link could not bootstrap.
  // Fired outside the lock to preserve the rule that on_link_change runs
  // unlocked; the timer cannot race here because last_hb_ticks was just
  // set to "now" for every port.
  if (MONITOR.cfg.start_ports_up) {
    for (uint8_t i = 0; i < MONITOR.cfg.num_ports; i++) {
      MONITOR.cfg.on_link_change(i, true, MONITOR.cfg.ctx);
    }
  }

  return BmOK;
}

/*!
  @brief Observe one received L2 frame.

  @details Should be called by the transport's RX path for every frame. The
           monitor inspects the bytes to decide whether the frame is a BCMP
           heartbeat; non-heartbeat frames are silently ignored (returning
           BmOK). On a heartbeat, refreshes per-port last-seen and may fire
           a synchronous up edge if the port was previously down.

  @param port_num One-based ingress port number (1..num_ports). Matches the
                  port number convention used elsewhere in bm_core (1..15;
                  0 means "all", which is invalid here).
  @param frame    Pointer to the raw L2 Ethernet frame. May be modified by
                  callers afterward; the monitor does not retain the
                  pointer.
  @param len      Frame length in bytes.

  @return BmOK on success (whether or not the frame was a heartbeat).
  @return BmEINVAL on null pointer, out-of-range port, or before init.
 */
BmErr bm_link_heartbeat_monitor_observe(uint8_t port_num, const uint8_t *frame,
                                        uint32_t len) {
  if (!MONITOR.initialized || !frame) {
    return BmEINVAL;
  }
  if (port_num == 0 || port_num > MONITOR.cfg.num_ports) {
    return BmEINVAL;
  }

  if (!is_bcmp_heartbeat(frame, len)) {
    return BmOK; // not an error; just not relevant
  }

  uint8_t idx = port_num - 1;
  bool fire_up = false;

  bm_semaphore_take(MONITOR.lock, UINT32_MAX);
  MONITOR.ports[idx].last_hb_ticks = bm_get_tick_count();
  if (!MONITOR.ports[idx].up) {
    MONITOR.ports[idx].up = true;
    fire_up = true;
  }
  bm_semaphore_give(MONITOR.lock);

  if (fire_up) {
    MONITOR.cfg.on_link_change(idx, true, MONITOR.cfg.ctx);
  }
  return BmOK;
}

/*!
  @brief Stop the timer, release internal resources, and mark the monitor
         uninitialized. Idempotent.

  @details Production code typically does not need to call this — the monitor
           is a boot-time singleton. Exposed mainly so unit tests can reset
           state between cases.

  @return BmOK in all cases.
 */
BmErr bm_link_heartbeat_monitor_deinit(void) {
  if (!MONITOR.initialized) {
    return BmOK; // idempotent
  }
  MONITOR.initialized = false;
  if (MONITOR.timer) {
    bm_timer_stop(MONITOR.timer, 0);
    bm_timer_delete(MONITOR.timer, 0);
    MONITOR.timer = NULL;
  }
  if (MONITOR.lock) {
    bm_semaphore_delete(MONITOR.lock);
    MONITOR.lock = NULL;
  }
  return BmOK;
}
