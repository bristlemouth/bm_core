#include <stdio.h>
#include <string.h>
#include "bm_os.h"
#include "dfu.h"
#include "bm_dfu_generic.h"
#include "dfu_host.h"
#include "timer_callback_handler.h"
#include "device.h"

typedef struct dfu_host_ctx_t {
    BmQueue dfu_event_queue;
    BmTimer ack_timer;
    uint8_t ack_retry_num;
    BmTimer heartbeat_timer;
    BmDfuImgInfo img_info;
    uint64_t self_node_id;
    uint64_t client_node_id;
    BcmpDfuTxFunc bcmp_dfu_tx;
    uint32_t bytes_remaining;
    UpdateFinishCb update_complete_callback;
    BmTimer update_timer;
    uint32_t host_timeout_ms;
} dfu_host_ctx_t;

#define flash_read_timeout_ms 5 * 1000

static dfu_host_ctx_t host_ctx;

static void ack_timer_handler(BmTimer tmr);
static void heartbeat_timer_handler(BmTimer tmr);
static void update_timer_handler(BmTimer tmr);

static void bm_dfu_host_req_update();
static void bm_dfu_host_send_reboot();
static void bm_dfu_host_transition_to_error(BmDfuErr err);
static void bm_dfu_host_start_update_timer(uint32_t timeoutMs);

/**
 * @brief ACK Timer Handler function
 *
 * @note Puts ACK Timeout event into DFU Subsystem event queue
 *
 * @param *tmr    Pointer to Timer struct
 * @return none
 */
static void ack_timer_handler(BmTimer tmr) {
    (void) tmr;
    BmDfuEvent evt = {DfuEventAckTimeout, NULL,0};

    if(bm_queue_send(host_ctx.dfu_event_queue, &evt, 0) != BmOK) {
        // configASSERT(false);
        // TODO - handle this better?
        printf("Failed to send ACK timeout event\n");
    }
}

/**
 * @brief  hook to send a DFU heartbeat out on the timer_callback_handler thread
 *
 * @note Sends a heartbeat to the client.
 *
 * @param *arg
 * @return none
 */
static void send_hb_timer_cb(void *arg) {
    (void) arg;
    bm_dfu_send_heartbeat(host_ctx.client_node_id);
}

/**
 * @brief Heartbeat Timer Handler function
 *
 * @note Puts Heartbeat event onto the timer_callback_handler thread
 *
 * @param *tmr    Pointer to Timer struct
 * @return none
 */
static void heartbeat_timer_handler(BmTimer tmr) {
    (void) tmr;
    timer_callback_handler_send_cb(send_hb_timer_cb,NULL,0);
}

/**
 * @brief Update Timer Handler Function
 *
 * @note Aborts DFU if timer fires.
 *
 * @return none
 */
static void update_timer_handler(BmTimer tmr) {
    (void) tmr;
    BmDfuEvent evt = {DfuEventAbort, NULL,0};

    if(bm_queue_send(host_ctx.dfu_event_queue, &evt, 0) != BmOK) {
        // configASSERT(false);
    }
}

/**
 * @brief Send Request Update to Client
 *
 * @note Stuff Update Request bm_frame with image info and put into BM Serial TX Queue
 *
 * @return none
 */
static void bm_dfu_host_req_update() {
    BcmpDfuStart update_start_req_evt;

    printf("Sending Update to Client\n");

    /* Populate the appropriate event */
    update_start_req_evt.info.img_info = host_ctx.img_info;
    update_start_req_evt.info.addresses.src_node_id = host_ctx.self_node_id;
    update_start_req_evt.info.addresses.dst_node_id = host_ctx.client_node_id;
    update_start_req_evt.header.frame_type = BcmpDFUStartMessage;
    if(host_ctx.bcmp_dfu_tx((BcmpMessageType)(update_start_req_evt.header.frame_type), (uint8_t *)(&update_start_req_evt), sizeof(update_start_req_evt))){
        printf("Message %d sent \n",update_start_req_evt.header.frame_type);
    } else {
        printf("Failed to send message %d\n",update_start_req_evt.header.frame_type);
    }
}

