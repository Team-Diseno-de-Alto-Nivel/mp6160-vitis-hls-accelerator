cmake_minimum_required(VERSION 3.16)
project(systemc_image_processor CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

include(FetchContent)

# ── SystemC setup ─────────────────────────────────────────────────────────────
if(DEFINED ENV{SYSTEMC_HOME})
    message(STATUS "Using SystemC from SYSTEMC_HOME=$ENV{SYSTEMC_HOME}")
    set(SYSTEMC_HOME $ENV{SYSTEMC_HOME})
    include_directories(${SYSTEMC_HOME}/include)
    find_library(SYSTEMC_LIB
        NAMES systemc
        PATHS ${SYSTEMC_HOME}/lib ${SYSTEMC_HOME}/lib-linux64 ${SYSTEMC_HOME}/lib-macosx64
        REQUIRED)
else()
    message(STATUS "SYSTEMC_HOME not set — downloading SystemC 2.3.4 via FetchContent (requires internet)")
    set(DISABLE_ASYNC_UPDATES     ON  CACHE INTERNAL "")
    set(BUILD_SOURCE_DOCUMENTATION OFF CACHE INTERNAL "")
    set(CMAKE_CXX_STANDARD 17 CACHE STRING "C++ standard" FORCE)
    FetchContent_Declare(
        SystemC
        URL https://github.com/accellera-official/systemc/archive/refs/tags/2.3.4.tar.gz
        DOWNLOAD_EXTRACT_TIMESTAMP TRUE
    )
    FetchContent_MakeAvailable(SystemC)
    set(SYSTEMC_LIB SystemC::systemc)
endif()

# ── Shared configuration ──────────────────────────────────────────────────────
# ../common holds the memory map and image format, shared with the ARM64 driver
# and mirrored by the gem5 config. Absolute path so the sim finds the images
# regardless of the working directory it is launched from.
set(COMMON_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../common)
set(IMAGES_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../../images)

# ── Accelerator library ───────────────────────────────────────────────────────
# Kept separate so the gem5 SystemC build (scons EXTRAS=...) compiles the very
# same source instead of a copy.
add_library(accelerator_sc STATIC
    src/accelerator/accelerator.cpp
)
target_include_directories(accelerator_sc PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${COMMON_DIR}
)
target_link_libraries(accelerator_sc PUBLIC ${SYSTEMC_LIB})

# ── Main simulation executable ────────────────────────────────────────────────
add_executable(sim
    src/main.cpp
    src/cpu/cpu.cpp
    src/bus/bus.cpp
    src/ram/ram.cpp
    src/disk/disk.cpp
)
target_include_directories(sim PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src ${COMMON_DIR})
target_compile_definitions(sim PRIVATE IMAGES_DIR="${IMAGES_DIR}")
target_link_libraries(sim accelerator_sc ${SYSTEMC_LIB})
