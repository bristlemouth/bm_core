#include <gtest/gtest.h>
#include <helpers.hpp>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>

#include "fff.h"

DEFINE_FFF_GLOBALS;

extern "C" {
#include "bm_link_heartbeat_monitor.h"
#include "mock_bm_os.h"
#include "mock_timer_callback_handler.h"
}

// ---------------------------------------------------------------------------
// Test fixtures
// ---------------------------------------------------------------------------

namespace {

constexpr uint32_t TIMEOUT_MS = 20000;
constexpr uint32_t CHECK_PERIOD_MS = 1000;

// Track on_link_change invocations.
struct LinkChangeRecord {
  uint8_t port_idx;
  bool up;
};

struct CallbackTracker {
  std::vector<LinkChangeRecord> events;
  void *expected_ctx = nullptr;
  uint32_t bad_ctx_count = 0;
};

void on_link_change(uint8_t port_idx, bool up, void *ctx) {
  auto *t = static_cast<CallbackTracker *>(ctx);
  if (t->expected_ctx != nullptr && ctx != t->expected_ctx) {
    t->bad_ctx_count++;
  }
  t->events.push_back({port_idx, up});
}

// ---------------------------------------------------------------------------
// Frame builders
// ---------------------------------------------------------------------------
//
// Mirror the on-wire layout used by network/l2.c:
//   [0..5]   destination MAC
//   [6..11]  source MAC
//   [12..13] ethertype           (big-endian)
//   [14..17] IPv6 vers/class/flow
//   [18..19] IPv6 payload length (big-endian)
//   [20]     IPv6 next header
//   [21]     IPv6 hop limit
//   [22..37] IPv6 source addr
//   [38..53] IPv6 dest addr
//   [54..]   upper-layer payload

constexpr size_t ETH_HDR_LEN = 14;
constexpr size_t IPV6_HDR_LEN = 40;
constexpr size_t FRAME_HDR_LEN = ETH_HDR_LEN + IPV6_HDR_LEN;
constexpr size_t BCMP_HDR_LEN = 12;
constexpr uint16_t ETH_TYPE_IPV6 = 0x86DD;
constexpr uint8_t IP_PROTO_BCMP = 0xBC;
constexpr uint16_t HEARTBEAT_TYPE = 0x0001;
constexpr uint16_t INFO_REQUEST_TYPE = 0x0004; // some other BCMP type

std::vector<uint8_t> build_frame(uint16_t ethertype, uint8_t next_header,
                                 uint16_t bcmp_type, size_t total_len) {
  std::vector<uint8_t> f(total_len, 0);
  f[12] = static_cast<uint8_t>(ethertype >> 8);
  f[13] = static_cast<uint8_t>(ethertype & 0xFF);
  f[20] = next_header;
  if (total_len >= FRAME_HDR_LEN + sizeof(bcmp_type)) {
    memcpy(f.data() + FRAME_HDR_LEN, &bcmp_type, sizeof(bcmp_type));
  }
  return f;
}

std::vector<uint8_t> build_heartbeat_frame() {
  return build_frame(ETH_TYPE_IPV6, IP_PROTO_BCMP, HEARTBEAT_TYPE,
                     FRAME_HDR_LEN + BCMP_HDR_LEN);
}

std::vector<uint8_t> build_bcmp_non_heartbeat_frame() {
  return build_frame(ETH_TYPE_IPV6, IP_PROTO_BCMP, INFO_REQUEST_TYPE,
                     FRAME_HDR_LEN + BCMP_HDR_LEN);
}

std::vector<uint8_t> build_udp_frame() {
  return build_frame(ETH_TYPE_IPV6, /*UDP*/ 0x11, 0, FRAME_HDR_LEN + 8);
}

// ---------------------------------------------------------------------------
// Mock helpers
// ---------------------------------------------------------------------------
//
// The mocked bm_timer_create returns a sentinel pointer; we don't need it to
// fire on its own, the tests invoke the captured callback directly with a
// controlled bm_get_tick_count.

void *MUTEX_HANDLE = reinterpret_cast<void *>(0xA11CABBA);
void *TIMER_HANDLE = reinterpret_cast<void *>(0xCAFEFEED);

// Captured timer callback so tests can drive the down-edge logic directly.
BmTimerCb captured_timer_cb = nullptr;

BmTimer create_timer_capture(const char *name, uint32_t period_ms,
                             bool auto_reload, void *time_id, BmTimerCb cb) {
  (void)name;
  (void)period_ms;
  (void)auto_reload;
  (void)time_id;
  captured_timer_cb = cb;
  return TIMER_HANDLE;
}

uint32_t mocked_now_ms = 0;
uint32_t get_tick_count_custom() { return mocked_now_ms; }
uint32_t ms_to_ticks_passthrough(uint32_t ms) { return ms; }

// Synchronously runs the offloaded handler. The production code posts the
// liveness scan to the timer_callback_handler task; in unit tests we just
// invoke it inline so the existing tests can drive the down-edge logic by
// calling captured_timer_cb() directly.
bool send_cb_passthrough(timer_handler_cb cb, void *arg, uint32_t timeout_ms) {
  (void)timeout_ms;
  if (cb) {
    cb(arg);
  }
  return true;
}

void install_default_os_mocks() {
  RESET_FAKE(bm_mutex_create);
  RESET_FAKE(bm_semaphore_take);
  RESET_FAKE(bm_semaphore_give);
  RESET_FAKE(bm_semaphore_delete);
  RESET_FAKE(bm_timer_create);
  RESET_FAKE(bm_timer_start);
  RESET_FAKE(bm_timer_stop);
  RESET_FAKE(bm_timer_delete);
  RESET_FAKE(bm_get_tick_count);
  RESET_FAKE(bm_ms_to_ticks);
  RESET_FAKE(timer_callback_handler_send_cb);

  bm_mutex_create_fake.return_val = MUTEX_HANDLE;
  bm_semaphore_take_fake.return_val = BmOK;
  bm_semaphore_give_fake.return_val = BmOK;
  bm_timer_create_fake.custom_fake = create_timer_capture;
  bm_timer_start_fake.return_val = BmOK;
  bm_timer_stop_fake.return_val = BmOK;
  bm_get_tick_count_fake.custom_fake = get_tick_count_custom;
  bm_ms_to_ticks_fake.custom_fake = ms_to_ticks_passthrough;
  timer_callback_handler_send_cb_fake.custom_fake = send_cb_passthrough;

  captured_timer_cb = nullptr;
  mocked_now_ms = 1000;
}

class HeartbeatMonitor : public ::testing::Test {
protected:
  CallbackTracker tracker;

