#pragma once

#include <systemc>
#include <tlm>
#include <tlm_utils/simple_target_socket.h>
#include <cstdint>
#include "memory_map.h"

/* Absolute path injected by CMake so the sim runs from any working directory. */
#ifndef IMAGES_DIR
#define IMAGES_DIR "images"
#endif

SC_MODULE(Disk) {
public:
    tlm_utils::simple_target_socket<Disk> target_socket;

    SC_CTOR(Disk);

private:
    void b_transport(tlm::tlm_generic_payload& payload, sc_core::sc_time& delay);

    static constexpr const char* INPUT_PATH  = IMAGES_DIR "/input/image.raw";
    static constexpr const char* OUTPUT_PATH = IMAGES_DIR "/output/output.raw";

    static constexpr uint64_t DISK_INPUT_ADDR  = DISK_BASE + DISK_IMG_IN;
    static constexpr uint64_t DISK_OUTPUT_ADDR = DISK_BASE + DISK_IMG_OUT;
};
