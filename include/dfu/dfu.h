#ifndef __BM_DFU_H__
#define __BM_DFU_H__

#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include "bm_os.h"
#include "messages.h"
#include "lib_state_machine.h"


// TODO - remove these when we can if we can
#ifdef __cplusplus
extern "C" {
#endif

#define bm_dfu_max_chunk_size (1024) // TODO: put this in an app config header
#define bm_dfu_max_chunk_retries 5

#define bm_img_page_length 2048

typedef enum {
  BmDfuErrNone,
  BmDfuErrTooLarge,
  BmDfuErrSameVer,
  BmDfuErrMismatchLen,
  BmDfuErrBadCrc,
  BmDfuErrImgChunkAccess,
  BmDfuErrTimeout,
  BmDfuErrBmFrame,
  BmDfuErrAborted,
  BmDfuErrWrongVer,
  BmDfuErrInProgress,
  BmDfuErrChunkSize,
  BmDfuErrUnkownNodeId,
  BmDfuErrConfirmationAbort,
  // All errors below this are "fatal"
  BmDfuErrFlashAccess,
} BmDfuErr;

enum BmDfuHfsmStates {
  BmDfuStateInit,
  BmDfuStateIdle,
  BmDfuStateError,
  BmDfuStateClientReceiving,
  BmDfuStateClientValidating,
  BmDfuStateClientRebootReq,
  BmDfuStateClientRebootDone,
  BmDfuStateClientActivating,
  BmDfuStateHostReqUpdate,
  BmDfuStateHostUpdate,
  BmNumDfuStates,
};

enum BmDfuEvtType {
  DfuEventNone,
  DfuEventInitSuccess,
  DfuEventReceivedUpdateRequest,
  DfuEventChunkRequest,
  DfuEventImageChunk,
  DfuEventUpdateEnd,
  DfuEventAckReceived,
  DfuEventAckTimeout,
  DfuEventChunkTimeout,
  DfuEventHeartbeat,
  DfuEventAbort,
  DfuEventBeginHost,
  DfuEventRebootRequest,
  DfuEventReboot,
  DfuEventBootComplete,
};

typedef bool (*BcmpDfuTxFunc)(BcmpMessageType type, uint8_t *buff, uint16_t len);
typedef void (*UpdateFinishCb)(bool success, BmDfuErr error, uint64_t node_id);

typedef struct {
  uint8_t type;
  uint8_t *buf;
  size_t len;
} BmDfuEvent;

typedef struct __attribute__((__packed__)) DfuHostStartEvent {
  BcmpDfuStart start;
  UpdateFinishCb finish_cb;
  uint32_t timeoutMs;
} DfuHostStartEvent;

#define DFU_REBOOT_MAGIC (0xBADC0FFE)

typedef struct __attribute__((__packed__)) {
  uint32_t magic;
  uint8_t major;
  uint8_t minor;
  uint64_t host_node_id;
  uint32_t gitSHA;
} ReboootClientUpdateInfo;

#ifndef ENABLE_TESTING
extern ReboootClientUpdateInfo client_update_reboot_info __attribute__((section(".noinit")));
#else // ENABLE_TESTING
extern ReboootClientUpdateInfo client_update_reboot_info;
#endif // ENABLE_TESTING

BmQueue bm_dfu_get_event_queue(void);
BmDfuEvent bm_dfu_get_current_event(void);
void bm_dfu_set_error(BmDfuErr error);
BmDfuErr bm_dfu_get_error(void);
void bm_dfu_set_pending_state_change(uint8_t new_state);

void bm_dfu_send_ack(uint64_t dst_node_id, uint8_t success, BmDfuErr err_code);
void bm_dfu_req_next_chunk(uint64_t dst_node_id, uint16_t chunk_num);
void bm_dfu_update_end(uint64_t dst_node_id, uint8_t success, BmDfuErr err_code);
void bm_dfu_send_heartbeat(uint64_t dst_node_id);

void bm_dfu_init(BcmpDfuTxFunc bcmp_dfu_tx);
void bm_dfu_process_message(uint8_t *buf, size_t len);
bool bm_dfu_initiate_update(BmDfuImgInfo info, uint64_t dest_node_id,
                            UpdateFinishCb update_finish_callback, uint32_t timeoutMs);

/*!
 * UNIT TEST FUNCTIONS BELOW HERE
 */
#ifdef ENABLE_TESTING
LibSmContext *bm_dfu_test_get_sm_ctx(void);
void bm_dfu_test_set_dfu_event_and_run_sm(BmDfuEvent evt);
void bm_dfu_test_set_client_fa(void *fa);
#endif //ENABLE_TESTING

#ifdef __cplusplus
}
#endif

#endif // __BM_DFU_H__
