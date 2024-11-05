# Wrappers

There are several functions that the Bristlemouth library utilizes that are dependent on the platform architecture.
These functions are wrapped in the Bristlemouth library to provide a consistent interface across all platforms.
The following files contain the declarations of these functions that a user will need to implement for their platform:

- `bm_rtc.h`
```c
BmErr bm_rtc_set(const RtcTimeAndDate *time_and_date);
BmErr bm_rtc_get(RtcTimeAndDate *time_and_date);
uint64_t bm_rtc_get_micro_seconds(RtcTimeAndDate *time_and_date);
```

- `bm_configs_generic.h`
```c
bool bm_config_read(BmConfigPartition partition, uint32_t offset,
                    uint8_t *buffer, size_t length, uint32_t timeout_ms);
bool bm_config_write(BmConfigPartition partition, uint32_t offset,
                     uint8_t *buffer, size_t length, uint32_t timeout_ms);
void bm_config_reset(void);
```

- `bm_dfu_generic.h`
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

