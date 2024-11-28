#define __STDC_FORMAT_MACROS
#include "dfu_client.h"
#include "bcmp.h"
#include "bm_config.h"
#include "bm_dfu_generic.h"
#include "bm_os.h"
#include "crc.h"
#include "device.h"
#include "dfu.h"
#include <inttypes.h>
#include <stdio.h>
#include <string.h>

#define timer_err_check_and_return(e, f)                                       \
  bm_err_report(e, f);                                                         \
  if (e != BmOK) {                                                             \
    bm_dfu_client_transition_to_error(BmDfuErrAborted);                        \
    return err;                                                                \
  }

typedef struct DfuClientCtx {
  BmQueue dfu_event_queue;
  /* Variables from DFU Start */
  uint32_t image_size;
  uint16_t num_chunks;
  uint16_t crc16;
  uint16_t running_crc16;
  /* Variables from DFU Payload */
  uint16_t chunk_length;
  /* Flash Mem variables */
  const void *fa;
  uint16_t img_page_byte_counter;
  uint32_t img_flash_offset;
  uint8_t img_page_buf[bm_img_page_length];
  /* Chunk variables */
  uint8_t chunk_retry_num;
  uint16_t current_chunk;
  BmTimer chunk_timer;
  uint64_t self_node_id;
  uint64_t host_node_id;
} DfuClientCtx;

static DfuClientCtx CLIENT_CTX;

static void bm_dfu_client_abort(BmDfuErr err);
static void bm_dfu_client_send_reboot_request();
static void bm_dfu_client_send_boot_complete(uint64_t host_node_id);
static void bm_dfu_client_transition_to_error(BmDfuErr err);
static void bm_dfu_client_fail_update_and_reboot(void);

/**
 * @brief Send DFU Abort to Host
 *
 * @note Stuff DFU Abort bm_frame and put into BM Serial TX Queue
 *
 * @param err    error to set in abort message
 *
 * @return none
 */
static void bm_dfu_client_abort(BmDfuErr err_code) {
  BcmpDfuAbort abort_msg;

  /* Populate the appropriate event */
  abort_msg.err.addresses.dst_node_id = CLIENT_CTX.host_node_id;
  abort_msg.err.addresses.src_node_id = CLIENT_CTX.self_node_id;
  abort_msg.err.err_code = err_code;
  abort_msg.err.success = 0;
  abort_msg.header.frame_type = BcmpDFUAbortMessage;
  BmErr err = bcmp_tx(&multicast_global_addr,
                      (BcmpMessageType)(abort_msg.header.frame_type),
                      (uint8_t *)(&abort_msg), sizeof(abort_msg), 0, NULL);
  if (err == BmOK) {
    bm_debug("Message %d sent \n", abort_msg.header.frame_type);
  } else {
    bm_debug("Failed to send message %d, error %d\n",
             abort_msg.header.frame_type, err);
  }
}

static void bm_dfu_client_send_reboot_request() {
  BcmpDfuRebootReq reboot_req;
  reboot_req.addr.src_node_id = CLIENT_CTX.self_node_id;
  reboot_req.addr.dst_node_id = CLIENT_CTX.host_node_id;
  reboot_req.header.frame_type = BcmpDFURebootReqMessage;
  BmErr err = bcmp_tx(
      &multicast_global_addr, (BcmpMessageType)(reboot_req.header.frame_type),
      (uint8_t *)(&reboot_req), sizeof(BcmpDfuRebootReq), 0, NULL);
  if (err == BmOK) {
    bm_debug("Message %d sent \n", reboot_req.header.frame_type);
  } else {
    bm_debug("Failed to send message %d, error %d\n",
             reboot_req.header.frame_type, err);
  }
}

static void bm_dfu_client_send_boot_complete(uint64_t host_node_id) {
  BcmpDfuBootComplete boot_compl;
  boot_compl.addr.src_node_id = CLIENT_CTX.self_node_id;
  boot_compl.addr.dst_node_id = host_node_id;
  boot_compl.header.frame_type = BcmpDFUBootCompleteMessage;
  BmErr err = bcmp_tx(
      &multicast_global_addr, (BcmpMessageType)(boot_compl.header.frame_type),
      (uint8_t *)(&boot_compl), sizeof(BcmpDfuBootComplete), 0, NULL);
  if (err == BmOK) {
    bm_debug("Message %d sent \n", boot_compl.header.frame_type);
  } else {
    bm_debug("Failed to send message %d, error %d\n",
             boot_compl.header.frame_type, err);
  }
}

