#include "dfu_host.h"
#include "bcmp.h"
#include "bm_config.h"
#include "bm_dfu_generic.h"
#include "bm_os.h"
#include "device.h"
#include "dfu.h"
#include "timer_callback_handler.h"
#include <stdio.h>
#include <string.h>

#define timer_err_check_and_return(e, f)                                       \
  bm_err_report(e, f);                                                         \
  if (e != BmOK) {                                                             \
    bm_dfu_host_transition_to_error(BmDfuErrAborted);                          \
    return err;                                                                \
  }
#define data_queue_size (3)

typedef struct dfu_host_ctx_t {
  BmQueue dfu_event_queue;
  BmTimer ack_timer;
  uint8_t ack_retry_num;
  BmTimer heartbeat_timer;
  BmDfuImgInfo img_info;
  uint64_t self_node_id;
  uint64_t client_node_id;
  uint32_t bytes_remaining;
  UpdateFinishCb update_complete_callback;
  BmTimer update_timer;
  uint32_t host_timeout_ms;
  BmQueue data_queue;
} dfu_host_ctx_t;

#define flash_read_timeout_ms 5 * 1000

static dfu_host_ctx_t host_ctx;

static void ack_timer_handler(BmTimer tmr);
static void heartbeat_timer_handler(BmTimer tmr);
static void update_timer_handler(BmTimer tmr);

static void bm_dfu_host_req_update(void);
static void bm_dfu_host_send_reboot(void);
static void bm_dfu_host_transition_to_error(BmDfuErr dfu_err);
static BmErr bm_dfu_host_start_update_timer(uint32_t timeoutMs);

/**
 * @brief ACK Timer Handler function
 *
 * @note Puts ACK Timeout event into DFU Subsystem event queue
 *
 * @param *tmr    Pointer to Timer struct
 * @return none
 */