  void SetUp() override { install_default_os_mocks(); }

  void TearDown() override { bm_link_heartbeat_monitor_deinit(); }

  BmLinkHeartbeatMonitorCfg make_cfg(uint8_t num_ports = 4,
                                     bool start_up = false) {
    BmLinkHeartbeatMonitorCfg cfg{};
    cfg.num_ports = num_ports;
    cfg.timeout_ms = TIMEOUT_MS;
    cfg.check_period_ms = CHECK_PERIOD_MS;
    cfg.start_ports_up = start_up;
    cfg.on_link_change = on_link_change;
    cfg.ctx = &tracker;
    tracker.expected_ctx = &tracker;
    return cfg;
  }
};

} // namespace

// ---------------------------------------------------------------------------
// init / deinit
// ---------------------------------------------------------------------------

TEST_F(HeartbeatMonitor, init_rejects_invalid_args) {
  EXPECT_EQ(bm_link_heartbeat_monitor_init(nullptr), BmEINVAL);

  BmLinkHeartbeatMonitorCfg cfg = make_cfg();
  cfg.on_link_change = nullptr;
  EXPECT_EQ(bm_link_heartbeat_monitor_init(&cfg), BmEINVAL);

  cfg = make_cfg();
  cfg.num_ports = 0;
  EXPECT_EQ(bm_link_heartbeat_monitor_init(&cfg), BmEINVAL);

  cfg = make_cfg();
  cfg.num_ports = BM_LINK_HEARTBEAT_MAX_PORTS + 1;
  EXPECT_EQ(bm_link_heartbeat_monitor_init(&cfg), BmEINVAL);
}

