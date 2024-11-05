#pragma once

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
  BM_CFG_PARTITION_USER,
  BM_CFG_PARTITION_SYSTEM,
  BM_CFG_PARTITION_HARDWARE,
  BM_CFG_PARTITION_COUNT
} BmConfigPartition;

typedef struct {
  // fw version
  uint8_t major;
  uint8_t minor;
  uint8_t revision;
  uint32_t gitSHA;
} __attribute__((packed)) BmFwVersion;

typedef struct {
  // Partition id
  BmConfigPartition partition;
  // Partion crc
  uint32_t crc32;
} __attribute__((packed)) BmConfigCrc;

typedef struct {
  // crc of the this message (excluding itself)
  uint32_t network_crc32;
  // Config crc
  BmConfigCrc config_crc;
  // fw info
  BmFwVersion fw_info;
  // Number of nodes in the topology
  uint16_t num_nodes;
  // Size of the map in bytes
  uint16_t map_size_bytes;
  // Node list (node size uint64_t) and cbor config map
  uint8_t node_list_and_cbor_config_map[0];
} __attribute__((packed)) BmNetworkInfo;

typedef enum {
  BM_COMMON_LOG_LEVEL_NONE = 0,
  BM_COMMON_LOG_LEVEL_FATAL = 1,
  BM_COMMON_LOG_LEVEL_ERROR = 2,
  BM_COMMON_LOG_LEVEL_WARNING = 3,
  BM_COMMON_LOG_LEVEL_INFO = 4,
  BM_COMMON_LOG_LEVEL_DEBUG = 5,
} BmLogLevel;

typedef struct {
  // Log level
  BmLogLevel level;
  // String length of the message (without terminator)
  uint32_t message_length;
  // print header (true) or not (false)
  bool print_header;
  // timestamp UTC
  uint64_t timestamp_utc_s;
  // Message string
  char message[0];
} __attribute__((packed)) BmLog;

#ifdef __cplusplus
}
#endif
