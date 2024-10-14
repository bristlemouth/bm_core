#include "dfu.h"
#include <inttypes.h>

#define bm_dfu_max_ack_retries 2
#define bm_dfu_host_ack_timeout_ms 10000UL
#define bm_dfu_host_heartbeat_timeout_ms 1000UL
#define bm_dfu_update_default_timeout_ms (5 * 60 * 1000)

typedef int (*bm_dfu_chunk_req_cb)(uint16_t chunk_num, uint16_t *chunk_len, uint8_t *buf,
                                   uint16_t buf_len);

void s_host_req_update_entry(void);
void s_host_req_update_run(void);
void s_host_update_entry(void);
void s_host_update_run(void);

void bm_dfu_host_init(BcmpDfuTxFunc bcmp_dfu_tx);
void bm_dfu_host_set_params(UpdateFinishCb update_complete_callback,
                            uint32_t host_timeout_ms);
bool bm_dfu_host_client_node_valid(uint64_t client_node_id);
