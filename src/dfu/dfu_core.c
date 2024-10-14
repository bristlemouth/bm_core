#include <string.h>
#include <stdio.h>
#include "bm_os.h"
#include "dfu.h"
#include "dfu_client.h"
#include "dfu_host.h"
#include "bm_dfu_generic.h"
#include "messages.h"
#include "packet.h"
#include "device.h"

typedef struct dfu_core_ctx_t {
    LibSmContext sm_ctx;
    BmDfuEvent current_event;
    bool pending_state_change;
    uint8_t new_state;
    BmDfuErr error;
    uint64_t self_node_id;
    uint64_t client_node_id;
    BcmpDfuTxFunc bcmp_dfu_tx;
    UpdateFinishCb update_finish_callback;
} dfu_core_ctx_t;

#ifndef CI_TEST
ReboootClientUpdateInfo client_update_reboot_info __attribute__((section(".noinit")));
#else // CI_TEST
ReboootClientUpdateInfo client_update_reboot_info;
#endif // CI_TEST

static dfu_core_ctx_t dfu_ctx;

static BmQueue dfu_event_queue;

static void bm_dfu_send_nop_event(void);
static const LibSmState* bm_dfu_check_transitions(uint8_t current_state);

static void s_init_run(void);
static void s_idle_entry(void);
static void s_idle_exit(void);
static void s_idle_run(void);
static void s_error_run(void) {}
static void s_error_entry(void);

static const LibSmState dfu_states[BmNumDfuStates] = {
    {
        .state_enum = BmDfuStateInit,
        .state_name = "Init", // The name MUST NOT BE NULL
        .run = s_init_run, // This function MUST NOT BE NULL
        .on_state_exit = NULL, // This function can be NULL
        .on_state_entry = NULL, // This function can be NULL
    },
    {
        .state_enum = BmDfuStateIdle,
        .state_name = "Idle",
        .run = s_idle_run,
        .on_state_exit = s_idle_exit,
        .on_state_entry = s_idle_entry,
    },
    {
        .state_enum = BmDfuStateError,
        .state_name = "Error",
        .run = s_error_run,
        .on_state_exit = NULL,
        .on_state_entry = s_error_entry,
    },
    {
        .state_enum = BmDfuStateClientReceiving,
        .state_name = "Client Rx",
        .run = s_client_receiving_run,
        .on_state_exit = NULL,
        .on_state_entry = s_client_receiving_entry,
    },
    {
        .state_enum = BmDfuStateClientValidating,
        .state_name = "Client Validating",
        .run = s_client_validating_run,
        .on_state_exit = NULL,
        .on_state_entry = s_client_validating_entry,
    },
    {
        .state_enum = BmDfuStateClientRebootReq,
        .state_name = "Client Reboot Request",
        .run = s_client_reboot_req_run,
        .on_state_exit = NULL,
        .on_state_entry = s_client_reboot_req_entry,
    },
    {
        .state_enum = BmDfuStateClientRebootDone,
        .state_name = "Client Reboot Done",
        .run = s_client_update_done_run,
        .on_state_exit = NULL,
        .on_state_entry = s_client_update_done_entry,
    },
    {
        .state_enum = BmDfuStateClientActivating,
        .state_name = "Client Activating",
        .run = s_client_activating_run,
        .on_state_exit = NULL,
        .on_state_entry = s_client_activating_entry,
    },
    {
        .state_enum = BmDfuStateHostReqUpdate,
        .state_name = "Host Reqeust Update",
        .run = s_host_req_update_run,
        .on_state_exit = NULL,
        .on_state_entry = s_host_req_update_entry,
    },
    {
        .state_enum = BmDfuStateHostUpdate,
        .state_name = "Host Update",
        .run = s_host_update_run,
        .on_state_exit = NULL,
        .on_state_entry = s_host_update_entry,
    },
};

static void bm_dfu_send_nop_event(void) {
    /* Force running the current state by sending a NOP event */
    BmDfuEvent evt = {DfuEventNone, NULL, 0};
    if(bm_queue_send(dfu_event_queue, &evt, 0) != BmOK) {
        printf("Message could not be added to Queue\n");
    }
}

