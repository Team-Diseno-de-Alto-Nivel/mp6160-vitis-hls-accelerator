#pragma once

#include <systemc>
#include <tlm>
#include <tlm_utils/simple_target_socket.h>
#include <cstdint>
#include <vector>

SC_MODULE(RAM) {
public:
    tlm_utils::simple_target_socket<RAM> target_socket;

    SC_CTOR(RAM);

private:
    static constexpr uint64_t RAM_SIZE = 64 * 1024 * 1024; // 64 MB
    std::vector<uint8_t> memory;

    void b_transport(tlm::tlm_generic_payload& payload, sc_core::sc_time& delay);
    void validate_address_and_size(uint64_t addr, unsigned int size);
};