/**
 * @brief Send Chunk to Client
 *
 * @note Stuff bm_frame with image chunk and put into BM Serial TX Queue
 *
 * @return none
 */
static void bm_dfu_host_send_chunk(BmDfuEventChunkRequest* req) {
    printf("Processing chunk id %" PRIX32 "\n",req->seq_num);
    uint32_t payload_len = (host_ctx.bytes_remaining >= host_ctx.img_info.chunk_size) ? host_ctx.img_info.chunk_size : host_ctx.bytes_remaining;
    uint32_t payload_len_plus_header = sizeof(BcmpDfuPayload) + payload_len;
    uint8_t* buf = (uint8_t*)bm_malloc(payload_len_plus_header);
    // configASSERT(buf);
    BcmpDfuPayload *payload_header = (BcmpDfuPayload *)buf;
    payload_header->header.frame_type = BcmpDFUPayloadMessage;
    payload_header->chunk.addresses.src_node_id = host_ctx.self_node_id;
    payload_header->chunk.addresses.dst_node_id = host_ctx.client_node_id;
    payload_header->chunk.payload_length = payload_len;

    uint32_t flash_offset = DFU_IMG_START_OFFSET_BYTES + host_ctx.img_info.image_size - host_ctx.bytes_remaining;
    do {
        if (bm_dfu_host_get_chunk(flash_offset, payload_header->chunk.payload_buf, payload_len, flash_read_timeout_ms) != BmOK) {
            printf("Failed to read chunk from flash.\n");
            bm_dfu_host_transition_to_error(BmDfuErrFlashAccess);
            break;
        }
        if(host_ctx.bcmp_dfu_tx((BcmpMessageType)(payload_header->header.frame_type), buf, payload_len_plus_header)){
            host_ctx.bytes_remaining -= payload_len;
            printf("Message %d sent, payload size: %" PRIX32 ", remaining: %" PRIX32 "\n",payload_header->header.frame_type, payload_len, host_ctx.bytes_remaining);
        } else {
            printf("Failed to send message %d\n",payload_header->header.frame_type);
            bm_dfu_host_transition_to_error(BmDfuErrImgChunkAccess);
        }
    } while(0);

    bm_free(buf);
}

/**
 * @brief Send an update reboot to Client
 *
 * @return none
 */
static void bm_dfu_host_send_reboot() {
    BcmpDfuReboot reboot_msg;
    reboot_msg.addr.src_node_id = host_ctx.self_node_id;
    reboot_msg.addr.dst_node_id = host_ctx.client_node_id;
    reboot_msg.header.frame_type = BcmpDFURebootMessage;
    if(host_ctx.bcmp_dfu_tx((BcmpMessageType)(reboot_msg.header.frame_type), (uint8_t*)(&reboot_msg), sizeof(BcmpDfuReboot))){
        printf("Message %d sent \n",reboot_msg.header.frame_type);
    } else {
        printf("Failed to send message %d\n",reboot_msg.header.frame_type);
    }
}

/**
 * @brief Entry Function for the Request Update State
 *
 * @note The Host sends an update request to the client and starts the ACK timeout timer
 *
 * @return none
 */
void s_host_req_update_entry(void) {
    BmDfuEvent curr_evt = bm_dfu_get_current_event();

    /* Check if we even have a buf to inspect */
    if (! curr_evt.buf) {
        return;
    }

    BmDfuFrame *frame = (BmDfuFrame *)(curr_evt.buf);
    BmDfuEventImgInfo* img_info_evt = (BmDfuEventImgInfo*)(&((uint8_t *)(frame))[1]);
    host_ctx.img_info = img_info_evt->img_info;
    host_ctx.bytes_remaining = host_ctx.img_info.image_size;
    host_ctx.client_node_id = img_info_evt->addresses.dst_node_id;

    printf("DFU Client Node Id: %016" PRIx64 "\n", host_ctx.client_node_id);

    host_ctx.ack_retry_num = 0;
    /* Request Client Firmware Update */
    bm_dfu_host_req_update();

    /* Kickoff ACK timeout */
    // configASSERT(xTimerStart(host_ctx.ack_timer, 10));
    bm_timer_start(host_ctx.ack_timer, 10);
}

