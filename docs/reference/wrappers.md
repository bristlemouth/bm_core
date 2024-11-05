# Wrappers

There are several functions that the Bristlemouth library utilizes that are dependent on the platform architecture.
These functions are wrapped in the Bristlemouth library to provide a consistent interface across all platforms.
The following files contain the declarations of these functions that will need to be implemented for their platform:

## `bm_rtc.h`
```c
BmErr bm_rtc_set(const RtcTimeAndDate *time_and_date);
BmErr bm_rtc_get(RtcTimeAndDate *time_and_date);
uint64_t bm_rtc_get_micro_seconds(RtcTimeAndDate *time_and_date);
```
All of the rtc wrapper functions are required by the BCMP time ?module?.
They are used to set/get/sync time between Bristlemouth devices.

## `bm_configs_generic.h`
```c
bool bm_config_read(BmConfigPartition partition, uint32_t offset,
                    uint8_t *buffer, size_t length, uint32_t timeout_ms);
bool bm_config_write(BmConfigPartition partition, uint32_t offset,
                     uint8_t *buffer, size_t length, uint32_t timeout_ms);
void bm_config_reset(void);
```
All  of the config wrapper functions are required by the BCMP config ?module?.
They are used to read/write/reset the configuration of the Bristlemouth device.
Since the configuration can be stored in a variety of locations,
the user must implement these functions to read/write the configuration from the correct location.
These funcitons need to be implemented by the user in the user code in a file such as `my_configs_wrapper.c`:
```c
#include "bm_configs_generic.h"
#include "my_config_driver.h"

bool bm_config_read(BmConfigPartition partition, uint32_t offset,
                    uint8_t *buffer, size_t length, uint32_t timeout_ms) {
    // Implement the function here
    bool rval = false;
    rval = read_from_my_config_driver(partition, offset, buffer, length);
    return rval;
}
```
Then the user needs to include this file into the build just like they would with their own code.

## `bm_dfu_generic.h`
```c
BmErr bm_dfu_client_set_confirmed(void);
BmErr bm_dfu_client_set_pending_and_reset(void);
BmErr bm_dfu_client_fail_update_and_reset(void);
BmErr bm_dfu_client_flash_area_open(const void **flash_area);
BmErr bm_dfu_client_flash_area_close(const void *flash_area);
BmErr bm_dfu_client_flash_area_write(const void *flash_area, uint32_t off,
                                     const void *src, uint32_t len);
BmErr bm_dfu_client_flash_area_erase(const void *flash_area, uint32_t off,
                                     uint32_t len);
uint32_t bm_dfu_client_flash_area_get_size(const void *flash_area);
bool bm_dfu_client_confirm_is_enabled(void);
void bm_dfu_client_confirm_enable(bool en);
BmErr bm_dfu_host_get_chunk(uint32_t offset, uint8_t *buffer, size_t len,
                            uint32_t timeouts);
void bm_dfu_core_lpm_peripheral_active(void);
void bm_dfu_core_lpm_peripheral_inactive(void);
```
The DFU module is not required for all applications, but if it is used, these most of these functions must be implemented.


