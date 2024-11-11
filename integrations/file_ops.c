#include "file_ops.h"
#include "bm_common_pub_sub.h"
#include "bm_os.h"
#include "pubsub.h"
#include <string.h>

#define max_file_name_len 64

static const uint8_t FAPPEND_TYPE = 1;

/*!
 @brief Bristlemouth generic append to file

 @param[in] target_node_id - node_id to send to (0 = all nodes), the accept if it is
                             subscribed to the topic that the printf is publishing to
 @param[in] file_name - file name to print to
 @param[in] *buf - buffer to send

 @return BmOk if successful
 @return BmErr if unsuccessful
*/
BmErr bm_file_append(uint64_t target_node_id, const char *file_name,
                     const uint8_t *buf, uint16_t len) {
  BmErr err = BmEINVAL;

  if (file_name && buf) {
    err = BmENOMEM;
    int32_t fname_len = bm_strnlen(file_name, max_file_name_len);
    const size_t file_append_pub_len =
        sizeof(bm_print_publication_t) + len + fname_len;
    bm_print_publication_t *file_append_pub =
        (bm_print_publication_t *)bm_malloc(file_append_pub_len);

    if (file_append_pub) {
      file_append_pub->target_node_id = target_node_id;
      file_append_pub->fname_len = fname_len;
      file_append_pub->data_len = len;
      memcpy(file_append_pub->fnameAndData, file_name, fname_len);
      memcpy(&file_append_pub->fnameAndData[fname_len], buf, len);
      err = bm_pub("fappend", file_append_pub, file_append_pub_len,
                   FAPPEND_TYPE, BM_COMMON_PUB_SUB_VERSION)
                ? BmOK
                : BmENETDOWN;
      bm_free(file_append_pub);
    }
  }

  return err;
}
