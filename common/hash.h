#ifndef __HASH_H__
#define __HASH_H__

#include "util.h"
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define invalid_key UINT32_MAX
#define tombstone_key (invalid_key - 1)
#define max_key_size tombstone_key

typedef struct {
  void *table;
  uint32_t *keys;
  uint16_t length;
  uint16_t size;
  uint16_t count;
} Hash;

Hash *hash_create(uint16_t size, uint16_t length);
BmErr hash_delete(Hash *hash);

BmErr hash_create_static(Hash *hash, void *table, uint32_t *keys, uint16_t size,
                         uint16_t length);

BmErr hash_insert(Hash *hash, uint32_t key, const void *data);
BmErr hash_look_up(Hash *hash, uint32_t key, void *data);
BmErr hash_remove(Hash *hash, uint32_t key);
uint16_t hash_get_count(Hash *hash);

#ifdef __cplusplus
}
#endif

#endif