TEST_F(HeartbeatMonitor, init_creates_timer_and_starts_it) {
  BmLinkHeartbeatMonitorCfg cfg = make_cfg();
  EXPECT_EQ(bm_link_heartbeat_monitor_init(&cfg), BmOK);

  EXPECT_EQ(bm_timer_create_fake.call_count, 1u);
  EXPECT_EQ(bm_timer_create_fake.arg1_val, CHECK_PERIOD_MS); // period
  EXPECT_TRUE(bm_timer_create_fake.arg2_val);                // auto_reload
  EXPECT_EQ(bm_timer_start_fake.call_count, 1u);
  EXPECT_NE(captured_timer_cb, nullptr);

  EXPECT_EQ(tracker.events.size(), 0u); // no spurious callbacks at init
}

TEST_F(HeartbeatMonitor, init_returns_enomem_when_timer_create_fails) {
  bm_timer_create_fake.custom_fake = nullptr;
  bm_timer_create_fake.return_val = nullptr;

  BmLinkHeartbeatMonitorCfg cfg = make_cfg();
  EXPECT_EQ(bm_link_heartbeat_monitor_init(&cfg), BmENOMEM);
}

TEST_F(HeartbeatMonitor, deinit_is_idempotent_and_releases_resources) {
  BmLinkHeartbeatMonitorCfg cfg = make_cfg();
  ASSERT_EQ(bm_link_heartbeat_monitor_init(&cfg), BmOK);
  EXPECT_EQ(bm_link_heartbeat_monitor_deinit(), BmOK);
  EXPECT_EQ(bm_timer_stop_fake.call_count, 1u);
  EXPECT_EQ(bm_timer_delete_fake.call_count, 1u);
  // Second call on deinited monitor: still OK, no further OS work.
  EXPECT_EQ(bm_link_heartbeat_monitor_deinit(), BmOK);
  EXPECT_EQ(bm_timer_stop_fake.call_count, 1u);
  EXPECT_EQ(bm_timer_delete_fake.call_count, 1u);
}

// ---------------------------------------------------------------------------
// observe()
// ---------------------------------------------------------------------------

TEST_F(HeartbeatMonitor, observe_rejects_invalid_args) {
  BmLinkHeartbeatMonitorCfg cfg = make_cfg(/*num_ports=*/2);
  ASSERT_EQ(bm_link_heartbeat_monitor_init(&cfg), BmOK);

  auto frame = build_heartbeat_frame();
  EXPECT_EQ(bm_link_heartbeat_monitor_observe(1, nullptr, frame.size()),
            BmEINVAL);
  EXPECT_EQ(bm_link_heartbeat_monitor_observe(0, frame.data(), frame.size()),
            BmEINVAL);
  EXPECT_EQ(bm_link_heartbeat_monitor_observe(3, frame.data(), frame.size()),
            BmEINVAL);
}

TEST_F(HeartbeatMonitor, observe_ignores_non_heartbeat_traffic) {
  BmLinkHeartbeatMonitorCfg cfg = make_cfg();
  ASSERT_EQ(bm_link_heartbeat_monitor_init(&cfg), BmOK);

  auto udp = build_udp_frame();
  auto bcmp_other = build_bcmp_non_heartbeat_frame();
  std::vector<uint8_t> tiny(10, 0); // too short

  EXPECT_EQ(bm_link_heartbeat_monitor_observe(1, udp.data(), udp.size()), BmOK);
  EXPECT_EQ(bm_link_heartbeat_monitor_observe(1, bcmp_other.data(),
                                              bcmp_other.size()),
            BmOK);
  EXPECT_EQ(bm_link_heartbeat_monitor_observe(1, tiny.data(), tiny.size()),
            BmOK);

  EXPECT_EQ(tracker.events.size(), 0u);
}

TEST_F(HeartbeatMonitor, observe_first_heartbeat_fires_up_edge) {
  BmLinkHeartbeatMonitorCfg cfg = make_cfg(/*num_ports=*/2);
  ASSERT_EQ(bm_link_heartbeat_monitor_init(&cfg), BmOK);

  auto hb = build_heartbeat_frame();
  EXPECT_EQ(bm_link_heartbeat_monitor_observe(2, hb.data(), hb.size()), BmOK);

  ASSERT_EQ(tracker.events.size(), 1u);
  EXPECT_EQ(tracker.events[0].port_idx, 1); // port_num 2 -> idx 1
  EXPECT_TRUE(tracker.events[0].up);
}