static const LibSmState* bm_dfu_check_transitions(uint8_t current_state){
    if (dfu_ctx.pending_state_change) {
        dfu_ctx.pending_state_change = false;
        return &dfu_states[dfu_ctx.new_state];
    }
    return &dfu_states[current_state];
}

static void s_init_run(void) {
    if (dfu_ctx.current_event.type == DfuEventInitSuccess) {
        if (client_update_reboot_info.magic == DFU_REBOOT_MAGIC) {
            bm_dfu_set_pending_state_change(BmDfuStateClientRebootDone);
        } else {
            bm_dfu_set_pending_state_change(BmDfuStateIdle);
        }
    }
}

static void s_idle_entry(void) {
    bm_dfu_core_lpm_peripheral_inactive();
    memset(&client_update_reboot_info, 0, sizeof(client_update_reboot_info));
}

static void s_idle_run(void) {
    if (dfu_ctx.current_event.type == DfuEventReceivedUpdateRequest) {
        /* Client */
        bm_dfu_client_process_update_request();
    } else if(dfu_ctx.current_event.type == DfuEventBeginHost) {
        /* Host */
        DfuHostStartEvent *start_event = (DfuHostStartEvent*)(dfu_ctx.current_event.buf);
        dfu_ctx.update_finish_callback = start_event->finish_cb;
        dfu_ctx.client_node_id = start_event->start.info.addresses.dst_node_id;
        bm_dfu_host_set_params(dfu_ctx.update_finish_callback, start_event->timeoutMs);
        bm_dfu_set_pending_state_change(BmDfuStateHostReqUpdate);
    }
}

static void s_idle_exit(void) {
    bm_dfu_core_lpm_peripheral_active();
}

/**
 * @brief Entry Function for the Error State
 *
 * @note The Host processes the current error state and either proceeds to the IDLE state or stays in Error (fatal)
 *
 * @return none
 */
static void s_error_entry(void) {
    bm_dfu_core_lpm_peripheral_inactive();
    switch (dfu_ctx.error) {
        case BmDfuErrFlashAccess:
            printf("Flash access error (Fatal Error)\n");
            break;
        case BmDfuErrImgChunkAccess:
            printf("Unable to get image chunk\n");
            break;
        case BmDfuErrTooLarge:
            printf("Image too large for Client\n");
            break;
        case BmDfuErrSameVer:
            printf("Client already loaded with image\n");
            break;
        case BmDfuErrMismatchLen:
            printf("Length mismatch\n");
            break;
        case BmDfuErrBadCrc:
            printf("CRC mismatch\n");
            break;
        case BmDfuErrTimeout:
            printf("DFU Timeout\n");
            break;
        case BmDfuErrBmFrame:
            printf("BM Processing Error\n");
            break;
        case BmDfuErrAborted:
            printf("BM Aborted Error\n");
            break;
        case BmDfuErrWrongVer:
            printf("Client booted with the wrong version.\n");
            break;
        case BmDfuErrInProgress:
            printf("A FW update is already in progress.\n");
            break;
        case BmDfuErrConfirmationAbort:
            printf("BM Aborted Error During Reboot Confirmation\n");
            break;
        case BmDfuErrNone:
        default:
            break;
    }

    if(dfu_ctx.update_finish_callback) {
        dfu_ctx.update_finish_callback(false, dfu_ctx.error, dfu_ctx.client_node_id);
    }

    if(dfu_ctx.error <  BmDfuErrFlashAccess) {
        bm_dfu_set_pending_state_change(BmDfuStateIdle);
    }
    return;
}


/* Consumes any incoming packets from the BCMP service. The payload types are translated
   into events that are placed into the subystem queue and are consumed by the DFU event thread. */
