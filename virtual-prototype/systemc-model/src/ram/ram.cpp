#include "ram.h"
#include <cstring>
#include <iomanip>
#include <sstream>

RAM::RAM(sc_core::sc_module_name name)
    : sc_module(name)
    , memory(RAM_SIZE, 0) {
    target_socket.register_b_transport(this, &RAM::b_transport);
    SC_REPORT_INFO("RAM", "Initialized 64 MB memory");
}

void RAM::validate_address_and_size(uint64_t addr, unsigned int size) {
    if (addr + size > RAM_SIZE) {
        std::ostringstream oss;
        oss << "Address out of bounds: 0x" << std::hex << addr << " + " << std::dec << size << " bytes";
        SC_REPORT_ERROR("RAM", oss.str().c_str());
    }
}

void RAM::b_transport(tlm::tlm_generic_payload& payload, sc_core::sc_time& delay) {
    tlm::tlm_command cmd = payload.get_command();
    uint64_t addr = payload.get_address();
    unsigned int size = payload.get_data_length();
    unsigned char* data = payload.get_data_ptr();

    validate_address_and_size(addr, size);

    if (cmd == tlm::TLM_READ_COMMAND) {
        std::memcpy(data, &memory[addr], size);
        std::ostringstream oss;
        oss << "READ  @ 0x" << std::hex << std::setw(8) << std::setfill('0') << addr
            << " " << std::dec << size << " bytes";
        SC_REPORT_INFO("RAM", oss.str().c_str());
    } else if (cmd == tlm::TLM_WRITE_COMMAND) {
        std::memcpy(&memory[addr], data, size);
        std::ostringstream oss;
        oss << "WRITE @ 0x" << std::hex << std::setw(8) << std::setfill('0') << addr
            << " " << std::dec << size << " bytes";
        SC_REPORT_INFO("RAM", oss.str().c_str());
    } else {
        payload.set_response_status(tlm::TLM_COMMAND_ERROR_RESPONSE);
        SC_REPORT_ERROR("RAM", "Unsupported TLM command");
        return;
    }

    payload.set_response_status(tlm::TLM_OK_RESPONSE);
    delay += sc_core::sc_time(10, sc_core::SC_NS);
}
