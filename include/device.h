#include "util.h"
#include <stdint.h>

typedef struct {
  uint64_t node_id;
  uint32_t git_sha;
  const char *device_name;
  const char *version_string;
  uint16_t vendor_id;
  uint16_t product_id;
  uint8_t hw_ver;
  uint8_t ver_major;
  uint8_t ver_minor;
  uint8_t ver_patch;
  uint8_t sn[16];
} DeviceCfg;

BmErr mac_address(uint8_t *mac, uint32_t size);
uint64_t node_id(void);
uint32_t git_sha(void);
uint16_t vendor_id(void);
uint16_t product_id(void);
uint8_t hardware_revision(void);
const char *device_name(void);
const char *version_string(void);
BmErr firmware_version(uint8_t *major, uint8_t *minor, uint8_t *patch);
BmErr serial_number(uint8_t *sn, uint32_t size);
BmErr device_init(DeviceCfg cfg);
