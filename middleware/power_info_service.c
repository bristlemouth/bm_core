#include "power_info_service.h"
#include "bm_config.h"
#include "bm_os.h"
#include "bm_service.h"
#include "bm_service_request.h"
#include "cb_queue.h"
#include <inttypes.h>
#include <string.h>

#define bus_power_service "bus_power_controller"
#define power_info_service_suffix "/timing"
#define power_info_service bus_power_service power_info_service_suffix

typedef struct {
  BmPowerInfoStatsCb power_cb;
  void *arg;
} PowerInfoServiceCtx;

static BmCbQueue service_queue = {0};
static PowerInfoServiceCtx service_ctx = {0};

/*!
  @brief Callback Handler For "bus_power_controller/timing/req" Topic

  @details Responsible for obtaining the power information via a callback
           and CBOR encoding that information to be sent on the
           "bus_power_controller/timing/rep" topic

  @param service_strlen service string length
  @param service service string
  @param req_data_len length of data for request
  @param req_data data for the request (unused, should not have any data)
  @param buffer_len length of reply data buffer
  @param reply_data reply data buffer

  @return true on success, false on failure
 */
static bool power_info_request_cb(size_t service_strlen, const char *service,
                                  size_t req_data_len, uint8_t *req_data,
                                  size_t *buffer_len, uint8_t *reply_data) {
  bool ret = false;
  PowerInfoReplyData d = {0};
  size_t encoded_len = 0;
  (void)(req_data);

  bm_debug("Data received on service: %.*s\n", (int)service_strlen, service);

  if (!req_data_len) {
    if (service_ctx.power_cb) {
      d = service_ctx.power_cb(service_ctx.arg);
      if (power_info_reply_encode(&d, reply_data, *buffer_len, &encoded_len) ==
          CborNoError) {
        *buffer_len = encoded_len;
        ret = true;
      } else {
        bm_debug("Could not properly encode data on service\n");
      }
    } else {
      bm_debug("Callback invalid, cannot reply on service\n");
    }
  } else {
    bm_debug("Invalid data received on %.*s\n", (int)service_strlen, service);
  }

  return ret;
}

/*!
  @brief Callback Handler For "bus_power_controller/timing/rep" Topic

  @details This is responsible for handling replies from the service request 
           to the "bus_power_controller/timing/req" topic. If a timeout did
           not occur, will invoke a callback passed into the
           power_info_service_request function.

  @param ack whether a reply was received or not
  @param msg_id id of message sent
  @param service_strlen service string length
  @param service service string
  @param reply_len reply buffer length
  @param reply_data reply buffer to be decoded

  @return true if message was handled properly, false if it was not
 */
static bool power_info_reply_cb(bool ack, uint32_t msg_id,
                                size_t service_strlen, const char *service,
                                size_t reply_len, uint8_t *reply_data) {
  bool ret = false;
  (void)msg_id;

  PowerInfoReplyData d = {0};
  if (ack &&
      power_info_reply_decode(&d, reply_data, reply_len) == CborNoError) {
    bm_debug("Service: %.*s\n", (int)service_strlen, service);
    bm_debug("Reply: \n");
    bm_debug(" * total_on_s: %" PRIu32 "\n", d.total_on_s);
    bm_debug(" * remaining_on_s: %" PRIu32 "\n", d.remaining_on_s);
    bm_debug(" * upcoming_off_s: %" PRIu32 "\n", d.upcoming_off_s);
    ret = true;
  } else {
    bm_debug("Power info service request failure, ack: %d\n", ack);
  }

  ret = queue_cb_dequeue(&service_queue, &d, ret) == BmOK;

  return ret;
}

/*!
  @brief Initialize The Power Information Timing Service

  @details Registers a service that informs other nodes on the network of
           the timing information for the power on the network. This
           information has the following layout:

           {
             "total_on_s": 310,
             "remaining_on_s": 121,
             ”upcoming_off_s”: 1500
           }

  @param power_cb callback necessary to obtain power info timing on the network
  @param arg argument to pass to callback function

  @return BmOK on success, BmErr on failure
 */
BmErr power_info_service_init(BmPowerInfoStatsCb power_cb, void *arg) {
  BmErr err = BmEINVAL;

  if (power_cb) {
    err = bm_service_register(strlen(power_info_service), power_info_service,
                              power_info_request_cb) == true
              ? BmOK
              : BmECANCELED;
    service_ctx.power_cb = power_cb;
    service_ctx.arg = arg;
  }

  return err;
}

/*!
  @brief Make A Power Information Timing Request

  @details Makes a request on the "bus_power_controller/timing/rep" topic and
           invokes a callback if a reply was received.

  @param reply_cb callback function if a reply is received
  @param timeout_s timeout in seconds to wait for reply

  @return BmOK on success, BmErr on failure
 */
BmErr power_info_service_request(BmPowerInfoReplyCb reply_cb,
                                 uint32_t timeout_s) {
  BmErr err = queue_cb_enqueue(&service_queue, (BmQueueCb)reply_cb);

  if (err == BmOK) {
    err = bm_service_request(strlen(power_info_service), power_info_service, 0,
                             NULL, power_info_reply_cb, timeout_s) == true
              ? BmOK
              : BmEBADMSG;
  }

  return err;
}