TEST_F(HeartbeatMonitor, observe_subsequent_heartbeats_do_not_refire) {
  BmLinkHeartbeatMonitorCfg cfg = make_cfg();
  ASSERT_EQ(bm_link_heartbeat_monitor_init(&cfg), BmOK);

  auto hb = build_heartbeat_frame();
  for (int i = 0; i < 5; i++) {
    mocked_now_ms += 1000;
    EXPECT_EQ(bm_link_heartbeat_monitor_observe(1, hb.data(), hb.size()), BmOK);
  }
  EXPECT_EQ(tracker.events.size(), 1u); // only the first one fired
}

TEST_F(HeartbeatMonitor, observe_per_port_independence) {
  BmLinkHeartbeatMonitorCfg cfg = make_cfg(/*num_ports=*/3);
  ASSERT_EQ(bm_link_heartbeat_monitor_init(&cfg), BmOK);

  auto hb = build_heartbeat_frame();
  EXPECT_EQ(bm_link_heartbeat_monitor_observe(1, hb.data(), hb.size()), BmOK);
  EXPECT_EQ(bm_link_heartbeat_monitor_observe(3, hb.data(), hb.size()), BmOK);

  ASSERT_EQ(tracker.events.size(), 2u);
  EXPECT_EQ(tracker.events[0].port_idx, 0);
  EXPECT_EQ(tracker.events[1].port_idx, 2);
  EXPECT_TRUE(tracker.events[0].up);
  EXPECT_TRUE(tracker.events[1].up);
}

// ---------------------------------------------------------------------------
// Timer-driven down edges
// ---------------------------------------------------------------------------

TEST_F(HeartbeatMonitor, timer_fires_down_edge_after_timeout) {
  BmLinkHeartbeatMonitorCfg cfg = make_cfg(/*num_ports=*/2);
  ASSERT_EQ(bm_link_heartbeat_monitor_init(&cfg), BmOK);
  ASSERT_NE(captured_timer_cb, nullptr);

  // Bring port 1 up.
  auto hb = build_heartbeat_frame();
  ASSERT_EQ(bm_link_heartbeat_monitor_observe(1, hb.data(), hb.size()), BmOK);
  ASSERT_EQ(tracker.events.size(), 1u);

  // Tick the timer at t = last_hb + timeout - 1 -- still within the window.
  mocked_now_ms += TIMEOUT_MS - 1;
  captured_timer_cb(TIMER_HANDLE);
  EXPECT_EQ(tracker.events.size(), 1u);

  // Tick past timeout: edge to down expected.
  mocked_now_ms += 2;
  captured_timer_cb(TIMER_HANDLE);
  ASSERT_EQ(tracker.events.size(), 2u);
  EXPECT_EQ(tracker.events[1].port_idx, 0);
  EXPECT_FALSE(tracker.events[1].up);

  // Subsequent ticks do not re-fire while still down.
  mocked_now_ms += TIMEOUT_MS;
  captured_timer_cb(TIMER_HANDLE);
  EXPECT_EQ(tracker.events.size(), 2u);
}

TEST_F(HeartbeatMonitor, timer_cb_offloads_to_handler_task) {
  // FreeRTOS docs forbid blocking inside the timer callback. Verify
  // the callback delegates via timer_callback_handler_send_cb rather
  // than running the (mutex-taking) liveness scan inline.
  BmLinkHeartbeatMonitorCfg cfg = make_cfg();
  ASSERT_EQ(bm_link_heartbeat_monitor_init(&cfg), BmOK);
  ASSERT_NE(captured_timer_cb, nullptr);

  uint32_t take_calls_before = bm_semaphore_take_fake.call_count;
  uint32_t send_cb_calls_before =
      timer_callback_handler_send_cb_fake.call_count;

  // Disable the passthrough so the bottom half does NOT run inline; we
  // only want to observe what the top half does.
  timer_callback_handler_send_cb_fake.custom_fake = nullptr;
  timer_callback_handler_send_cb_fake.return_val = true;

  captured_timer_cb(TIMER_HANDLE);

  EXPECT_EQ(timer_callback_handler_send_cb_fake.call_count,
            send_cb_calls_before + 1);
  // Timer callback must not have taken the lock.
  EXPECT_EQ(bm_semaphore_take_fake.call_count, take_calls_before);
}

