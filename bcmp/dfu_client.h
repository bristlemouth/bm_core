#include "dfu.h"

#define bm_dfu_client_chunk_timeout_ms 2000UL

void bm_dfu_client_process_update_request(void);

/* HFSM functions */
BmErr s_client_receiving_entry(void);
BmErr s_client_receiving_run(void);
BmErr s_client_validating_entry(void);
BmErr s_client_validating_run(void);
BmErr s_client_activating_entry(void);
BmErr s_client_activating_run(void);
BmErr s_client_reboot_req_entry(void);
BmErr s_client_reboot_req_run(void);
BmErr s_client_update_done_entry(void);
BmErr s_client_update_done_run(void);

void bm_dfu_client_init(void);
bool bm_dfu_client_host_node_valid(uint64_t host_node_id);
