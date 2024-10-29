# Usage

If you have your own sensor,
on your own microcontroller,
with your own firmware,
and you're wondering what would be required to make your device compatible with Bristlemouth,
read on!

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

# Include bm_core.cmake to add functions necessary to choose platform items
include(path/to/bm_core/cmake/bm_core.cmake)
setup_bm_ip_stack(LWIP "${BM_CORE_LWIP_INCLUDES}")
setup_bm_os(FREERTOS "${BM_CORE_FREERTOS_INCLUDES}")

# Add bm_core to the build
add_subdirectory(path/to/bm_core bmcore)

# Add directory path to bm_config.h here
target_include_directories(bmcore PRIVATE /path/to/config/directory)

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

Bristlemouth can be integrated into other build systems by...

<!--- Show how other build systems can utilize bm--->

## Running The Bristlemouth Stack
Once Bristlemouth has been added to the project,
it can be integrated within the project by configuring the following components:

<!--- TODO: Refine this--->
- Network Interface
- Bristlemouth

These items are configured by calling their initialization functions.
An example of this and how the Bristlemouth stack is to be serviced is shown below:

```C
Adin2111 adin;
Bristlemouth bm;

int main() {
    // other sensor setup
    adin->init(...pins...spi peripheral...);
    bm->init(adin, config);
    for(;;) {
        // other sensor code
        bm->update();
    }
}
```

<!--- Explain function parameters here?--->
