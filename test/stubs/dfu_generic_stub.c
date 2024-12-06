#include "mock_bm_dfu_generic.h"

DEFINE_FAKE_VALUE_FUNC(BmErr, bm_dfu_client_set_confirmed);
DEFINE_FAKE_VALUE_FUNC(BmErr, bm_dfu_client_set_pending_and_reset);
DEFINE_FAKE_VALUE_FUNC(BmErr, bm_dfu_client_fail_update_and_reset);
DEFINE_FAKE_VALUE_FUNC(BmErr, bm_dfu_client_flash_area_open, const void **);
DEFINE_FAKE_VALUE_FUNC(BmErr, bm_dfu_client_flash_area_close, const void *);
DEFINE_FAKE_VALUE_FUNC(BmErr, bm_dfu_client_flash_area_write, const void *,
                       uint32_t, const void *, uint32_t);
DEFINE_FAKE_VALUE_FUNC(BmErr, bm_dfu_client_flash_area_erase, const void *,
                       uint32_t, uint32_t);
DEFINE_FAKE_VALUE_FUNC(uint32_t, bm_dfu_client_flash_area_get_size,
                       const void *);
DEFINE_FAKE_VALUE_FUNC(BmErr, bm_dfu_host_get_chunk, uint32_t, uint8_t *,
                       size_t, uint32_t);
DEFINE_FAKE_VOID_FUNC(bm_dfu_core_lpm_peripheral_active);
DEFINE_FAKE_VOID_FUNC(bm_dfu_core_lpm_peripheral_inactive);
