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

BmErr spotter_log(uint64_t target_node_id, const char *file_name,
                  uint8_t print_time, const char *format, ...);
BmErr spotter_tx_data(const void *data, uint16_t data_len,
                      BmSerialNetworkType type);

#define spotter_log_console(target_node_id, format, ...)                       \
  spotter_log(target_node_id, NULL, USE_TIMESTAMP, format, ##__VA_ARGS__)

#ifdef __cplusplus
}
#endif