/**
 * @brief Chunk Timer Handler function
 *
 * @note Puts Chunk Timeout event into DFU Subsystem event queue
 *
 * @param *tmr    Timer handler
 * @return none
 */
static void chunk_timer_handler(BmTimer tmr) {
  (void)tmr;
  BmDfuEvent evt = {DfuEventChunkTimeout, NULL, 0};
  BmErr err = BmOK;

  bm_err_check(err, bm_queue_send(CLIENT_CTX.dfu_event_queue, &evt, 0));
}

/**
 * @brief Write received chunks to flash
 *
 * @note Stores bytes in a local buffer until a page-worth of bytes can be written to flash.
 *
 * @param man_decode_len    Length of received decoded payload
 * @param man_decode_buf    Buffer of decoded payload
 * @return int32_t 0 on success, non-0 on error
 */
static int32_t bm_dfu_process_payload(uint16_t len, uint8_t *buf) {
  int32_t retval = 0;

  if (!len || !buf) {
    bm_debug("Invalid function parameters in %s\n", __func__);
    return 1;
  }

  do {
    if (bm_img_page_length > (len + CLIENT_CTX.img_page_byte_counter)) {
      memcpy(&CLIENT_CTX.img_page_buf[CLIENT_CTX.img_page_byte_counter], buf,
             len);
      CLIENT_CTX.img_page_byte_counter += len;

      if (CLIENT_CTX.img_page_byte_counter == bm_img_page_length) {
        CLIENT_CTX.img_page_byte_counter = 0;

        /* Perform page write and increment flash byte counter */
        retval = bm_dfu_client_flash_area_write(
            CLIENT_CTX.fa, CLIENT_CTX.img_flash_offset, CLIENT_CTX.img_page_buf,
            bm_img_page_length);
        if (retval) {
          bm_debug("Unable to write DFU frame to Flash");
          break;
        } else {
          CLIENT_CTX.img_flash_offset += bm_img_page_length;
        }
      }
    } else {
      uint16_t _remaining_page_length =
          bm_img_page_length - CLIENT_CTX.img_page_byte_counter;
      memcpy(&CLIENT_CTX.img_page_buf[CLIENT_CTX.img_page_byte_counter], buf,
             _remaining_page_length);
      CLIENT_CTX.img_page_byte_counter += _remaining_page_length;

      if (CLIENT_CTX.img_page_byte_counter == bm_img_page_length) {
        CLIENT_CTX.img_page_byte_counter = 0;

        /* Perform page write and increment flash byte counter */
        retval = bm_dfu_client_flash_area_write(
            CLIENT_CTX.fa, CLIENT_CTX.img_flash_offset, CLIENT_CTX.img_page_buf,
            bm_img_page_length);
        if (retval) {
          bm_debug("Unable to write DFU frame to Flash");
          break;
        } else {
          CLIENT_CTX.img_flash_offset += bm_img_page_length;
        }
      }

      /* Memcpy the remaining bytes to next page */
      memcpy(&CLIENT_CTX.img_page_buf[CLIENT_CTX.img_page_byte_counter],
             &buf[_remaining_page_length], (len - _remaining_page_length));
      CLIENT_CTX.img_page_byte_counter += (len - _remaining_page_length);
    }
  } while (0);

  return retval;
}

/**
 * @brief Finish flash writes of final DFU chunk
 *
 * @note Writes dirty bytes in buffer to flash for final chunk
 *
 * @return int32_t  0 on success, non-0 on error
 */
static int32_t bm_dfu_process_end(void) {
  int32_t retval = 0;

  /* If there are any dirty bytes, write to flash */
  if (CLIENT_CTX.img_page_byte_counter != 0) {
    /* Perform page write and increment flash byte counter */
    retval = bm_dfu_client_flash_area_write(
        CLIENT_CTX.fa, CLIENT_CTX.img_flash_offset, CLIENT_CTX.img_page_buf,
        (CLIENT_CTX.img_page_byte_counter));
    if (retval) {
      bm_debug("Unable to write DFU frame to Flash\n");
    } else {
      CLIENT_CTX.img_flash_offset += CLIENT_CTX.img_page_byte_counter;
    }
  }

  bm_dfu_client_flash_area_close(CLIENT_CTX.fa);
  return retval;
}

