cmake_minimum_required(VERSION 3.16)
project(accel_driver C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

# gem5 SE mode has no /dev/mem, so the driver dereferences physical addresses
# that the gem5 config identity-maps into its page table. Turn OFF to build for
# the real KV260, where the buffers are reached by mmap()ing /dev/mem as root.
option(GEM5_SE "Build for gem5 SE mode instead of real hardware" ON)

add_executable(driver
    main.c
)

# ../common holds the memory map and image format, shared with the SystemC model.
target_include_directories(driver PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../common)

if(GEM5_SE)
    target_compile_definitions(driver PRIVATE GEM5_SE)
endif()
