# Wrappers

There are several functions that the Bristlemouth library utilizes that are dependent on the platform architecture.
These functions are wrapped in the Bristlemouth library to provide a consistent interface across all platforms.
The following files contain the declarations of these functions that will need to be implemented for their platform:

## `bm_rtc.h`

All of the rtc wrapper functions are required by the BCMP time module.
They are used to set/get/sync time between Bristlemouth devices.

```{eval-rst}
.. cpp:function:: BmErr bm_rtc_set(const RtcTimeAndDate *time_and_date)

Sets the devices RTC time and date.

:param time_and_date: The time and date to set the RTC to.

:returns: BmOK if the time and date was set successfully, otherwise an error
```

```
```{eval-rst}
.. cpp:function:: BmErr bm_rtc_get(RtcTimeAndDate *time_and_date)

Gets the devices RTC time and date.

:param time_and_date: The pointer to set to the devices time and date

:returns: BmOK if the time and date was retrieved successfully, otherwise an error
```

```{eval-rst}
.. cpp:function:: uint64_t bm_rtc_get_micro_seconds(RtcTimeAndDate *time_and_date)

Gets the devices RTC time and date in microseconds.

:param time_and_date: The time and date to get the microseconds from

:returns: The time and date in microseconds
```

## `bm_dfu_generic.h`

The DFU module allows for the updating of the firmware on the device.
It is not required to be implemented, but if it is, the following functions will need to be implemented.

```{eval-rst}
.. cpp:function:: BmErr bm_dfu_client_set_confirmed(void)

Sets the device to the confirmed state.

:returns: BmOK if the device was set to the confirmed state successfully, BmEINVAL otherwise
```

```{eval-rst}
.. cpp:function:: BmErr bm_dfu_client_set_pending_and_reset(void)

Sets the device to the pending state and resets the device.

:returns: BmOK, however this function should not return as the device will reset
```

```{eval-rst}
.. cpp:function:: BmErr bm_dfu_client_fail_update_and_reset(void)

Fails the update and resets the device. This can trigger a revert to the previous firmware.

:returns: BmOK, however this function should not return as the device will reset
```

```{eval-rst}
.. cpp:function:: BmErr bm_dfu_client_flash_area_open(const void **flash_area)

Opens the flash area for writing.

:param flash_area: The flash area to open

:returns: BmOK if the flash area was opened successfully, BmEINVAL otherwise
```

```{eval-rst}
.. cpp:function:: BmErr bm_dfu_client_flash_area_close(const void *flash_area)

Closes the flash area.

:param flash_area: The flash area to close

:returns: BmOK if the flash area was closed successfully, BmEINVAL otherwise
```

```{eval-rst}
.. cpp:function:: BmErr bm_dfu_client_flash_area_write(const void *flash_area, uint32_t off, \
                                                       const void *src, uint32_t len)

Writes to the flash area.

:param flash_area: The flash area to write to
:param off: The offset to write to
:param src: The source to write from
:param len: The length to write

:returns: BmOK if the flash area was written to successfully, BmEINVAL otherwise
```

```{eval-rst}
.. cpp:function:: BmErr bm_dfu_client_flash_area_erase(const void *flash_area, uint32_t off, \
                                                       uint32_t len)

Erases the flash area.

:param flash_area: The flash area to erase
:param off: The offset to erase from
:param len: The length to erase

:returns: BmOK if the flash area was erased successfully, BmEINVAL otherwise
```

```{eval-rst}
.. cpp:function:: uint32_t bm_dfu_client_flash_area_get_size(const void *flash_area)

Gets the size of the flash area.

:param flash_area: The flash area to get the size of

:returns: The size of the flash area
```

```{eval-rst}
.. cpp:function:: bool bm_dfu_client_confirm_is_enabled(void)

Checks if the confirm is enabled.

:returns: True if the confirm is enabled, false otherwise
```

```{eval-rst}
.. cpp:function:: void bm_dfu_client_confirm_enable(bool en)

Enables or disables the confirm.

:param en: True to enable the confirm, false to disable
```

```{eval-rst}
.. cpp:function:: BmErr bm_dfu_host_get_chunk(uint32_t offset, uint8_t *buffer, size_t len, \
                                              uint32_t timeout_ms)

Gets a chunk of the firmware.

:param offset: The offset to get the chunk from
:param buffer: The buffer to store the chunk in
:param len: The length of the chunk
:param timeout_ms: The timeout in milliseconds to get the chunk

:returns: BmOK if the chunk was retrieved successfully, BmEINVAL otherwise
```

```{eval-rst}
.. cpp:function:: void bm_dfu_core_lpm_peripheral_active(void)

Tells the device that the peripheral is active and
prevents the device from going into low power mode
```

```{eval-rst}
.. cpp:function:: void bm_dfu_core_lpm_peripheral_inactive(void)

Tells the device that the peripheral is inactive and
allows the device to go into low power mode
```

## How to implement these:
These files will be included in your project when you include the Bristlemouth library.

You will need to implement these functions in your project to provide the functionality that the Bristlemouth library requires. For example this may look like:

### my_rtc.c
```c
#include "my_boards_rtc.h"
#include "bm_rtc.h"

// Implement the bm_rtc_get function using the my_boards_rtc_get function
bm_rtc_set(const RtcTimeAndDate *time_and_date) {
    set_my_boards_rtc(time_and_date);
}
```