/**
 * @brief Process a DFU request from the Host
 *
 * @note Client confirms that the update is possible and necessary based on size and version numbers
 *
 * @return none
 */
void bm_dfu_client_process_update_request(void) {
  uint32_t image_size;
  uint16_t chunk_size;
  uint8_t minor_version;
  uint8_t major_version;

  BmDfuEvent curr_evt = bm_dfu_get_current_event();

  /* Check if we even have a buf to inspect */
  if (!curr_evt.buf) {
    return;
  }

  BmDfuFrame *frame = (BmDfuFrame *)(curr_evt.buf);
  BmDfuEventImgInfo *img_info_evt =
      (BmDfuEventImgInfo *)&((uint8_t *)(frame))[1];

  image_size = img_info_evt->img_info.image_size;
  chunk_size = img_info_evt->img_info.chunk_size;
  minor_version = img_info_evt->img_info.minor_ver;
  major_version = img_info_evt->img_info.major_ver;
  CLIENT_CTX.host_node_id = img_info_evt->addresses.src_node_id;

  if (img_info_evt->img_info.gitSHA != git_sha() ||
      img_info_evt->img_info.filter_key == BM_DFU_IMG_INFO_FORCE_UPDATE) {
    if (chunk_size > bm_dfu_max_chunk_size) {
      bm_dfu_client_abort(BmDfuErrAborted);
      bm_dfu_client_transition_to_error(BmDfuErrChunkSize);
      return;
    }
    CLIENT_CTX.image_size = image_size;

    /* We calculating the number of chunks that the client will be requesting based on the
           size of each chunk and the total size of the image. */
    if (image_size % chunk_size) {
      CLIENT_CTX.num_chunks = (image_size / chunk_size) + 1;
    } else {
      CLIENT_CTX.num_chunks = (image_size / chunk_size);
    }
    CLIENT_CTX.crc16 = img_info_evt->img_info.crc16;

    /* Open the secondary image slot */
    if (bm_dfu_client_flash_area_open(&CLIENT_CTX.fa) != BmOK) {
      bm_dfu_send_ack(CLIENT_CTX.host_node_id, 0, BmDfuErrFlashAccess);
      bm_dfu_client_transition_to_error(BmDfuErrFlashAccess);
    } else {

      if (bm_dfu_client_flash_area_get_size(CLIENT_CTX.fa) > image_size) {
        /* Erase memory in secondary image slot */
        bm_debug("Erasing flash\n");
        if (bm_dfu_client_flash_area_erase(
                CLIENT_CTX.fa, 0,
                bm_dfu_client_flash_area_get_size(CLIENT_CTX.fa)) != BmOK) {
          bm_debug("Error erasing flash!\n");
          bm_dfu_send_ack(CLIENT_CTX.host_node_id, 0, BmDfuErrFlashAccess);
          bm_dfu_client_transition_to_error(BmDfuErrFlashAccess);
        } else {
          bm_dfu_send_ack(CLIENT_CTX.host_node_id, 1, BmDfuErrNone);

          // Save image update info to noinit
          client_update_reboot_info.major = major_version;
          client_update_reboot_info.minor = minor_version;
          client_update_reboot_info.host_node_id = CLIENT_CTX.host_node_id;
          client_update_reboot_info.gitSHA = img_info_evt->img_info.gitSHA;
          client_update_reboot_info.magic = DFU_REBOOT_MAGIC;

          /* TODO: Fix this. Is this needed for FreeRTOS */
          bm_delay(10); // Needed so ACK can properly be sent/processed
          bm_dfu_set_pending_state_change(BmDfuStateClientReceiving);
        }
      } else {
        bm_debug("Image too large\n");
        bm_dfu_send_ack(CLIENT_CTX.host_node_id, 0, BmDfuErrTooLarge);
      }
    }
  } else {
    bm_debug("Same version requested\n");
    bm_dfu_send_ack(CLIENT_CTX.host_node_id, 0, BmDfuErrSameVer);
  }
}

BmErr s_client_validating_run(void) { return BmOK; }
BmErr s_client_activating_run(void) { return BmOK; }

/**
 * @brief Entry Function for the Client Receiving State
 *
 * @note Client will send the first request for image chunk 0 from the host and kickoff a Chunk timeout timer
 *
 * @return none
 */
