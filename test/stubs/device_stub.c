#include "mock_device.h"

DEFINE_FAKE_VALUE_FUNC(BmErr, mac_address, uint8_t *, uint32_t);
DEFINE_FAKE_VALUE_FUNC(uint64_t, node_id);
DEFINE_FAKE_VALUE_FUNC(uint32_t, git_sha);
DEFINE_FAKE_VALUE_FUNC(uint16_t, vendor_id);
DEFINE_FAKE_VALUE_FUNC(uint16_t, product_id);
DEFINE_FAKE_VALUE_FUNC(uint8_t, hardware_revision);
DEFINE_FAKE_VALUE_FUNC(const char *, device_name);
DEFINE_FAKE_VALUE_FUNC(const char *, version_string);
DEFINE_FAKE_VALUE_FUNC(BmErr, firmware_version, uint8_t *, uint8_t *,
                       uint8_t *);
DEFINE_FAKE_VALUE_FUNC(BmErr, serial_number, uint8_t *, uint32_t);
DEFINE_FAKE_VALUE_FUNC(BmErr, device_init, DeviceCfg);