TEST_F(HeartbeatMonitor, recovery_fires_up_edge_after_down) {
  BmLinkHeartbeatMonitorCfg cfg = make_cfg();
  ASSERT_EQ(bm_link_heartbeat_monitor_init(&cfg), BmOK);

  auto hb = build_heartbeat_frame();
  ASSERT_EQ(bm_link_heartbeat_monitor_observe(1, hb.data(), hb.size()), BmOK);

  // Force timeout.
  mocked_now_ms += TIMEOUT_MS + 1;
  captured_timer_cb(TIMER_HANDLE);
  ASSERT_EQ(tracker.events.size(), 2u);
  EXPECT_FALSE(tracker.events[1].up);

  // Receive heartbeat -> edge back up.
  ASSERT_EQ(bm_link_heartbeat_monitor_observe(1, hb.data(), hb.size()), BmOK);
  ASSERT_EQ(tracker.events.size(), 3u);
  EXPECT_TRUE(tracker.events[2].up);
}

// ---------------------------------------------------------------------------
// Bootstrap policies
// ---------------------------------------------------------------------------

TEST_F(HeartbeatMonitor, start_ports_up_fires_initial_up_callbacks) {
  BmLinkHeartbeatMonitorCfg cfg = make_cfg(/*num_ports=*/3, /*start_up=*/true);
  ASSERT_EQ(bm_link_heartbeat_monitor_init(&cfg), BmOK);

  // One synchronous up edge per port so the consumer (e.g. bm_l2) can
  // mirror the monitor's "ports up" state.
  ASSERT_EQ(tracker.events.size(), 3u);
  for (uint8_t i = 0; i < 3; i++) {
    EXPECT_EQ(tracker.events[i].port_idx, i);
    EXPECT_TRUE(tracker.events[i].up);
  }
}

TEST_F(HeartbeatMonitor,
       start_ports_up_fires_down_when_no_heartbeat_within_timeout) {
  BmLinkHeartbeatMonitorCfg cfg = make_cfg(/*num_ports=*/2, /*start_up=*/true);
  ASSERT_EQ(bm_link_heartbeat_monitor_init(&cfg), BmOK);

  // Two initial up edges from init, then two down edges from the timer.
  ASSERT_EQ(tracker.events.size(), 2u);

  mocked_now_ms += TIMEOUT_MS + 1;
  captured_timer_cb(TIMER_HANDLE);

  ASSERT_EQ(tracker.events.size(), 4u);
  EXPECT_FALSE(tracker.events[2].up);
  EXPECT_FALSE(tracker.events[3].up);
}

TEST_F(HeartbeatMonitor, start_ports_up_first_heartbeat_does_not_refire_up) {
  BmLinkHeartbeatMonitorCfg cfg = make_cfg(/*num_ports=*/2, /*start_up=*/true);
  ASSERT_EQ(bm_link_heartbeat_monitor_init(&cfg), BmOK);

  // Init fired one up edge per port.
  ASSERT_EQ(tracker.events.size(), 2u);

  auto hb = build_heartbeat_frame();
  EXPECT_EQ(bm_link_heartbeat_monitor_observe(1, hb.data(), hb.size()), BmOK);
  // Already considered up; observe just refreshes last_hb_ticks.
  EXPECT_EQ(tracker.events.size(), 2u);

  // Heartbeat at observe-time refreshed port 1's last_hb_ticks to
  // mocked_now_ms (1000). Advance past the timeout so both ports go down.
  mocked_now_ms += TIMEOUT_MS + 1;
  captured_timer_cb(TIMER_HANDLE);

  ASSERT_EQ(tracker.events.size(), 4u);
  EXPECT_FALSE(tracker.events[2].up);
  EXPECT_FALSE(tracker.events[3].up);
}

TEST_F(HeartbeatMonitor, callback_ctx_is_passed_through) {
  BmLinkHeartbeatMonitorCfg cfg = make_cfg();
  ASSERT_EQ(bm_link_heartbeat_monitor_init(&cfg), BmOK);

  auto hb = build_heartbeat_frame();
  ASSERT_EQ(bm_link_heartbeat_monitor_observe(1, hb.data(), hb.size()), BmOK);

  EXPECT_EQ(tracker.bad_ctx_count, 0u);
}
