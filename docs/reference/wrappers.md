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
Some functions are optional, depending on your device requirements and are marked with `(optional)`.
These optional functions still need to be defined, but can be left empty if not required.

```{eval-rst}
.. cpp:function:: BmErr bm_dfu_client_set_confirmed(void)

```

```{eval-rst}
.. cpp:function:: BmErr bm_dfu_client_set_pending_and_reset(void)

```

```{eval-rst}
.. cpp:function:: BmErr bm_dfu_client_fail_update_and_reset(void)

```

```{eval-rst}
.. cpp:function:: BmErr bm_dfu_client_flash_area_open(const void **flash_area)

```

```{eval-rst}
.. cpp:function:: BmErr bm_dfu_client_flash_area_close(const void *flash_area)

```

```{eval-rst}
.. cpp:function:: BmErr bm_dfu_client_flash_area_write(const void *flash_area, uint32_t off, \
                                                       const void *src, uint32_t len)

```


```{eval-rst}
.. cpp:function:: BmErr bm_dfu_client_flash_area_erase(const void *flash_area, uint32_t off, \
                                                       uint32_t len)

```

```{eval-rst}
.. cpp:function:: uint32_t bm_dfu_client_flash_area_get_size(const void *flash_area)

```

```{eval-rst}
.. cpp:function:: bool bm_dfu_client_confirm_is_enabled(void)

```

```{eval-rst}
.. cpp:function:: void bm_dfu_client_confirm_enable(bool en)

```

```{eval-rst}
.. cpp:function:: BmErr bm_dfu_host_get_chunk(uint32_t offset, uint8_t *buffer, size_t len, \
                                              uint32_t timeouts)

```

```{eval-rst}
(optional)
.. cpp:function:: void bm_dfu_core_lpm_peripheral_active(void)

```

```{eval-rst}
(optional)
.. cpp:function:: void bm_dfu_core_lpm_peripheral_inactive(void)

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