BmErr s_client_receiving_entry(void) {
  /* Start from Chunk #0 */
  CLIENT_CTX.current_chunk = 0;
  CLIENT_CTX.chunk_retry_num = 0;
  CLIENT_CTX.img_page_byte_counter = 0;
  CLIENT_CTX.img_flash_offset = 0;
  CLIENT_CTX.running_crc16 = 0;

  /* Request Next Chunk */
  bm_dfu_req_next_chunk(CLIENT_CTX.host_node_id, CLIENT_CTX.current_chunk);

  /* Kickoff Chunk timeout */
  return bm_timer_start(CLIENT_CTX.chunk_timer, 10);
}

/**
 * @brief Run Function for the Client Receiving State
 *
 * @note Client will periodically request specific image chunks from the Host
 *
 * @return none
 */
BmErr s_client_receiving_run(void) {
  BmDfuEvent curr_evt = bm_dfu_get_current_event();
  BmErr err = BmEINVAL;

  if (curr_evt.type == DfuEventImageChunk && curr_evt.buf) {
    BmDfuFrame *frame = (BmDfuFrame *)(curr_evt.buf);
    BmDfuEventImageChunk *image_chunk_evt =
        (BmDfuEventImageChunk *)&((uint8_t *)(frame))[1];

    /* Stop Chunk Timer */
    timer_err_check_and_return(err, bm_timer_stop(CLIENT_CTX.chunk_timer, 10));

    /* Get Chunk Length and Chunk */
    CLIENT_CTX.chunk_length = image_chunk_evt->payload_length;

    /* Calculate Running CRC */
    CLIENT_CTX.running_crc16 =
        crc16_ccitt(CLIENT_CTX.running_crc16, image_chunk_evt->payload_buf,
                    CLIENT_CTX.chunk_length);

    /* Process the frame */
    if (bm_dfu_process_payload(CLIENT_CTX.chunk_length,
                               image_chunk_evt->payload_buf)) {
      bm_dfu_client_transition_to_error(BmDfuErrBmFrame);
    }

    /* Request Next Chunk */
    CLIENT_CTX.current_chunk++;
    CLIENT_CTX.chunk_retry_num = 0;

    if (CLIENT_CTX.current_chunk < CLIENT_CTX.num_chunks) {
      bm_dfu_req_next_chunk(CLIENT_CTX.host_node_id, CLIENT_CTX.current_chunk);
      timer_err_check_and_return(err,
                                 bm_timer_start(CLIENT_CTX.chunk_timer, 10));
    } else {
      /* Process the frame */
      if (bm_dfu_process_end()) {
        bm_dfu_client_transition_to_error(BmDfuErrBmFrame);
      } else {
        bm_dfu_set_pending_state_change(BmDfuStateClientValidating);
      }
    }
  } else if (curr_evt.type == DfuEventChunkTimeout) {
    CLIENT_CTX.chunk_retry_num++;
    /* Try requesting chunk until max retries is reached */
    if (CLIENT_CTX.chunk_retry_num >= bm_dfu_max_chunk_retries) {
      bm_dfu_client_abort(BmDfuErrAborted);
      bm_dfu_client_transition_to_error(BmDfuErrTimeout);
    } else {
      bm_dfu_req_next_chunk(CLIENT_CTX.host_node_id, CLIENT_CTX.current_chunk);
      timer_err_check_and_return(err,
                                 bm_timer_start(CLIENT_CTX.chunk_timer, 10));
    }
  } else if (
      curr_evt.type ==
      DfuEventReceivedUpdateRequest) { // The host dropped our previous ack to the image, and we need to sync up.
    timer_err_check_and_return(err, bm_timer_stop(CLIENT_CTX.chunk_timer, 10));
    bm_dfu_send_ack(CLIENT_CTX.host_node_id, 1, BmDfuErrNone);
    // Start image from the beginning
    CLIENT_CTX.current_chunk = 0;
    CLIENT_CTX.chunk_retry_num = 0;
    CLIENT_CTX.img_page_byte_counter = 0;
    CLIENT_CTX.img_flash_offset = 0;
    CLIENT_CTX.running_crc16 = 0;
    bm_delay(100); // Allow host to process ACK and Get ready to send chunk.
    bm_dfu_req_next_chunk(CLIENT_CTX.host_node_id, CLIENT_CTX.current_chunk);
    timer_err_check_and_return(err, bm_timer_start(CLIENT_CTX.chunk_timer, 10));
  }
  /* TODO: (IMPLEMENT THIS PERIODICALLY ON HOST SIDE)
       If host is still waiting for chunk, it will send a heartbeat to client */
  else if (curr_evt.type == DfuEventHeartbeat) {
    timer_err_check_and_return(err, bm_timer_start(CLIENT_CTX.chunk_timer, 10));
  }

  return err;
}

