#include "device.h"
#include "fff.h"

DECLARE_FAKE_VALUE_FUNC(BmErr, mac_address, uint8_t *, uint32_t);
DECLARE_FAKE_VALUE_FUNC(uint64_t, node_id);
DECLARE_FAKE_VALUE_FUNC(uint32_t, git_sha);
DECLARE_FAKE_VALUE_FUNC(uint16_t, vendor_id);
DECLARE_FAKE_VALUE_FUNC(uint16_t, product_id);
DECLARE_FAKE_VALUE_FUNC(uint8_t, hardware_revision);
DECLARE_FAKE_VALUE_FUNC(const char *, device_name);
DECLARE_FAKE_VALUE_FUNC(const char *, version_string);
DECLARE_FAKE_VALUE_FUNC(BmErr, firmware_version, uint8_t *, uint8_t *,
                        uint8_t *);
DECLARE_FAKE_VALUE_FUNC(BmErr, serial_number, uint8_t *, uint32_t);
DECLARE_FAKE_VALUE_FUNC(BmErr, device_init, DeviceCfg);
