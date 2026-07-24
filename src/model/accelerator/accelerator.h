#pragma once

#include <systemc>
#include <tlm>
#include <tlm_utils/simple_target_socket.h>
#include <tlm_utils/simple_initiator_socket.h>
#include <cstdint>
// Relative, not search-path based: compiled into three builds (model CMake, gem5
// via EXTRAS, driver), and gem5's generated param file inherits no CPPPATH.
#include "../../common/memory_map.h"

SC_MODULE(Accelerator)
{
public:
    tlm_utils::simple_target_socket<Accelerator> target_socket;
    tlm_utils::simple_initiator_socket<Accelerator> init_socket;

    // AXI4-Lite control block, same register layout Vitis HLS generates for
    // grayscale_accel() — see memory_map.h. Driven both by the standalone CPU
    // module and, through gem5's TLM bridge, by the ARM64 driver.
    //
    // base_addr is a parameter because both the SystemC bus and the gem5
    // bridge deliver absolute addresses, but the peripheral may sit at a
    // different base in each topology.
    Accelerator(sc_core::sc_module_name name, uint64_t base_addr = ACCEL_BASE);

private:
    void b_transport(tlm::tlm_generic_payload & payload, sc_core::sc_time & delay);
    void process_image(uint32_t src_addr, uint32_t dst_addr, uint32_t pixel_count);
    uint8_t rgb_to_gray(uint8_t r, uint8_t g, uint8_t b);

    const uint64_t base_addr;

    uint32_t ctrl_register = 0;
    uint32_t src_register = 0;
    uint32_t dst_register = 0;
    uint32_t npix_register = 0;
};
