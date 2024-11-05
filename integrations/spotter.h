#include "util.h"
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef __cplusplus
extern "C" {
#endif

#define USE_TIMESTAMP 1
#define NO_TIMESTAMP 0

typedef enum {
  BmNetworkTypeCellularIriFallback = (1 << 0),
  BmNetworkTypeCellularOnly = (1 << 1),
} BmSerialNetworkType;

BmErr bm_fprintf(uint64_t target_node_id, const char *file_name,
                 uint8_t print_time, const char *format, ...);
BmErr bm_file_append(uint64_t target_node_id, const char *file_name,
                     const uint8_t *buff, uint16_t len);
BmErr spotter_tx_data(const void *data, uint16_t data_len,
                      BmSerialNetworkType type);

#define bm_printf(target_node_id, format, ...)                                 \
  bm_fprintf(target_node_id, NULL, USE_TIMESTAMP, format, ##__VA_ARGS__)

#ifdef __cplusplus
}
#endif
