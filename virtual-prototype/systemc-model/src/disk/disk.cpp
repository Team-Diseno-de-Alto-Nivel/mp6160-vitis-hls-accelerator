#include "disk.h"
#include <cstring>
#include <iomanip>
#include <sstream>

Disk::Disk(sc_core::sc_module_name name)
    : sc_module(name) {
    target_socket.register_b_transport(this, &Disk::b_transport);
    SC_REPORT_INFO("Disk", "Initialized mock disk storage");
}

void Disk::validate_write(uint64_t addr, unsigned int size) {
    if (size == 0) {
        SC_REPORT_ERROR("Disk", "Zero-length write not allowed");
    }
}

void Disk::b_transport(tlm::tlm_generic_payload& payload, sc_core::sc_time& delay) {
    tlm::tlm_command cmd = payload.get_command();
    uint64_t addr = payload.get_address();
    unsigned int size = payload.get_data_length();
    unsigned char* data = payload.get_data_ptr();

    if (cmd == tlm::TLM_READ_COMMAND) {
        if (storage.find(addr) == storage.end()) {
            std::ostringstream oss;
            oss << "READ @ 0x" << std::hex << std::setw(8) << std::setfill('0') << addr
                << " not found (zero-filled)";
            SC_REPORT_WARNING("Disk", oss.str().c_str());
            std::memset(data, 0, size);
        } else {
            const auto& stored = storage[addr];
            unsigned int copy_size = std::min(size, (unsigned int)stored.size());
            std::memcpy(data, stored.data(), copy_size);
            if (copy_size < size) {
                std::memset(data + copy_size, 0, size - copy_size);
            }
        }
        std::ostringstream oss;
        oss << "READ  @ 0x" << std::hex << std::setw(8) << std::setfill('0') << addr
            << " " << std::dec << size << " bytes";
        SC_REPORT_INFO("Disk", oss.str().c_str());
    } else if (cmd == tlm::TLM_WRITE_COMMAND) {
        validate_write(addr, size);
        storage[addr] = std::vector<uint8_t>(data, data + size);
        std::ostringstream oss;
        oss << "WRITE @ 0x" << std::hex << std::setw(8) << std::setfill('0') << addr
            << " " << std::dec << size << " bytes";
        SC_REPORT_INFO("Disk", oss.str().c_str());
    } else {
        payload.set_response_status(tlm::TLM_COMMAND_ERROR_RESPONSE);
        SC_REPORT_ERROR("Disk", "Unsupported TLM command");
        return;
    }

    payload.set_response_status(tlm::TLM_OK_RESPONSE);
    delay += sc_core::sc_time(50, sc_core::SC_NS);
}