void bm_dfu_process_message(uint8_t *buf, size_t len) {
    // configASSERT(buf);
    BmDfuEvent evt;
    BmDfuFrame *frame = (BmDfuFrame *)(buf);

    /* If this node is not the intended destination, then discard and continue to wait on queue */
    if (dfu_ctx.self_node_id != ((BmDfuEventAddress *)(frame->payload))->dst_node_id) {
        bm_free(buf);
        return;
    }

    bool valid_packet = true;
    switch(get_current_state_enum(&(dfu_ctx.sm_ctx))){
        case BmDfuStateInit:
        case BmDfuStateIdle:
        case BmDfuStateError: {
            break;
        }
        case BmDfuStateClientReceiving:
        case BmDfuStateClientValidating:
        case BmDfuStateClientRebootReq:
        case BmDfuStateClientRebootDone:
        case BmDfuStateClientActivating: {
            if(!bm_dfu_client_host_node_valid(((BmDfuEventAddress *)(frame->payload))->src_node_id)) {
                valid_packet = false;; // DFU packet from the wrong host! Drop packet.
            }
            break;
        }
        case BmDfuStateHostReqUpdate:
        case BmDfuStateHostUpdate: {
            if(!bm_dfu_host_client_node_valid(((BmDfuEventAddress *)(frame->payload))->src_node_id)){
                valid_packet = false; // DFU packet from the wrong client! Drop packet.
            }
            break;
        }
        default:
            // configASSERT(false);
            break;
    }

    if(!valid_packet) {
        bm_free(buf);
        return;
    }

    evt.buf = buf;
    evt.len = len;

    switch (frame->header.frame_type) {
        case BcmpDFUStartMessage:
            evt.type = DfuEventReceivedUpdateRequest;
            printf("Received update request\n");
            if(bm_queue_send(dfu_event_queue, &evt, 0) != BmOK) {
                bm_free(buf);
                printf("Message could not be added to Queue\n");
            }
            break;
        case BcmpDFUPayloadMessage:
            evt.type = DfuEventImageChunk;
            printf("Received Payload\n");
            if(bm_queue_send(dfu_event_queue, &evt, 0) != BmOK) {
                bm_free(buf);
                printf("Message could not be added to Queue\n");
            }
            break;
        case BcmpDFUEndMessage:
            evt.type = DfuEventUpdateEnd;
            printf("Received DFU End\n");
            if(bm_queue_send(dfu_event_queue, &evt, 0) != BmOK) {
                bm_free(buf);
                printf("Message could not be added to Queue\n");
            }
            break;
        case BcmpDFUAckMessage:
            evt.type = DfuEventAckReceived;
            printf("Received ACK\n");
            if(bm_queue_send(dfu_event_queue, &evt, 0) != BmOK) {
                bm_free(buf);
                printf("Message could not be added to Queue\n");
            }
            break;
        case BcmpDFUAbortMessage:
            evt.type = DfuEventAbort;
            printf("Received Abort\n");
            if(bm_queue_send(dfu_event_queue, &evt, 0) != BmOK) {
                bm_free(buf);
                printf("Message could not be added to Queue\n");
            }
            break;
        case BcmpDFUHeartbeatMessage:
            evt.type = DfuEventHeartbeat;
            printf("Received DFU Heartbeat\n");
            if(bm_queue_send(dfu_event_queue, &evt, 0) != BmOK) {
                bm_free(buf);
                printf("Message could not be added to Queue\n");
            }
            break;
        case BcmpDFUPayloadReqMessage:
            evt.type = DfuEventChunkRequest;
            if(bm_queue_send(dfu_event_queue, &evt, 0) != BmOK) {
                bm_free(buf);
                printf("Message could not be added to Queue\n");
            }
            break;
        case BcmpDFURebootReqMessage:
            evt.type = DfuEventRebootRequest;
            if(bm_queue_send(dfu_event_queue, &evt, 0) != BmOK) {
                bm_free(buf);
                printf("Message could not be added to Queue\n");
            }
            break;
        case BcmpDFURebootMessage:
            evt.type = DfuEventReboot;
            if(bm_queue_send(dfu_event_queue, &evt, 0) != BmOK) {
                bm_free(buf);
                printf("Message could not be added to Queue\n");
            }
            break;
        case BcmpDFUBootCompleteMessage:
            evt.type = DfuEventBootComplete;
            if(bm_queue_send(dfu_event_queue, &evt, 0) != BmOK) {
                bm_free(buf);
                printf("Message could not be added to Queue\n");
            }
            break;
        default:
            // configASSERT(false);
            break;
        }
}

