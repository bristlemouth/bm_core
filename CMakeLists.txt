cmake_minimum_required(VERSION 3.29)

# Set the project name and specify that it's a C project
project(Bristlemouth VERSION 0.11 LANGUAGES C)

enable_testing()

# Set the C standard to C17 and enable strict compliance
set(CMAKE_C_STANDARD 17)
set(CMAKE_C_STANDARD_REQUIRED YES)
set(CMAKE_C_EXTENSIONS NO)

# Add include directories
include_directories(
    ${CMAKE_SOURCE_DIR}/include
)

# Add the source files
set(SOURCES
    src/packet.c
    src/service.c
    # Add more source files as needed
)

# 🤦‍♂️
# https://stackoverflow.com/questions/68145157/unrecognized-command-line-option-arch-did-you-mean-march
#set(APPLE FALSE)

if(NOT CMAKE_BUILD_TYPE STREQUAL "Test")
    # Specify the target architecture
    set(TARGET_ARCH "-mcpu=cortex-m33 -mthumb")  # Adjust according to your specific target
    set(TARGET_FPU "-mfpu=fpv4-sp-d16")  # Adjust or remove if your target does not have an FPU
    set(TARGET_FLOAT_ABI "-mfloat-abi=hard")  # Adjust or remove if your target does not use hardware floating-point

    # Define the compiler
    set(CMAKE_C_COMPILER "arm-none-eabi-gcc")

    # Set compiler flags for optimization and debugging
    set(COMMON_FLAGS "-Wall -Wextra -Wpedantic -Werror -Os ${TARGET_ARCH} ${TARGET_FPU} ${TARGET_FLOAT_ABI}")

    # Add any other flags you need, such as for floating-point hardware or other optimizations
    set(CMAKE_C_FLAGS_DEBUG "${COMMON_FLAGS} -g")
    set(CMAKE_C_FLAGS_RELEASE "${COMMON_FLAGS} -DNDEBUG")

    # Define the library target
    add_library(bmcore ${SOURCES})

    # Specify any additional include directories or linked libraries
    target_include_directories(bmcore PUBLIC
        ${CMAKE_SOURCE_DIR}/include
    )

    target_link_libraries(bmcore PUBLIC bmnetif)
else()
    add_subdirectory("test")
endif()
