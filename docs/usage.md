# Usage

If you have your own sensor,
on your own microcontroller,
with your own firmware,
and you're wondering what would be required to make your device compatible with Bristlemouth,
read on!

## Setting Up The Repository

The repository can be added as a git submodule to a project utilizing the `git submodule add` command,
see [this](https://git-scm.com/book/en/v2/Git-Tools-Submodules) for more information.
Or placed into a project by any other method.
If cloned/added as a submodule to your project,
make sure you update all of the submodules in `bm_core` by `cd`'ing into `bm_core` and running:
`git submodule update --init`.

## Configuration Header File

In order to configure the bristlemouth stack,
the integrator must include a header file within their application with the name `bm_config.h`.
Within this file,
there are macros that must be defined by the integrator in order to build the stack.
A template of this file is found at `/bm_core/bm_config_template.h`,
which has all of the necessary macros that the user must define.
In order to ensure that this file is referenced properly in bm_core,
please view [Build System Integration](#build_system_integration).

(build_system_integration)=

## Build System Integration

Bristlemouth heavily relies on being built with CMake.
In order to integrate it within your own project,
CMake is highly recommended.
The following is an example
explaining how to integrate Bristlemouth into your own project utilizing CMake
with the following elements:

- LWIP as the IP stack
- FreeRTOS as the OS

```cmake
# Setup variables to hold include files here
set(BM_CORE_LWIP_INCLUDES
    path/to/includes/here
)
set(BM_CORE_FREERTOS_INCLUDES
    path/to/includes/here
)

# Add directory path to bm_config.h here
include_directories(/path/to/config/directory)

# Include bm_core.cmake to add functions necessary to choose platform items
include(path/to/bm_core/cmake/bm_core.cmake)
setup_bm_ip_stack(LWIP "${BM_CORE_LWIP_INCLUDES}")
setup_bm_os(FREERTOS "${BM_CORE_FREERTOS_INCLUDES}")

# Add bm_core to the build
add_subdirectory(path/to/bm_core bmcore)

# Link bm_core to the executable
target_link_libraries(${EXECUTABLE_NAME} bmcore)
```

The API:

- `setup_bm_ip_stack`
- `setup_bm_os`

Are necessary to configure bristlemouth to the platform you are using.
There are two arguments for these functions,

1. The selected platform
2. The include files (as a variable) from the integrators implementation of the selected platform

For IP stacks,
the following variables are supported (must be uppercase):

- [LWIP](https://savannah.nongnu.org/projects/lwip/)

For Operating Systems,
the following variables are supported (must be uppercase):

- [FREERTOS](https://www.freertos.org/)

When adding the include files,
these are the standard set of header files necessary for the platform.
For example,
in FreeRTOS this would be every general header file that FreeRTOS provides,
with the addition of the `FreeRTOSConfig.h` files location at a minimum.

<!--- TODO: show how other build systems can utilize bm--->

## Wrapper Functions

In order to run the Bristlemouth stack,
some functions must be implemented by the user to hook into the created library.
These hook functions are necessary for the following subsystems:

- Device Configuration
- Device Firmware Updates (DFU)
- RTC Time
- Network Driver

These functions are all declared in header files within `bm_core`,
but must be defined by the integrator in their own application.

Below is an example of how this would be implemented with an arbitrary header file named `some_bm_core_header.h`:

`some_bm_core_header.h`
```
#include <stdint.h>

void some_function(char *buf, uint32_t size);

```

`some_integrator_defined_source_file.c`
```
#include "some_bm_core_header.h"
#include <stdlib.h>


void some_function(char *buf, uint32_t size) {
  printf("This buffer states: %.*s\n", size, buf);
}
```

The following explains the necessary API for aforementioned subsystems.

### Configuration

Key-value configurations for the Bristlemouth stack are a paramount feature of the network stack.
[API](#configuration_api) is available get/set/delete keys locally on the device and over the network to other devices.
Configuration key-value pairs are expected to be stored persistently across boots of the device.
Meaning that the key-value pairs must be stored in some non-volatile memory (NVM).
This could be an external flash chipset,
flash on the processor itself,
EEPROM,
eMMC,
etc.
There are three configuration partitions that must be taken into account in the NVM:

1. User
2. System
3. Hardware

These partitions do not have a maximum size,
but should be adequately large enough to store the required key-value pairs expected to be set on the device.
Minimally,
each partition must be 4359 bytes (ensure it is aligned properly for the selected NVM method),
see type defined structure `ConfigPartition` in `bm_core/bcmp/configuration.h`.

The following shows how a 48kB EEPROM chip may be partitioned to hold the partitions mentioned above:

```{image} config_eeprom_example.png
:align: center
:width: 1000
:class: no-scaled-link
```


API that needs to be defined in the integrators application is found at `bm_core/bcmp/bm_configs_generic.h`.

These functions are described below:

```{eval-rst}
.. cpp:function:: bool bm_config_read(BmConfigPartition partition, \
                                      uint32_t offset, uint8_t *buffer, \
                                      size_t length, uint32_t timeout_ms);

  Read from a configuration partition and store read data into a buffer.
  This is called when initializing the system to load the partitions into RAM.
  Integrators are responsible for reading from the passed partition type within
  their own definition of this function.

  :param partition: Partition to read from (User, System, Hardware)
  :param offset: Offset from beginning of partition location to read from
  :param buffer: Buffer to store read data into
  :param length: Length of buffer
  :param timeout_ms: Timeout to read from partition in milliseconds

  :returns: true if able to read from partition properly, false otherwise
```

```{eval-rst}
.. cpp:function:: bool bm_config_write(BmConfigPartition partition, \
                                      uint32_t offset, uint8_t *buffer, \
                                      size_t length, uint32_t timeout_ms);

  Write to a configuration partition from provided buffer.
  This is only called when committing updated values to a specified partition.
  Integrators are responsible for writing to the passed partition type within
  their own definition of this function.

  :param partition: Partition to write to (User, System, Hardware)
  :param offset: Offset from beginning of partition location to write to
  :param buffer: Buffer to write to specific partition
  :param length: Length of buffer
  :param timeout_ms: Timeout to write to partition in milliseconds

  :returns: true if able to write to partition properly, false otherwise
```

```{eval-rst}
.. cpp:function:: void bm_config_reset(void)

  Reset the processor.
  This is called when the configuration values are saved to flash (after a
  commit).
```

### RTC

Several RTC functions are required by the BCMP time module.
They are used to set/get/sync time between Bristlemouth devices.

API that needs to be defined in the integrators application is found at `bm_core/bcmp/bm_rtc.h`.

These functions are described below:

```{eval-rst}
.. cpp:function:: BmErr bm_rtc_set(const RtcTimeAndDate *time_and_date)

  Sets the devices RTC time and date.

  :param time_and_date: The time and date to set the RTC to.

  :returns: BmOK if the time and date was set successfully, otherwise an error
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

### DFU

The DFU module allows for the updating of the firmware on the device.
It is not required to be implemented, but if it is, the following functions will need to be implemented.

API that needs to be defined in the integrators application is found at `bm_core/bcmp/bm_dfu_generic.h`.

These functions are described below:

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

## Running The Bristlemouth Stack

Once Bristlemouth has been added to the project build,
the code integration is very simple: a single call to `bristlemouth_init`.

```C
#include "bristlemouth.h"

void network_device_power_callback(bool on) {
    // Provide or cut off power to the network chip.
    // On the mote, we set pin PH1 high or low.
}

int main(void) {
    // ... your other setup code...

    BmErr err = bristlemouth_init(network_device_power_callback);
    if (err != BmOK) {
        // handle error appropriately for your system
    }

    while (true) {
        // ... your other ongoing code...
    }
}
```

This assumes the use of the Bristlemouth mote hardware with lwip and FreeRTOS.
Other future integrations are possible and even easy without many changes.
Some examples are other IP stacks,
other operating systems,
and other single-pair ethernet (SPE) chips.

If you need to perform such an integration,
you should post details on the
[forum](https://bristlemouth.discourse.group/).
We will help you very quickly!