/**
 * @brief Get DFU Subsystem Event Queue
 *
 * @note Used by DFU host and client contexts to put events into the Subsystem Queue
 *
 * @param none
 * @return BmQueue to DFU Subsystem event Queue
 */
BmQueue bm_dfu_get_event_queue(void) {
    return dfu_event_queue;
}

/**
 * @brief Get latest DFU event
 *
 * @note Get the event currently stored in the DFU Core context
 *
 * @param none
 * @return BmDfuEvent Latest DFU Event enum
 */
BmDfuEvent bm_dfu_get_current_event(void) {
    return dfu_ctx.current_event;
}

/**
 * @brief Set DFU Core Error
 *
 * @note Set the error of the DFU context which will be used by the Error State logic
 *
 * @param error  Specific DFU Error value
 * @return none
 */
void bm_dfu_set_error(BmDfuErr error) {
    dfu_ctx.error = error;
}

void bm_dfu_set_pending_state_change(uint8_t new_state) {
    dfu_ctx.pending_state_change = 1;
    dfu_ctx.new_state = new_state;
    printf("Transitioning to state: %s\n", dfu_states[new_state].state_name);
    bm_dfu_send_nop_event();
}

/**
 * @brief Send ACK/NACK to Host
 *
 * @note Stuff ACK bm_frame with success and err_code and put into BM Serial TX Queue
 *
 * @param success       1 for ACK, 0 for NACK
 * @param err_code      Error Code enum, read by Host on NACK
 * @return none
 */
void bm_dfu_send_ack(uint64_t dst_node_id, uint8_t success, BmDfuErr err_code) {
    BcmpDfuAck ack_msg;

    /* Stuff ACK Event */
    ack_msg.ack.success = success;
    ack_msg.ack.err_code = err_code;
    ack_msg.ack.addresses.dst_node_id = dst_node_id;
    ack_msg.ack.addresses.src_node_id = dfu_ctx.self_node_id;
    ack_msg.header.frame_type = BcmpDFUAckMessage;

    if(dfu_ctx.bcmp_dfu_tx((BcmpMessageType)(ack_msg.header.frame_type), (uint8_t*)(&ack_msg), sizeof(ack_msg))){
        printf("Message %d sent \n",ack_msg.header.frame_type);
    } else {
        printf("Failed to send message %d\n",ack_msg.header.frame_type);
    }
}

/**
 * @brief Send Chunk Request
 *
 * @note Stuff Chunk Request bm_frame with chunk number and put into BM Serial TX Queue
 *
 * @param chunk_num     Image Chunk number requested
 * @return none
 */
void bm_dfu_req_next_chunk(uint64_t dst_node_id, uint16_t chunk_num)
{
    BcmpDfuPayloadReq chunk_req_msg;

    /* Stuff Chunk Request Event */
    chunk_req_msg.chunk_req.seq_num = chunk_num;
    chunk_req_msg.chunk_req.addresses.src_node_id = dfu_ctx.self_node_id;
    chunk_req_msg.chunk_req.addresses.dst_node_id = dst_node_id;
    chunk_req_msg.header.frame_type = BcmpDFUPayloadReqMessage;

    if(dfu_ctx.bcmp_dfu_tx((BcmpMessageType)(chunk_req_msg.header.frame_type), (uint8_t*)(&chunk_req_msg), sizeof(chunk_req_msg))){
        printf("Message %d sent \n", chunk_req_msg.header.frame_type);
    } else {
        printf("Failed to send message %d\n", chunk_req_msg.header.frame_type);
    }
}

/**
 * @brief Send DFU END
 *
 * @note Stuff DFU END bm_frame with success and err_code and put into BM Serial TX Queue
 *
 * @param success       1 for Successful Update, 0 for Unsuccessful
 * @param err_code      Error Code enum, read by Host on Unsuccessful update
 * @return none
 */
