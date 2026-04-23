#include "hash.h"
#include "bm_os.h"
#include <string.h>

#define get_idx_wrap_around(key, i) ((key + i) % hash->length)
#define valid_key(key) (key != invalid_key && key != tombstone_key)

/*!
 @brief Iterate Through Hash Table And Obtain Index For Requested Action

 @param hash Hash instance to iterate through table
 @param key Key to find in the hash instances key array
 @param idx Idx in the hash table that the key exists
 @param insert Whether this operation is an insert operation or not

 @return BmOK on success
         BmENOSPC if table is full and insert operation
         BmEALREADY if this exact key exists in the table and insert operation
         BmENODATA if key could not be found or table is empty
 */
static BmErr iterate_table(const Hash *hash, uint32_t key, uint32_t *idx,
                           bool insert) {
  if (insert && hash->count == hash->length) {
    return BmENOSPC;
  } else if (!insert && !hash->count) {
    return BmENODATA;
  }

  for (*idx = 0; *idx < hash->length; (*idx)++) {
    uint32_t conditioned_idx = get_idx_wrap_around(key, *idx);
    uint32_t table_key = hash->keys[conditioned_idx];
    bool is_empty = table_key == invalid_key;
    bool is_available = is_empty || table_key == tombstone_key;
    bool is_equal = key == table_key;

    if (insert && is_available) {
      break;
    } else if (insert && is_equal) {
      return BmEALREADY;
    } else if (!insert && is_empty) {
      return BmENODATA;
    } else if (!insert && is_equal) {
      break;
    }
  }

  if (*idx >= hash->length) {
    return BmENODATA;
  }

  *idx = get_idx_wrap_around(key, *idx);
  return BmOK;
}

/*!
 @brief Create A Dynamically Allocated Hash Table

 @details Creates the hash struct and buffers needed to store the hash table
          as well as keys.

 @param size Size of each element in the hash table
 @param length Length of the hash table

 @return Pointer to hash element on success
         NULL on failure
 */
Hash *hash_create(uint16_t size, uint16_t length) {
  if (!size || !length) {
    return NULL;
  }

  Hash *hash = bm_malloc(sizeof(Hash));
  if (!hash) {
    return NULL;
  }

  const uint32_t hash_table_total_size = size * length;
  const uint32_t keys_size = length * sizeof(*hash->keys);

  hash->table = bm_malloc(hash_table_total_size);
  if (!hash->table) {
    bm_free(hash);
    return NULL;
  }

  hash->keys = bm_malloc(keys_size);
  if (!hash->keys) {
    bm_free(hash->table);
    bm_free(hash);
    return NULL;
  }

  memset(hash->table, 0, hash_table_total_size);
  memset(hash->keys, invalid_key, keys_size);
  hash->size = size;
  hash->length = length;
  hash->count = 0;

  return hash;
}

/*!
 @brief Free A Dynamically Allocated Hash Table

 @param hash Hash instance to free

 @return BmOK on success
         BmEINVAL if hash is invalid
 */
BmErr hash_delete(Hash *hash) {
  if (!hash) {
    return BmEINVAL;
  }

  bm_free(hash->table);
  bm_free(hash->keys);
  hash->table = NULL;
  hash->keys = NULL;
  bm_free(hash);
  hash = NULL;

  return BmOK;
}

/*!
 @brief Create A Statically Allocated Hash Table

 @details Initializes a hash table with static elements. The hash structure
          and buffers for both table and keys must be pre-allocated to
          preserve the state of the hash table during runtime. The hash table
          buffer must be (size * length) bytes long, the keys buffer must be
          length uint32_t elements.

 @param hash Hash instance to initialize
 @param table Table buffer of size * length bytes
 @param keys Keys buffer of length
 @param size Size of each element in the hash table
 @param length Length of the hash table

 @return BmOK on success
         BmEINVAL if invalid parameters
 */
