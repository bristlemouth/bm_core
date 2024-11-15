#include "spotter.h"
#include "bcmp.h"
#include "bm_common_pub_sub.h"
#include "bm_os.h"
#include "pubsub.h"
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define max_file_name_len 64
#define spotter_tx_max_transmit_size_bytes 311
#define max_str_len(fname_len)                                                 \
  (int32_t)(max_payload_len - sizeof(bm_print_publication_t) - fname_len)
static const uint8_t FPRINTF_TYPE = 1;
static const uint8_t PRINTF_TYPE = 1;
static const char *SPOTTER_TRANSMIT_DATA_TOPIC = "spotter/transmit-data";
static const char *SPOTTER_PRINTF_TOPIC = "spotter/printf";
static const char *SPOTTER_FPRINTF_TOPIC = "spotter/fprintf";
static const uint8_t SPOTTER_TRANSMIT_TOPIC_TYPE = 1;

typedef struct {
  // Network type to send over.
  BmSerialNetworkType type;
  // Data
  uint8_t data[0];
} __attribute__((packed)) BmSerialNetworkDataHeader;

/*!
 @brief Bristlemouth Generic fprintf Function

 @details Will publish the data to end in a file or
          to the console depending on if there is a file name

 @param[in] target_node_id - node_id to send to (0 = all nodes), the accept if it is
                             subscribed to the topic that the printf is publishing to
 @param[in] file_name - (optional) file name to print to (this will append to file")
 @param[in] print_time - whether or not the timestamp will be written to the file
 @param[in] *format - normal printf format string

 @return BmOk if successful
 @return BmErr if unsuccessful
*/
BmErr spotter_log(uint64_t target_node_id, const char *file_name,
                  uint8_t print_time, const char *format, ...) {
  BmErr err = BmOK;
  bm_print_publication_t *printf_pub = NULL;
  va_list va;

  do {
    va_start(va, format);
    // check how long the string we are printing will be
    int32_t data_len = vsnprintf(NULL, 0, format, va);
    va_start(va, format);
    if (data_len == 0) {
      err = BmENODATA;
      break;
    }

    int32_t fname_len = 0;
    if (file_name) {
      fname_len = bm_strnlen(file_name, max_file_name_len);
      if (fname_len >= max_file_name_len) {
        err = BmEMSGSIZE;
        break;
      }
    }

    if (data_len > max_str_len(fname_len)) {
      err = BmEMSGSIZE;
      break;
    }

    err = BmENOMEM;
    // Add 1 for NULL termination character on data_len
    size_t printf_pub_len =
        sizeof(bm_print_publication_t) + data_len + fname_len + 1;
    printf_pub = (bm_print_publication_t *)bm_malloc(printf_pub_len);

    if (printf_pub) {
      err = BmOK;
      memset(printf_pub, 0, printf_pub_len);
      printf_pub->target_node_id = target_node_id;
      printf_pub->fname_len = fname_len;
      printf_pub->data_len = data_len;
      printf_pub->print_time = print_time;

      if (file_name) {
        memcpy(printf_pub->fnameAndData, file_name, fname_len);
      }

      data_len +=
          1; // add one so vsnprintf doesn't overwrite the last character with null
      // do this after we malloc a buffer, so the extra null after the last character doesn't print to the file/ascii terminal
      int32_t res = vsnprintf((char *)&printf_pub->fnameAndData[fname_len],
                              data_len, format, va);
      if (res < 0 || (res != data_len - 1)) {
        err = BmEBADMSG;
        break;
      }

      if (file_name) {
        if (bm_pub(SPOTTER_FPRINTF_TOPIC, printf_pub, printf_pub_len,
                   FPRINTF_TYPE, BM_COMMON_PUB_SUB_VERSION) != BmOK) {
          err = BmENETDOWN;
        }
      } else {
        if (bm_pub(SPOTTER_PRINTF_TOPIC, printf_pub, printf_pub_len,
                   PRINTF_TYPE, BM_COMMON_PUB_SUB_VERSION) != BmOK) {
          err = BmENETDOWN;
        }
      }
    }
  } while (0);

  va_end(va);

  if (printf_pub) {
    bm_free(printf_pub);
    printf_pub = NULL;
  }

  return err;
}

/*!
 @brief Transmit data via the Spotter's satellite or cellular connection.
 
 @details Currently, there are 2 notable limitations on this function.
            - First, only network type `BmNetworkTypeCellularIriFallback`
              shows up in the Sofar Ocean API.
            - Second, the `data_len` has a realistic max of 311 bytes because
              Iridium messages are limited to 340 bytes, and Spotter adds a
              29-byte header.
 
 @param data Pointer to the data to transmit.
 @param data_len Length of the data to transmit. Max: 311.
 @param type Network type to send over. MUST be BmNetworkTypeCellularIriFallback.

 @return BmOk if successful
 @return BmErr if unsuccessful
*/
BmErr spotter_tx_data(const void *data, uint16_t data_len,
                      BmSerialNetworkType type) {
  BmErr err = BmEINVAL;
  size_t msg_len = sizeof(BmSerialNetworkDataHeader) + data_len;
  uint8_t *data_buf = (uint8_t *)bm_malloc(msg_len);
  if (data_buf) {
    do {
      if (data_len > spotter_tx_max_transmit_size_bytes) {
        break;
      }
      BmSerialNetworkDataHeader *header = (BmSerialNetworkDataHeader *)data_buf;
      header->type = type;
      memcpy(header->data, data, data_len);
      err = bm_pub(SPOTTER_TRANSMIT_DATA_TOPIC, data_buf, msg_len,
                   SPOTTER_TRANSMIT_TOPIC_TYPE, BM_COMMON_PUB_SUB_VERSION);
    } while (0);
    bm_free(data_buf);
  }
  return err;
}