void bm_dfu_update_end(uint64_t dst_node_id, uint8_t success, BmDfuErr err_code) {
    BcmpDfuEnd update_end_msg;

    /* Stuff Update End Event */
    update_end_msg.result.success = success;
    update_end_msg.result.err_code = err_code;
    update_end_msg.result.addresses.dst_node_id = dst_node_id;
    update_end_msg.result.addresses.src_node_id = dfu_ctx.self_node_id;
    update_end_msg.header.frame_type = BcmpDFUEndMessage;

    if(dfu_ctx.bcmp_dfu_tx((BcmpMessageType)(update_end_msg.header.frame_type), (uint8_t*)(&update_end_msg), sizeof(update_end_msg))){
        printf("Message %d sent \n",update_end_msg.header.frame_type);
    } else {
        printf("Failed to send message %d\n",update_end_msg.header.frame_type);
    }
}

/**
 * @brief Send Heartbeat to other device
 *
 * @note Put DFU Heartbeat BM serial frame into BM Serial TX Queue
 *
 * @return none
 */
void bm_dfu_send_heartbeat(uint64_t dst_node_id) {
    BcmpDfuHeartbeat heartbeat_msg;
    heartbeat_msg.addr.dst_node_id = dst_node_id;
    heartbeat_msg.addr.src_node_id = dfu_ctx.self_node_id;
    heartbeat_msg.header.frame_type = BcmpDFUHeartbeatMessage;

    if(dfu_ctx.bcmp_dfu_tx((BcmpMessageType)(heartbeat_msg.header.frame_type), (uint8_t*)(&heartbeat_msg), sizeof(heartbeat_msg))){
        printf("Message %d sent \n",heartbeat_msg.header.frame_type);
    } else {
        printf("Failed to send message %d\n",heartbeat_msg.header.frame_type);
    }
}

/* This thread consumes events from the event queue and progresses the state machine */
static void bm_dfu_event_thread(void*) {
    printf("BM DFU Subsystem thread started\n");

    while (1) {
        dfu_ctx.current_event.type = DfuEventNone;
        if(bm_queue_receive(dfu_event_queue, &dfu_ctx.current_event, BM_MAX_DELAY_UINT32) == BmOK) {
            lib_sm_run(&(dfu_ctx.sm_ctx));
        }
        if (dfu_ctx.current_event.buf) {
            bm_free(dfu_ctx.current_event.buf);
        }
    }
}

///*!
//  Process a DFU message. Allocates memory that the consumer is in charge of freeing.
//  \param pbuf[in] pbuf buffer
//  \return none
//*/
static BmErr dfu_copy_and_process_message(BcmpProcessData data) {
  BmErr err = BmEINVAL;
  uint8_t *buf = (uint8_t *)(bm_malloc((data.size)));
  if (buf) {
    memcpy(buf, data.payload, (data.size));
    bm_dfu_process_message(buf, (data.size));
    err = BmOK;
  }
  return err;
}

