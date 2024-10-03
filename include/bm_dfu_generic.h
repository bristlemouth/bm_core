#include "util.h"
#include <stdint.h>

BmErr bm_dfu_set_confirmed(void);
BmErr bm_dfu_set_pending_and_reset(void);
BmErr bm_dfu_fail_update_and_reset(void);
BmErr bm_dfu_flash_area_open(const void **flash_area);
BmErr bm_dfu_flash_area_close(const void *flash_area);
BmErr bm_dfu_flash_area_write(const void *flash_area, uint32_t off, const void *src, uint32_t len);
BmErr bm_dfu_flash_area_erase(const void *flash_area, uint32_t off, uint32_t len);
uint32_t bm_dfu_flash_area_get_size(const void *flash_area);
bool bm_dfu_confirm_is_enabled(void);
void bm_dfu_confirm_enable(bool en);