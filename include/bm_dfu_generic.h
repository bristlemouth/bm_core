#include "util.h"


// TODO - add the dfu functions that we will need... need to reference the dfu code to see what we need
// write
// read
// erase

// These are all the flash / mcu boot functions that are called in bcmp_dfu_client.cpp
// FLASH_AREA_IMAGE_SECONDARY --> this is called as an input into the flash_area_open function, so i think we can put it into the bm_generic_flash_area_open function
// boot_set_confirmed
// boot_set_pending
// flash_area_close
// flash_area_erase
// flash_area_get_size
// flash_area_open
// flash_area_write


BmErr bm_dfu_set_confirmed(void);
BmErr bm_dfu_set_pending_and_reset(void);
BmErr bm_dfu_fail_update_and_reset(void);
BmErr bm_dfu_flash_area_close(const void *flash_area);
BmErr bm_dfu_flash_area_erase(const void *flash_area, uint32_t off, uint32_t len);
BmErr bm_dfu_flash_area_get_size(const void *flash_area, uint32_t *size);
BmErr bm_dfu_flash_area_open(const void **flash_area);
BmErr bm_dfu_flash_area_write(const void *flash_area, uint32_t off, const void *src, uint32_t len);