static void ack_timer_handler(BmTimer tmr) {
  (void)tmr;
  BmDfuEvent evt = {DfuEventAckTimeout, NULL, 0};

  if (bm_queue_send(host_ctx.dfu_event_queue, &evt, 0) != BmOK) {
    bm_debug("Failed to queue ACK timeout event...\n");
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
  (void)arg;
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
  (void)tmr;
  timer_callback_handler_send_cb(send_hb_timer_cb, NULL, 0);
}

/**
 * @brief Update Timer Handler Function
 *
 * @note Aborts DFU if timer fires.
 *
 * @return none
 */
static void update_timer_handler(BmTimer tmr) {
  (void)tmr;
  BmDfuEvent evt = {DfuEventAbort, NULL, 0};

  if (bm_queue_send(host_ctx.dfu_event_queue, &evt, 0) != BmOK) {
    bm_debug("Failed to queue DFU abort event...\n");
  }
}

/**
 * @brief Send Request Update to Client
 *
 * @note Stuff Update Request bm_frame with image info and put into BM Serial TX Queue
 *
 * @return none
 */
static void bm_dfu_host_req_update(void) {
  BcmpDfuStart update_start_req_evt;

  bm_debug("Sending Update to Client\n");

  /* Populate the appropriate event */
  update_start_req_evt.info.img_info = host_ctx.img_info;
  update_start_req_evt.info.addresses.src_node_id = host_ctx.self_node_id;
  update_start_req_evt.info.addresses.dst_node_id = host_ctx.client_node_id;
  update_start_req_evt.header.frame_type = BcmpDFUStartMessage;
  BmErr err = bcmp_tx(&multicast_global_addr,
                      (BcmpMessageType)(update_start_req_evt.header.frame_type),
                      (uint8_t *)(&update_start_req_evt),
                      sizeof(update_start_req_evt), 0, NULL);
  if (err == BmOK) {
    bm_debug("Message %d sent \n", update_start_req_evt.header.frame_type);
  } else {
    bm_debug("Failed to send message %d\n",
             update_start_req_evt.header.frame_type);
  }
}

/**
 * @brief Send Chunk to Client
 *
 * @note Stuff bm_frame with image chunk and put into BM Serial TX Queue
 *
 * @return none
 */
static BmErr bm_dfu_host_send_chunk(BmDfuEventChunkRequest *req) {
  BmErr err = BmENOMEM;
  bm_debug("Processing chunk id %" PRIX32 "\n", req->seq_num);
  uint32_t payload_len =
      (host_ctx.bytes_remaining >= host_ctx.img_info.chunk_size)
          ? host_ctx.img_info.chunk_size
          : host_ctx.bytes_remaining;
  uint32_t payload_len_plus_header = sizeof(BcmpDfuPayload) + payload_len;
  uint8_t *buf = (uint8_t *)bm_malloc(payload_len_plus_header);

  if (buf) {
    BcmpDfuPayload *payload_header = (BcmpDfuPayload *)buf;
    payload_header->header.frame_type = BcmpDFUPayloadMessage;
    payload_header->chunk.addresses.src_node_id = host_ctx.self_node_id;
    payload_header->chunk.addresses.dst_node_id = host_ctx.client_node_id;
    payload_header->chunk.payload_length = payload_len;

    uint32_t flash_offset = DFU_IMG_START_OFFSET_BYTES +
                            host_ctx.img_info.image_size -
                            host_ctx.bytes_remaining;
    do {
      if (bm_dfu_internal()) {
        err = bm_dfu_host_get_chunk(flash_offset,
                                    payload_header->chunk.payload_buf,
                                    payload_len, flash_read_timeout_ms);
      } else if (host_ctx.data_queue) {
        //TODO: handle gracefully if cannot malloc
        uint8_t *data_buf = (uint8_t *)bm_malloc(host_ctx.img_info.chunk_size);
        err = bm_queue_receive(host_ctx.data_queue, data_buf,
                               host_ctx.host_timeout_ms);
        memcpy(payload_header->chunk.payload_buf, data_buf, payload_len);
        bm_free(data_buf);
      }
      if (err != BmOK) {
        bm_debug("Failed to read chunk from flash.\n");
        bm_dfu_host_transition_to_error(BmDfuErrFlashAccess);
        break;
      }

      err = bcmp_tx(&multicast_global_addr,
                    (BcmpMessageType)(payload_header->header.frame_type), buf,
                    payload_len_plus_header, 0, NULL);
      if (err == BmOK) {
        host_ctx.bytes_remaining -= payload_len;
        bm_debug("Message %d sent, payload size: %" PRIX32
                 ", remaining: %" PRIX32 "\n",
                 payload_header->header.frame_type, payload_len,
                 host_ctx.bytes_remaining);
      } else {
        bm_debug("Failed to send message %d\n",
                 payload_header->header.frame_type);
        bm_dfu_host_transition_to_error(BmDfuErrImgChunkAccess);
      }
    } while (0);

    bm_free(buf);
  }

  return err;
}

/**
 * @brief Send an update reboot to Client
 *
 * @return none
 */
static void bm_dfu_host_send_reboot(void) {
  BcmpDfuReboot reboot_msg;
  reboot_msg.addr.src_node_id = host_ctx.self_node_id;
  reboot_msg.addr.dst_node_id = host_ctx.client_node_id;
  reboot_msg.header.frame_type = BcmpDFURebootMessage;
  BmErr err = bcmp_tx(&multicast_global_addr,
                      (BcmpMessageType)(reboot_msg.header.frame_type),
                      (uint8_t *)(&reboot_msg), sizeof(BcmpDfuReboot), 0, NULL);
  if (err == BmOK) {
    bm_debug("Message %d sent \n", reboot_msg.header.frame_type);
  } else {
    bm_debug("Failed to send message %d\n", reboot_msg.header.frame_type);
  }
}

/**
 * @brief Entry Function for the Request Update State
 *
 * @note The Host sends an update request to the client and starts the ACK timeout timer
 *
 * @return none
 */
BmErr s_host_req_update_entry(void) {
  BmDfuEvent curr_evt = bm_dfu_get_current_event();

  /* Check if we even have a buf to inspect */
  if (!curr_evt.buf) {
    return BmENODATA;
  }

  BmDfuFrame *frame = (BmDfuFrame *)(curr_evt.buf);
  BmDfuEventImgInfo *img_info_evt =
      (BmDfuEventImgInfo *)(&((uint8_t *)(frame))[1]);
  host_ctx.img_info = img_info_evt->img_info;
  host_ctx.bytes_remaining = host_ctx.img_info.image_size;
  host_ctx.client_node_id = img_info_evt->addresses.dst_node_id;

  bm_debug("DFU Client Node Id: %016" PRIx64 "\n", host_ctx.client_node_id);

  host_ctx.ack_retry_num = 0;
  /* Request Client Firmware Update */
  bm_dfu_host_req_update();

  //TODO: place restrictions on how big the chunksize can be here?
  if (!bm_dfu_internal()) {
    host_ctx.data_queue =
        bm_queue_create(data_queue_size, img_info_evt->img_info.chunk_size);
  }

  /* Kickoff ACK timeout */
  return bm_timer_start(host_ctx.ack_timer, 10);
}

/**
 * @brief Run Function for the Request Update State
 *
 * @note The state is waiting on an ACK from the client to begin the update. Returns to idle state on timeout
 *
 * @return none
 */
BmErr s_host_req_update_run(void) {
  BmErr err = BmOK;
  BmDfuEvent curr_evt = bm_dfu_get_current_event();

  if (curr_evt.type == DfuEventAckReceived) {
    /* Stop ACK Timer */
    timer_err_check_and_return(err, bm_timer_stop(host_ctx.ack_timer, 10));
    BmDfuFrame *frame = (BmDfuFrame *)(curr_evt.buf);
    if (frame) {
      BmDfuEventResult *result_evt =
          (BmDfuEventResult *)(&((uint8_t *)(frame))[1]);

      if (result_evt->success) {
        bm_dfu_set_pending_state_change(BmDfuStateHostUpdate);
      } else {
        bm_dfu_host_transition_to_error((BmDfuErr)(result_evt->err_code));
      }
    } else {
      err = BmEINVAL;
    }
  } else if (curr_evt.type == DfuEventAckTimeout) {
    host_ctx.ack_retry_num++;

    /* Wait for ack until max retries is reached */
    if (host_ctx.ack_retry_num >= bm_dfu_max_ack_retries) {
      bm_dfu_host_transition_to_error(BmDfuErrTimeout);
    } else {
      bm_dfu_host_req_update();
      timer_err_check_and_return(err, bm_timer_start(host_ctx.ack_timer, 10));
    }
  } else if (curr_evt.type == DfuEventAbort) {
    BmDfuErr dfu_err = BmDfuErrAborted;
    if (curr_evt.buf) {
      BcmpDfuAbort *abort_evt = (BcmpDfuAbort *)(curr_evt.buf);
      dfu_err = (BmDfuErr)(abort_evt->err.err_code);
    }
    bm_debug("Recieved abort in request.\n");
    bm_dfu_host_transition_to_error(dfu_err);
  }

  return err;
}

/**
 * @brief Entry Function for the Update State
 *
 * @note The Host starts the host global timeout.
 *
 * @return none
 */
BmErr s_host_update_entry(void) {
  return bm_dfu_host_start_update_timer(host_ctx.host_timeout_ms);
}

/**
 * @brief Run Function for the Update State
 *
 * @note Host state that sends chunks of image to Client. Exits on global timeout timeout or abort/end message received from client
 *
 * @return none
 */
BmErr s_host_update_run(void) {
  BmErr err = BmOK;
  BmDfuFrame *frame = NULL;
  BmDfuEvent curr_evt = bm_dfu_get_current_event();

  /* Check if we even have a buf to inspect */
  if (curr_evt.buf) {
    frame = (BmDfuFrame *)(curr_evt.buf);
  }

  if (curr_evt.type == DfuEventChunkRequest) {
    if (frame) {
      BmDfuEventChunkRequest *chunk_req_evt =
          (BmDfuEventChunkRequest *)(&((uint8_t *)(frame))[1]);

      /* Request Next Chunk */
      /* Send Heartbeat to Client */
      timer_err_check_and_return(err,
                                 bm_timer_start(host_ctx.heartbeat_timer, 10));

      /* resend the frame to the client as is */
      bm_err_check(err, bm_dfu_host_send_chunk(chunk_req_evt));

      timer_err_check_and_return(err,
                                 bm_timer_stop(host_ctx.heartbeat_timer, 10));
    } else {
      err = BmENODATA;
    }
  } else if (curr_evt.type == DfuEventRebootRequest) {
    bm_dfu_host_send_reboot();
  } else if (curr_evt.type == DfuEventBootComplete) {
    if (frame) {
      bm_dfu_update_end(host_ctx.client_node_id, true, BmDfuErrNone);
    } else {
      err = BmENODATA;
    }
  } else if (curr_evt.type == DfuEventUpdateEnd) {
    timer_err_check_and_return(err, bm_timer_stop(host_ctx.update_timer, 100));
    if (frame) {
      BmDfuEventResult *update_end_evt =
          (BmDfuEventResult *)(&((uint8_t *)(frame))[1]);

      if (update_end_evt->success) {
        bm_debug("Successfully updated Client\n");
      } else {
        bm_debug("Client Update Failed\n");
      }
      if (host_ctx.update_complete_callback) {
        host_ctx.update_complete_callback(update_end_evt->success,
                                          (BmDfuErr)(update_end_evt->err_code),
                                          host_ctx.client_node_id);
      }
      bm_dfu_set_pending_state_change(BmDfuStateIdle);
    } else {
      err = BmENODATA;
    }
  } else if (curr_evt.type == DfuEventAbort) {
    BmDfuErr dfu_err = BmDfuErrAborted;
    if (curr_evt.buf) {
      BcmpDfuAbort *abort_evt = (BcmpDfuAbort *)(curr_evt.buf);
      dfu_err = (BmDfuErr)(abort_evt->err.err_code);
    }
    bm_debug("Recieved abort in run.\n");
    bm_dfu_host_transition_to_error(dfu_err);
  }

  return err;
}

BmErr s_host_update_exit(void) {
  if (host_ctx.data_queue) {
    bm_queue_delete(host_ctx.data_queue);
    host_ctx.data_queue = NULL;
  }

  return BmOK;
}

void bm_dfu_host_init(void) {
  int tmr_id = 0;

  /* Store relevant variables */
  host_ctx.self_node_id = node_id();

  /* Get DFU Subsystem Queue And Nullify Data Queue */
  host_ctx.dfu_event_queue = bm_dfu_get_event_queue();
  host_ctx.data_queue = NULL;

  /* Initialize ACK and Heartbeat Timer */
  host_ctx.ack_timer = bm_timer_create(
      "DFU Host Ack", bm_ms_to_ticks(bm_dfu_host_ack_timeout_ms), false,
      (void *)&tmr_id, ack_timer_handler);
  if (!host_ctx.ack_timer) {
    bm_debug("Could not create DFU host ACK timer...\n");
  }

  host_ctx.heartbeat_timer = bm_timer_create(
      "DFU Host Heartbeat", bm_ms_to_ticks(bm_dfu_host_heartbeat_timeout_ms),
      true, (void *)&tmr_id, heartbeat_timer_handler);
  if (!host_ctx.heartbeat_timer) {
    bm_debug("Could not create DFU host heartbeat timer...\n");
  }
  host_ctx.update_timer = bm_timer_create(
      "update timer", bm_ms_to_ticks(bm_dfu_update_default_timeout_ms), false,
      (void *)&tmr_id, update_timer_handler);
  if (host_ctx.update_timer) {
    bm_debug("Could not create DFU host update timer...\n");
  }
}

void bm_dfu_host_set_params(UpdateFinishCb update_complete_callback,
                            uint32_t host_timeout_ms) {
  host_ctx.update_complete_callback = update_complete_callback;
  host_ctx.host_timeout_ms = host_timeout_ms;
}

static BmErr bm_dfu_host_start_update_timer(uint32_t timeoutMs) {
  BmErr err = BmOK;

  bm_err_check(err, bm_timer_stop(host_ctx.update_timer, 100) == BmOK &&
                            bm_timer_change_period(host_ctx.update_timer,
                                                   bm_ms_to_ticks(timeoutMs),
                                                   100) == BmOK &&
                            bm_timer_start(host_ctx.update_timer, 100) == BmOK
                        ? BmOK
                        : BmETIMEDOUT);

  return err;
}

static void bm_dfu_host_transition_to_error(BmDfuErr dfu_err) {
  BmErr err = BmOK;

  bm_err_check(err,
               bm_timer_stop(host_ctx.update_timer, 100) == BmOK &&
                       bm_timer_stop(host_ctx.heartbeat_timer, 10) == BmOK &&
                       bm_timer_stop(host_ctx.ack_timer, 10) == BmOK
                   ? BmOK
                   : BmETIMEDOUT);
  bm_dfu_set_error(dfu_err);
  bm_dfu_set_pending_state_change(BmDfuStateError);
}

bool bm_dfu_host_client_node_valid(uint64_t client_node_id) {
  return host_ctx.client_node_id == client_node_id;
}

BmErr bm_dfu_host_queue_data(uint8_t *data, uint32_t size) {
  BmErr err = BmEINVAL;

  if (data && host_ctx.data_queue && size == host_ctx.img_info.chunk_size) {
    err = bm_queue_send(host_ctx.data_queue, data, 0);
  }

  return err;
}
