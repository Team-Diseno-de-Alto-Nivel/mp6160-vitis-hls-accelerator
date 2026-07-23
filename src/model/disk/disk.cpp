#include "disk.h"
#include <filesystem>
#include <fstream>
#include <sstream>

Disk::Disk(sc_core::sc_module_name name)
    : sc_module(name) {
    target_socket.register_b_transport(this, &Disk::b_transport);
    SC_REPORT_INFO("Disk", "Initialized disk storage");
}

void Disk::b_transport(tlm::tlm_generic_payload& payload, sc_core::sc_time& delay) {
    const uint64_t addr = payload.get_address();
    unsigned char* ptr = payload.get_data_ptr();
    const unsigned int len = payload.get_data_length();

    if (payload.get_command() == tlm::TLM_READ_COMMAND) {
        if (addr != DISK_INPUT_ADDR) {
            SC_REPORT_ERROR("Disk", "Unknown read address");
            payload.set_response_status(tlm::TLM_ADDRESS_ERROR_RESPONSE);
            return;
        }

        std::ifstream file(INPUT_PATH, std::ios::binary);
        if (!file.is_open()) {
            SC_REPORT_ERROR("Disk", (std::string("Failed to open: ") + INPUT_PATH).c_str());
            payload.set_response_status(tlm::TLM_GENERIC_ERROR_RESPONSE);
            return;
        }

        file.read(reinterpret_cast<char*>(ptr), len);
        if (!file) {
            SC_REPORT_ERROR("Disk", "Error reading input image");
            payload.set_response_status(tlm::TLM_GENERIC_ERROR_RESPONSE);
            return;
        }

        std::ostringstream oss;
        oss << "READ  " << len << " bytes from " << INPUT_PATH;
        SC_REPORT_INFO("Disk", oss.str().c_str());
    } else if (payload.get_command() == tlm::TLM_WRITE_COMMAND) {
        if (addr != DISK_OUTPUT_ADDR) {
            SC_REPORT_ERROR("Disk", "Unknown write address");
            payload.set_response_status(tlm::TLM_ADDRESS_ERROR_RESPONSE);
            return;
        }

        std::filesystem::create_directories(std::filesystem::path(OUTPUT_PATH).parent_path());

        std::ofstream file(OUTPUT_PATH, std::ios::binary);
        if (!file.is_open()) {
            SC_REPORT_ERROR("Disk", (std::string("Failed to create: ") + OUTPUT_PATH).c_str());
            payload.set_response_status(tlm::TLM_GENERIC_ERROR_RESPONSE);
            return;
        }

        file.write(reinterpret_cast<char*>(ptr), len);
        if (!file) {
            SC_REPORT_ERROR("Disk", "Error writing output image");
            payload.set_response_status(tlm::TLM_GENERIC_ERROR_RESPONSE);
            return;
        }

        std::ostringstream oss;
        oss << "WRITE " << len << " bytes to " << OUTPUT_PATH;
        SC_REPORT_INFO("Disk", oss.str().c_str());
    } else {
        SC_REPORT_ERROR("Disk", "Unsupported TLM command");
        payload.set_response_status(tlm::TLM_COMMAND_ERROR_RESPONSE);
        return;
    }

    // Simulated disk latency: higher than RAM
    delay += sc_core::sc_time(100, sc_core::SC_NS);
    payload.set_response_status(tlm::TLM_OK_RESPONSE);
}