/**
 * @brief Run Function for the Request Update State
 *
 * @note The state is waiting on an ACK from the client to begin the update. Returns to idle state on timeout
 *
 * @return none
 */
void s_host_req_update_run(void)
{
    BmDfuEvent curr_evt = bm_dfu_get_current_event();

    if (curr_evt.type == DfuEventAckReceived) {
        /* Stop ACK Timer */
        // configASSERT(xTimerStop(host_ctx.ack_timer, 10));
        bm_timer_stop(host_ctx.ack_timer, 10);
        // configASSERT(curr_evt.buf);
        BmDfuFrame *frame = (BmDfuFrame *)(curr_evt.buf);
        BmDfuEventResult* result_evt = (BmDfuEventResult*)(&((uint8_t *)(frame))[1]);

        if (result_evt->success) {
            bm_dfu_set_pending_state_change(BmDfuStateHostUpdate);
        } else {
            bm_dfu_host_transition_to_error((BmDfuErr)(result_evt->err_code));
        }
    } else if (curr_evt.type == DfuEventAckTimeout) {
        host_ctx.ack_retry_num++;

        /* Wait for ack until max retries is reached */
        if (host_ctx.ack_retry_num >= bm_dfu_max_ack_retries) {
            bm_dfu_host_transition_to_error(BmDfuErrTimeout);
        } else {
            bm_dfu_host_req_update();
            // configASSERT(xTimerStart(host_ctx.ack_timer, 10));
            bm_timer_start(host_ctx.ack_timer, 10);
        }
    } else if (curr_evt.type == DfuEventAbort) {
        BmDfuErr err = BmDfuErrAborted;
        if (curr_evt.buf)
        {
            BcmpDfuAbort* abort_evt = (BcmpDfuAbort *)(curr_evt.buf);
            err = (BmDfuErr)(abort_evt->err.err_code);
        }
        printf("Recieved abort in request.\n");
        bm_dfu_host_transition_to_error(err);
    }
}

/**
 * @brief Entry Function for the Update State
 *
 * @note The Host starts the host global timeout.
 *
 * @return none
 */
void s_host_update_entry(void) {
    bm_dfu_host_start_update_timer(host_ctx.host_timeout_ms);
}

/**
 * @brief Run Function for the Update State
 *
 * @note Host state that sends chunks of image to Client. Exits on global timeout timeout or abort/end message received from client
 *
 * @return none
 */
void s_host_update_run(void) {
    BmDfuFrame *frame = NULL;
    BmDfuEvent curr_evt = bm_dfu_get_current_event();

    /* Check if we even have a buf to inspect */
    if (curr_evt.buf) {
        frame = (BmDfuFrame *)(curr_evt.buf);
    }

    if (curr_evt.type == DfuEventChunkRequest) {
        // configASSERT(frame);
        BmDfuEventChunkRequest* chunk_req_evt = (BmDfuEventChunkRequest*)(&((uint8_t *)(frame))[1]);

        /* Request Next Chunk */
        /* Send Heartbeat to Client */
        // configASSERT(xTimerStart(host_ctx.heartbeat_timer, 10));
        bm_timer_start(host_ctx.heartbeat_timer, 10);

        /* resend the frame to the client as is */
        bm_dfu_host_send_chunk(chunk_req_evt);

        // configASSERT(xTimerStop(host_ctx.heartbeat_timer, 10));
        bm_timer_stop(host_ctx.heartbeat_timer, 10);
    } else if (curr_evt.type == DfuEventRebootRequest) {
        // configASSERT(frame);
        bm_dfu_host_send_reboot();
    } else if (curr_evt.type == DfuEventBootComplete) {
        // configASSERT(frame);
        bm_dfu_update_end(host_ctx.client_node_id, true, BmDfuErrNone);
    }
    else if (curr_evt.type == DfuEventUpdateEnd) {
        // configASSERT(xTimerStop(host_ctx.update_timer, 100));
        bm_timer_stop(host_ctx.update_timer, 100);
        // configASSERT(frame);
        BmDfuEventResult* update_end_evt = (BmDfuEventResult*)(&((uint8_t *)(frame))[1]);

        if (update_end_evt->success) {
            printf("Successfully updated Client\n");
        } else {
            printf("Client Update Failed\n");
        }
        if(host_ctx.update_complete_callback) {
            host_ctx.update_complete_callback(update_end_evt->success, (BmDfuErr)(update_end_evt->err_code), host_ctx.client_node_id);
        }
        bm_dfu_set_pending_state_change(BmDfuStateIdle);
    } else if (curr_evt.type == DfuEventAbort) {
        BmDfuErr err = BmDfuErrAborted;
        if (curr_evt.buf)
        {
            BcmpDfuAbort* abort_evt = (BcmpDfuAbort *)(curr_evt.buf);
            err = (BmDfuErr)(abort_evt->err.err_code);
        }
        printf("Recieved abort in run.\n");
        bm_dfu_host_transition_to_error(err);
    }
}

