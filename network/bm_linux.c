/// @file bm_linux.c
/// @brief Linux native IP stack implementation of bm_ip.h APIs for bm_sbc.

#include "bm_ip.h"
#include "bm_os.h"

#include <stdbool.h>
#include <stddef.h>
#include <string.h>

// ---------------------------------------------------------------------------
// All functions below are stubs.  They satisfy the linker so that bmcore
// builds; real implementations will follow in a later milestone.
// ---------------------------------------------------------------------------

BmErr bm_ip_init(void) { return BmOK; }

void *bm_l2_new(uint32_t size) { (void)size; return NULL; }
void *bm_l2_get_payload(void *buf) { (void)buf; return NULL; }
void bm_l2_tx_prep(void *buf, uint32_t size) { (void)buf; (void)size; }
void bm_l2_free(void *buf) { (void)buf; }
BmErr bm_l2_submit(void *buf, uint32_t size) { (void)buf; (void)size; return BmEINVAL; }
BmErr bm_l2_set_netif(bool up) { (void)up; return BmOK; }

const char *bm_ip_get_str(uint8_t idx) { (void)idx; return "::"; }
const BmIpAddr *bm_ip_get(uint8_t idx) { (void)idx; return NULL; }

void bm_ip_rx_cleanup(void *payload) { (void)payload; }
void *bm_ip_tx_new(const BmIpAddr *dst, uint32_t size) { (void)dst; (void)size; return NULL; }
BmErr bm_ip_tx_copy(void *payload, const void *data, uint32_t size,
                    uint32_t offset) {
  (void)payload; (void)data; (void)size; (void)offset;
  return BmEINVAL;
}
BmErr bm_ip_tx_perform(void *payload, const BmIpAddr *dst) {
  (void)payload; (void)dst;
  return BmEINVAL;
}
void bm_ip_tx_cleanup(void *payload) { (void)payload; }

void *bm_udp_bind_port(const BmIpAddr *addr, uint16_t port,
                       BmErr (*cb)(uint16_t, void *, uint64_t, uint32_t)) {
  (void)addr; (void)port; (void)cb;
  return NULL;
}
void *bm_udp_new(uint32_t size) { (void)size; return NULL; }
void *bm_udp_get_payload(void *buf) { (void)buf; return NULL; }
BmErr bm_udp_reference_update(void *buf) { (void)buf; return BmEINVAL; }
void bm_udp_cleanup(void *buf) { (void)buf; }
BmErr bm_udp_tx_perform(void *pcb, void *buf, uint32_t size,
                        const BmIpAddr *addr, uint16_t port) {
  (void)pcb; (void)buf; (void)size; (void)addr; (void)port;
  return BmEINVAL;
}
void bm_ip_buf_shrink(void *buf, uint32_t size) { (void)buf; (void)size; }