/**
 * @brief Entry Function for the Client Validation State
 *
 * @note If the CRC and image lengths match, move to Client Activation State
 *
 * @return none
 */
BmErr s_client_validating_entry(void) {
  /* Verify image length */
  if (CLIENT_CTX.image_size != CLIENT_CTX.img_flash_offset) {
    bm_debug("Rx Len: %" PRIu32 ", Actual Len: %" PRIu32 "\n",
             CLIENT_CTX.image_size, CLIENT_CTX.img_flash_offset);
    bm_dfu_update_end(CLIENT_CTX.host_node_id, 0, BmDfuErrMismatchLen);
    bm_dfu_client_transition_to_error(BmDfuErrMismatchLen);

  } else {
    /* Verify CRC. If ok, then move to Activating state */
    if (CLIENT_CTX.crc16 == CLIENT_CTX.running_crc16) {
      bm_dfu_set_pending_state_change(BmDfuStateClientRebootReq);
    } else {
      bm_debug("Expected Image CRC: %d | Calculated Image CRC: %d\n",
               CLIENT_CTX.crc16, CLIENT_CTX.running_crc16);
      bm_dfu_update_end(CLIENT_CTX.host_node_id, 0, BmDfuErrBadCrc);
      bm_dfu_client_transition_to_error(BmDfuErrBadCrc);
    }
  }

  return BmOK;
}

/**
 * @brief Entry Function for the Client Activating State
 *
 * @note Upon confirmation to update recepit from the host, the device will set pending image bit and reboot
 *
 * @return none
 */
BmErr s_client_activating_entry(void) {
  /* Set as temporary switch. New application must confirm or else MCUBoot will
    switch back to old image */
  bm_debug("Successful transfer. Should be resetting\n");

  /* Add a small delay so DFU_END message can get out to (Host Node + Desktop) before resetting device */
  bm_delay(10);
  bm_dfu_client_set_pending_and_reset();

  return BmOK;
}

/**
 * @brief Entry Function for the Client reboot request State
 *
 * @note Client will send the first request for reboot from the host and kickoff a Chunk timeout timer
 *
 * @return none
 */
BmErr s_client_reboot_req_entry(void) {
  CLIENT_CTX.chunk_retry_num = 0;
  /* Request reboot */
  bm_dfu_client_send_reboot_request();

  /* Kickoff Chunk timeout */
  return bm_timer_start(CLIENT_CTX.chunk_timer, 10);
}

/**
 * @brief Run Function for the Client Reboot Request State
 *
 * @note Upon validation of received image, the device will request confirmation to reboot from the host.
 *
 * @return none
 */
BmErr s_client_reboot_req_run(void) {
  BmDfuEvent curr_evt = bm_dfu_get_current_event();
  BmErr err = BmOK;

  if (curr_evt.type == DfuEventReboot) {
    if (curr_evt.buf) {
      err = bm_timer_stop(CLIENT_CTX.chunk_timer, 10);
      bm_dfu_set_pending_state_change(BmDfuStateClientActivating);
    } else {
      err = BmENODATA;
    }
  } else if (curr_evt.type == DfuEventChunkTimeout) {
    CLIENT_CTX.chunk_retry_num++;
    /* Try requesting reboot until max retries is reached */
    if (CLIENT_CTX.chunk_retry_num >= bm_dfu_max_chunk_retries) {
      bm_dfu_client_abort(BmDfuErrAborted);
      bm_dfu_client_transition_to_error(BmDfuErrTimeout);
    } else {
      bm_dfu_client_send_reboot_request();
      err = bm_timer_start(CLIENT_CTX.chunk_timer, 10);
    }
  }

  return err;
}

/**
 * @brief Entry Function for the Client Update Done
 *
 * @return none
 */
