#include "bm_configs_generic.h"
#include "configuration.h"
#include "messages.h"
#include "util.h"
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

BmErr bcmp_config_init(void);
bool bcmp_config_get(uint64_t target_node_id, BmConfigPartition partition,
                     size_t key_len, const char *key, BmErr *err,
                     BmErr (*reply_cb)(uint8_t *));
bool bcmp_config_set(uint64_t target_node_id, BmConfigPartition partition,
                     size_t key_len, const char *key, size_t value_size,
                     void *val, BmErr *err, BmErr (*reply_cb)(uint8_t *));
bool bcmp_config_commit(uint64_t target_node_id, BmConfigPartition partition,
                        BmErr *err);
bool bcmp_config_status_request(uint64_t target_node_id,
                                BmConfigPartition partition, BmErr *err,
                                BmErr (*reply_cb)(uint8_t *));
bool bcmp_config_del_key(uint64_t target_node_id, BmConfigPartition partition,
                         size_t key_len, const char *key,
                         BmErr (*reply_cb)(uint8_t *));

#ifdef __cplusplus
}
#endif