void bm_dfu_init(BcmpDfuTxFunc bcmp_dfu_tx) {
    // configASSERT(bcmp_dfu_tx);
    dfu_ctx.bcmp_dfu_tx = bcmp_dfu_tx;
    BmDfuEvent evt;
    BmErr retval;

    /* Store relevant variables from bristlemouth.c */
    dfu_ctx.self_node_id = node_id();

    /* Initialize current event to NULL */
    dfu_ctx.current_event.type = DfuEventNone;
    dfu_ctx.current_event.buf = NULL;
    dfu_ctx.current_event.len = 0;

    /* Set initial state of DFU State Machine*/
    lib_sm_init(&(dfu_ctx.sm_ctx), &(dfu_states[BmDfuStateInit]), bm_dfu_check_transitions);

    dfu_event_queue = bm_queue_create( 5, sizeof(BmDfuEvent));
    // configASSERT(dfu_event_queue);

    bm_dfu_client_init(bcmp_dfu_tx);
    bm_dfu_host_init(bcmp_dfu_tx);

    evt.type = DfuEventInitSuccess;
    evt.buf = NULL;
    evt.len = 0;

    // TODO - figure out how where it is best to define the priority... in bm_os.h?
    retval = bm_task_create(bm_dfu_event_thread,
                       "DFU Event",
                       1024,
                       NULL,
                       BM_DFU_EVENT_TASK_PRIORITY,
                       NULL);
    if (retval != BmOK) {
        // TODO - handle this better
        printf("Failed to create DFU Event thread\n");
    }
    // configASSERT(retval == BmOK);

    if(bm_queue_send(dfu_event_queue, &evt, 0) != BmOK) {
        printf("Message could not be added to Queue\n");
    }

    BcmpPacketCfg process_dfu_message = {
      false,
      false,
      dfu_copy_and_process_message,
  };
  BmErr err = BmOK;
  bm_err_check(err, packet_add(&process_dfu_message, BcmpDFUStartMessage));
  bm_err_check(err, packet_add(&process_dfu_message, BcmpDFUPayloadReqMessage));
  bm_err_check(err, packet_add(&process_dfu_message, BcmpDFUPayloadMessage));
  bm_err_check(err, packet_add(&process_dfu_message, BcmpDFUEndMessage));
  bm_err_check(err, packet_add(&process_dfu_message, BcmpDFUAckMessage));
  bm_err_check(err, packet_add(&process_dfu_message, BcmpDFUAbortMessage));
  bm_err_check(err, packet_add(&process_dfu_message, BcmpDFUHeartbeatMessage));
  bm_err_check(err, packet_add(&process_dfu_message, BcmpDFURebootReqMessage));
  bm_err_check(err, packet_add(&process_dfu_message, BcmpDFURebootMessage));
  bm_err_check(err, packet_add(&process_dfu_message, BcmpDFUBootCompleteMessage));
  bm_err_check(err, packet_add(&process_dfu_message, BcmpDFULastMessageMessage));
}

bool bm_dfu_initiate_update(BmDfuImgInfo info, uint64_t dest_node_id, UpdateFinishCb update_finish_callback, uint32_t timeoutMs) {
    bool ret = false;
    do {
        if(info.chunk_size > bm_dfu_max_chunk_size) {
            printf("Invalid chunk size for DFU\n");
            break;
        }
        if(get_current_state_enum(&(dfu_ctx.sm_ctx)) != BmDfuStateIdle) {
            printf("Not ready to start update.\n");
            if(update_finish_callback) {
                update_finish_callback(false, BmDfuErrInProgress, dest_node_id);
            }
            break;
        }
        BmDfuEvent evt;
        size_t size = sizeof(DfuHostStartEvent);
        evt.type = DfuEventBeginHost;
        uint8_t *buf = (uint8_t*)(bm_malloc(size));
        // configASSERT(buf);

        DfuHostStartEvent *start_event = (DfuHostStartEvent*)(buf);
        start_event->start.header.frame_type = BcmpDFUStartMessage;
        start_event->start.info.addresses.dst_node_id = dest_node_id;
        start_event->start.info.addresses.src_node_id = dfu_ctx.self_node_id;
        memcpy(&start_event->start.info.img_info, &info, sizeof(BmDfuImgInfo));
        start_event->finish_cb = update_finish_callback;
        start_event->timeoutMs = timeoutMs;
        evt.buf = buf;
        evt.len = size;
        if(bm_queue_send(dfu_event_queue, &evt, 0) != BmOK) {
            bm_free(buf);
            if(update_finish_callback) {
                update_finish_callback(false, BmDfuErrInProgress, dest_node_id);
            }
            printf("Message could not be added to Queue\n");
            break;
        }
        ret = true;
    } while(0);
    return ret;
}

BmDfuErr bm_dfu_get_error(void) {
    return dfu_ctx.error;
}

/*!
 * UNIT TEST FUNCTIONS BELOW HERE
 */
#ifdef CI_TEST
LibSmContext* bm_dfu_test_get_sm_ctx(void) {
    return &dfu_ctx.sm_ctx;
}

void bm_dfu_test_set_dfu_event_and_run_sm(BmDfuEvent evt) {
    memcpy(&dfu_ctx.current_event, &evt, sizeof(BmDfuEvent));
    lib_sm_run(dfu_ctx.sm_ctx);
}
#endif //CI_TEST
