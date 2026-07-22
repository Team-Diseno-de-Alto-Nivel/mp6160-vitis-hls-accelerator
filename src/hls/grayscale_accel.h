#pragma once

#include <cstdint>

// Top-level HLS kernel: RGB->grayscale over AXI4 (m_axi burst read/write) with
// AXI4-Lite control. Implemented as a 3-stage HLS DATAFLOW pipeline that
// separates the I/O stages from the compute stage (see grayscale_accel.cpp).
// See docs/Enunciado.md and README.md "Memory Map" for the AXI4-Lite control
// register layout Vitis generates from this signature.
void grayscale_accel(
    const unsigned char *rgb_in,
    unsigned char       *gray_out,
    uint32_t             num_pixels
);