BmErr s_client_update_done_entry(void) {
  BmErr err = BmOK;
  CLIENT_CTX.host_node_id = client_update_reboot_info.host_node_id;
  CLIENT_CTX.chunk_retry_num = 0;

  if (git_sha() == client_update_reboot_info.gitSHA) {
    // We usually want to confirm the update, but if we want to force-confirm, we read a flag in the configuration,
    // confirm, reset the config flag, and then reboot.
    if (!bm_dfu_client_confirm_is_enabled()) {
      memset(&client_update_reboot_info, 0, sizeof(client_update_reboot_info));
      bm_dfu_client_set_confirmed();
      bm_dfu_client_confirm_enable(true); // Reboot!
    } else {
      bm_dfu_client_send_boot_complete(client_update_reboot_info.host_node_id);
      /* Kickoff Chunk timeout */
      err = bm_timer_start(CLIENT_CTX.chunk_timer, 10);
    }
  } else {
    bm_dfu_update_end(client_update_reboot_info.host_node_id, false,
                      BmDfuErrWrongVer);
    bm_dfu_client_fail_update_and_reboot();
  }

  return err;
}

/**
 * @brief Run Function for the Client Update Done
 *
 * @note After rebooting and confirming an update, client will let the host know that the update is complete.
 *
 * @return none
 */
BmErr s_client_update_done_run(void) {
  BmErr err = BmOK;
  BmDfuEvent curr_evt = bm_dfu_get_current_event();

  if (curr_evt.type == DfuEventUpdateEnd) {
    if (curr_evt.buf) {
      err = bm_timer_stop(CLIENT_CTX.chunk_timer, 10);
      bm_dfu_client_set_confirmed();
      bm_debug("Boot confirmed!\n Update success!\n");
      bm_dfu_update_end(client_update_reboot_info.host_node_id, true,
                        BmDfuErrNone);
      bm_dfu_set_pending_state_change(BmDfuStateIdle);
    } else {
      err = BmENODATA;
    }
  } else if (curr_evt.type == DfuEventChunkTimeout) {
    CLIENT_CTX.chunk_retry_num++;
    /* Try requesting confirmation until max retries is reached */
    if (CLIENT_CTX.chunk_retry_num >= bm_dfu_max_chunk_retries) {
      bm_dfu_client_abort(BmDfuErrConfirmationAbort);
      bm_dfu_client_fail_update_and_reboot();
    } else {
      /* Request confirmation */
      bm_dfu_client_send_boot_complete(client_update_reboot_info.host_node_id);
      err = bm_timer_start(CLIENT_CTX.chunk_timer, 10);
    }
  }

  return err;
}

/**
 * @brief Initialization function for the DFU Client subsystem
 *
 * @note Gets relevant message queues and semaphores, and creates Chunk Timeout Timer
 *
 * @param sock    Socket from DFU Core
 * @return none
 */

void bm_dfu_client_init(void) {
  int32_t tmr_id = 0;

  /* Store relevant variables */
  CLIENT_CTX.self_node_id = node_id();

  /* Get DFU Subsystem Queue */
  CLIENT_CTX.dfu_event_queue = bm_dfu_get_event_queue();

  CLIENT_CTX.chunk_timer = bm_timer_create(
      "DFU Client Chunk Timer", bm_ms_to_ticks(bm_dfu_client_chunk_timeout_ms),
      false, (void *)&tmr_id, chunk_timer_handler);
  if (!CLIENT_CTX.chunk_timer) {
    bm_debug("Could not create DFU client timer...\n");
  }
}

static void bm_dfu_client_transition_to_error(BmDfuErr err) {
  if (bm_timer_stop(CLIENT_CTX.chunk_timer, 10) != BmOK) {
    bm_debug("Could not stop dfu timer in: %s\n", __func__);
  }
  bm_dfu_set_error(err);
  bm_dfu_set_pending_state_change(BmDfuStateError);
}

static void bm_dfu_client_fail_update_and_reboot(void) {
  memset(&client_update_reboot_info, 0, sizeof(client_update_reboot_info));
  bm_delay(100); // Wait a bit for any previous messages sent.
  bm_dfu_client_fail_update_and_reset();
}

bool bm_dfu_client_host_node_valid(uint64_t host_node_id) {
  return CLIENT_CTX.host_node_id == host_node_id;
}

/*!
 * UNIT TEST FUNCTIONS BELOW HERE
 */
#ifdef ENABLE_TESTING
void bm_dfu_test_set_client_fa(void *fa) { CLIENT_CTX.fa = fa; }
#endif //ENABLE_TESTING
