#pragma once

#include <cstdint>

// Top-level HLS kernel. See docs/Enunciado.md and README.md "Memory Map" for
// the AXI4-Lite control register layout Vitis generates from this signature.
// TODO: team fills in the RGB->grayscale pipeline (I/O stages + compute stage).
void grayscale_accel(
    const unsigned char *rgb_in,
    unsigned char       *gray_out,
    uint32_t             num_pixels
);
