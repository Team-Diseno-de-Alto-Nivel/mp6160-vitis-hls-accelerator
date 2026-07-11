#pragma once

#include <systemc>
#include <tlm>
#include <tlm_utils/simple_target_socket.h>
#include <cstdint>
#include <vector>
#include <map>

SC_MODULE(Disk) {
public:
    tlm_utils::simple_target_socket<Disk> target_socket;

    SC_CTOR(Disk);

private:
    // Mock storage: address → data mapping
    std::map<uint64_t, std::vector<uint8_t>> storage;

    void b_transport(tlm::tlm_generic_payload& payload, sc_core::sc_time& delay);
    void validate_write(uint64_t addr, unsigned int size);
};
