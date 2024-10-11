#include "dfu.h"

// TODO - remove these when we can if we can
#ifdef __cplusplus
extern "C"
{
#endif

#define BM_DFU_CLIENT_CHUNK_TIMEOUT_MS  2000UL

void bm_dfu_client_process_update_request(void);

/* HFSM functions */
void s_client_receiving_entry(void);
void s_client_receiving_run(void);
void s_client_validating_entry(void);
void s_client_validating_run(void);
void s_client_activating_entry(void);
void s_client_activating_run(void);
void s_client_reboot_req_entry(void);
void s_client_reboot_req_run(void);
void s_client_update_done_entry(void);
void s_client_update_done_run(void);

void bm_dfu_client_init(bcmp_dfu_tx_func_t bcmp_dfu_tx);
bool bm_dfu_client_host_node_valid(uint64_t host_node_id);

#ifdef __cplusplus
}
#endif