BmErr hash_create_static(Hash *hash, void *table, uint32_t *keys, uint16_t size,
                         uint16_t length) {
  if (!hash || !table || !keys || !size || !length) {
    return BmEINVAL;
  }

  const uint32_t hash_table_total_size = size * length;
  const uint32_t keys_size = length * sizeof(*hash->keys);

  hash->table = table;
  hash->keys = keys;

  memset(hash->table, 0, hash_table_total_size);
  memset(hash->keys, invalid_key, keys_size);
  hash->size = size;
  hash->length = length;
  hash->count = 0;

  return BmOK;
}

/*!
 @brief Insert Data Into Hash Table

 @details Inserts data with key into hash table with linear probing. When a
          collision occurs, the algorithm linearly searches for the next
          available slot in the table.

 @note invalid_key and tombstone_key are key values that cannot be used

 @param hash Hash instance to add data to
 @param key Key associated with data
 @param data Data to store in table, must be exactly hash->size bytes

 @return BmOK on success
         BmEINVAL if argument is invalid
         BmENOSPC if table is full
         BmEALREADY if this exact key exists in the table
 */
BmErr hash_insert(Hash *hash, uint32_t key, const void *data) {
  if (!hash || !data || !valid_key(key)) {
    return BmEINVAL;
  }

  uint32_t i;
  BmErr err = iterate_table(hash, key, &i, true);
  if (err != BmOK) {
    return err;
  }

  // Set key as used
  hash->keys[i] = key;

  // Copy data to location
  i *= hash->size;
  memcpy((uint8_t *)hash->table + i, data, hash->size);
  hash->count++;

  return BmOK;
}

/*!
 @brief Look Up Data In Hash Table

 @details Finds data in hash table with key and copies data to a buffer if
          found.

 @note invalid_key and tombstone_key are key values that cannot be used

 @param hash Hash instance to perform look up
 @param key Key value to look up
 @param data Buffer to store hash table data into 

 @return BmOK on success
         BmEINVAL if argument is invalid
         BmENODATA if key could not be found or table is empty
 */
BmErr hash_look_up(Hash *hash, uint32_t key, void *data) {
  if (!hash || !data || !valid_key(key)) {
    return BmEINVAL;
  }

  uint32_t i;
  BmErr err = iterate_table(hash, key, &i, false);
  if (err != BmOK) {
    return err;
  }

  // Copy location to data
  i *= hash->size;
  memcpy(data, (uint8_t *)hash->table + i, hash->size);

  return BmOK;
}

/*!
 @brief Remove Key In Hash Table

 @details Finds data in hash table with key and removes it. Marks the
          key as a tombstone to indicate that it is available to
          have data inserted again.

 @note invalid_key and tombstone_key are key values that cannot be used

 @param hash Hash instance to perform erase
 @param key Key value to remove

 @return BmOK on success
         BmEINVAL if argument is invalid
         BmENODATA if key could not be found or table is empty
 */
BmErr hash_remove(Hash *hash, uint32_t key) {
  if (!hash || !valid_key(key)) {
    return BmEINVAL;
  }

  uint32_t i;
  BmErr err = iterate_table(hash, key, &i, false);
  if (err != BmOK) {
    return err;
  }

  // Set key as unused
  hash->keys[i] = tombstone_key;
  hash->count--;

  return BmOK;
}

/*!
 @brief Get Number Of Inserted Elements In Hash Table

 @param hash Hash instance to obtain count

 @return Count of hash elements
         0 if hash is invalid
 */
uint16_t hash_get_count(Hash *hash) {
  if (!hash) {
    return 0;
  }

  return hash->count;
}

/*!
 @brief Returns Load Factor Of Hash Table

 @details Load factor is calculated as a value between 0 and 100.

 @param hash Hash instance to obtain load factor

 @return Load factor from 0 - 100
         0 if hash is invalid
 */
uint8_t hash_get_load(Hash *hash) {
  if (!hash || !hash->length) {
    return 0;
  }

  return hash->count * 100 / hash->length;
}
