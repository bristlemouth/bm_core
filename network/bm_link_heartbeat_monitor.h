#ifndef __BM_LINK_HEARTBEAT_MONITOR_H__
#define __BM_LINK_HEARTBEAT_MONITOR_H__

#include "util.h"
#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/*!
  @file bm_link_heartbeat_monitor.h
  @brief Heartbeat-driven link-state monitor for transports without a PHY.

  @details For transports that lack an out-of-band link signal (UART tunnels,
           network sockets, etc.), this helper infers per-port link state by
           watching the BCMP heartbeats already produced by every active node.

           The transport calls bm_link_heartbeat_monitor_observe() for every
           received L2 frame. Heartbeats refresh per-port liveness; an
           internal timer fires the configured on_link_change callback when a
           port times out or comes back online.

           This module is a singleton: a single monitor lives in the .c file's
           BSS (~150 bytes on a 32-bit target). Executables that never call
           init pay only this fixed cost; with linker --gc-sections it is
           reclaimed entirely. Multi-instance use is intentionally unsupported.

           This module is the software analogue of bm_adin2111's hardware
           link-change interrupt. It does not modify L2, BCMP, or any other
           bm_core component.
 */

/// Maximum number of ports the monitor will accept. BM L2 encodes ingress/
/// egress ports in 4 bits, so 15 is the spec limit.
#define BM_LINK_HEARTBEAT_MAX_PORTS 15

/*!
  @brief Edge-triggered link-state callback.

  @details Fired once per state transition (up<->down). Not called during
           steady state. Called outside the monitor's internal lock so the
           callback may safely acquire other locks (typically the L2
           callback registry).

  @param port_idx Zero-based port index (port_num - 1).
  @param up      True if the port has become live, false if it has timed out.
  @param ctx     Caller-supplied context from BmLinkHeartbeatMonitorCfg.
 */
typedef void (*BmLinkHeartbeatMonitorCb)(uint8_t port_idx, bool up, void *ctx);

/*!
  @brief Configuration passed to bm_link_heartbeat_monitor_init.

  @details All fields are copied into the monitor; the cfg pointer need not
           remain valid after init returns.
 */
typedef struct {
  /// Number of ports to track (1..BM_LINK_HEARTBEAT_MAX_PORTS).
  uint8_t num_ports;

  /// Mark a port down after this many milliseconds with no observed
  /// heartbeat. Should be a small multiple of the BCMP heartbeat period
  /// (typically 2 * 10s = 20000 ms).
  uint32_t timeout_ms;

  /// How often the monitor wakes to check for timed-out ports.
  /// Setting this lower trades wake-ups for sharper down-edge timing.
  /// Typical value: 1000.
  uint32_t check_period_ms;

  /// If true, all ports start in the up state and no initial callback is
  /// fired. The first down edge can occur after timeout_ms with no
  /// observed heartbeat. Use this when the transport's send path does
  /// not gate on link state and you want heartbeats to flow at boot.
  ///
  /// If false, all ports start down. The first up edge fires when a
  /// heartbeat is observed. Note that L2 will mask TX on ports it
  /// believes are down (see bm_l2_tx in network/l2.c) — only set this to
  /// false if the transport bypasses that mask, otherwise the link can
  /// never bootstrap.
  bool start_ports_up;

  /// Edge-triggered callback. Required (init returns BmEINVAL if null).
  BmLinkHeartbeatMonitorCb on_link_change;

  /// Opaque context passed to on_link_change.
  void *ctx;
} BmLinkHeartbeatMonitorCfg;

BmErr bm_link_heartbeat_monitor_init(const BmLinkHeartbeatMonitorCfg *cfg);
BmErr bm_link_heartbeat_monitor_observe(uint8_t port_num, const uint8_t *frame,
                                        uint32_t len);
BmErr bm_link_heartbeat_monitor_deinit(void);

#ifdef __cplusplus
}
#endif

#endif // __BM_LINK_HEARTBEAT_MONITOR_H__
