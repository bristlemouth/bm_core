#include "network_device.h"
#include "util.h"
#include <stdbool.h>
#include <stdint.h>
#include <string.h>

#ifdef __cplusplus
extern "C" {
#endif

#ifndef bm_l2_tx_task_priority
#define bm_l2_tx_task_priority 7
#endif

/* We store the egress port for the RX device and the ingress port of the TX device
   in bytes 5 (index 4) and 6 (index 5) of the SRC IPv6 address. */
#define egress_port_idx 4
#define ingress_port_idx 5

typedef void (*L2LinkChangeCb)(uint8_t port, bool state);

BmErr bm_l2_link_output(void *buf, uint32_t length);
void bm_l2_deinit(void);
BmErr bm_l2_init(NetworkDevice network_device);
BmErr bm_l2_register_link_change_callback(L2LinkChangeCb cb);
bool bm_l2_get_port_state(uint8_t port);
BmErr bm_l2_netif_set_power(bool on);

#ifdef __cplusplus
}
#endif
