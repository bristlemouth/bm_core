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

typedef void (*L2LinkChangeCb)(uint8_t port, bool state);

BmErr bm_l2_handle_device_interrupt(void);
BmErr bm_l2_link_output(void *buf, uint32_t length);
void bm_l2_deinit(void);
BmErr bm_l2_init(NetworkDevice network_device);
BmErr bm_l2_register_link_change_callback(L2LinkChangeCb cb);
bool bm_l2_get_port_state(uint8_t port);
BmErr bm_l2_netif_set_power(bool on);

#ifdef __cplusplus
}
#endif