void bm_dfu_host_init(BcmpDfuTxFunc bcmp_dfu_tx) {
    // configASSERT(bcmp_dfu_tx);
    host_ctx.bcmp_dfu_tx = bcmp_dfu_tx;
    int tmr_id = 0;

    /* Store relevant variables */
    host_ctx.self_node_id = node_id();

    /* Get DFU Subsystem Queue */
    host_ctx.dfu_event_queue = bm_dfu_get_event_queue();

    /* Initialize ACK and Heartbeat Timer */
    host_ctx.ack_timer = bm_timer_create("DFU Host Ack", bm_ms_to_ticks(bm_dfu_host_ack_timeout_ms),
                                      false, (void *) &tmr_id, ack_timer_handler);
    // configASSERT(host_ctx.ack_timer);

    host_ctx.heartbeat_timer = bm_timer_create("DFU Host Heartbeat", bm_ms_to_ticks(bm_dfu_host_heartbeat_timeout_ms),
                                      true, (void *) &tmr_id, heartbeat_timer_handler);
    // configASSERT(host_ctx.heartbeat_timer);
    host_ctx.update_timer = bm_timer_create("update timer", bm_ms_to_ticks(bm_dfu_update_default_timeout_ms),
                                    false, (void *) &tmr_id, update_timer_handler);
    // configASSERT(host_ctx.update_timer);
}

void bm_dfu_host_set_params(UpdateFinishCb update_complete_callback, uint32_t host_timeout_ms) {
    host_ctx.update_complete_callback = update_complete_callback;
    host_ctx.host_timeout_ms = host_timeout_ms;
}

static void bm_dfu_host_start_update_timer(uint32_t timeoutMs) {
    // configASSERT(xTimerStop(host_ctx.update_timer, 100));
    // configASSERT(xTimerChangePeriod(host_ctx.update_timer, bm_ms_to_ticks(timeoutMs), 100));
    // configASSERT(xTimerStart(host_ctx.update_timer, 100));
    bm_timer_stop(host_ctx.update_timer, 100);
    bm_timer_change_period(host_ctx.update_timer, bm_ms_to_ticks(timeoutMs), 100);
    bm_timer_start(host_ctx.update_timer, 100);
}

static void bm_dfu_host_transition_to_error(BmDfuErr err) {
    // configASSERT(xTimerStop(host_ctx.update_timer, 100));
    // configASSERT(xTimerStop(host_ctx.heartbeat_timer, 10));
    // configASSERT(xTimerStop(host_ctx.ack_timer, 10));
    bm_timer_stop(host_ctx.update_timer, 100);
    bm_timer_stop(host_ctx.heartbeat_timer, 10);
    bm_timer_stop(host_ctx.ack_timer, 10);
    bm_dfu_set_error(err);
    bm_dfu_set_pending_state_change(BmDfuStateError);
}

bool bm_dfu_host_client_node_valid(uint64_t client_node_id) {
    return host_ctx.client_node_id == client_node_id;
}
