#include "accelerator.h"
#include "../utils/conversion.h"
#include <cstring>
#include <iomanip>
#include <sstream>

Accelerator::Accelerator(sc_core::sc_module_name name, uint64_t base_addr)
    : sc_module(name)
    , base_addr(base_addr) {
    target_socket.register_b_transport(this, &Accelerator::b_transport);
    SC_REPORT_INFO("Accelerator", "Initialized RGB→Grayscale accelerator");
}

uint8_t Accelerator::rgb_to_gray(uint8_t r, uint8_t g, uint8_t b) {
    return ::rgb_to_gray(r, g, b);
}

void Accelerator::b_transport(tlm::tlm_generic_payload& payload, sc_core::sc_time& delay) {
    const uint64_t addr = payload.get_address();
    const tlm::tlm_command cmd = payload.get_command();
    const unsigned int len = payload.get_data_length();
    unsigned char* data = payload.get_data_ptr();

    if (addr < base_addr || addr >= base_addr + ACCEL_REGION_SIZE) {
        SC_REPORT_ERROR("Accelerator", "Access outside the control region");
        payload.set_response_status(tlm::TLM_ADDRESS_ERROR_RESPONSE);
        return;
    }

    // AXI4-Lite is 32 bits wide; anything else is a programming error.
    if (len != sizeof(uint32_t)) {
        std::ostringstream oss;
        oss << "Invalid register access size: " << len << " (expected 4 bytes)";
        SC_REPORT_ERROR("Accelerator", oss.str().c_str());
        payload.set_response_status(tlm::TLM_BURST_ERROR_RESPONSE);
        return;
    }

    const uint64_t offset = addr - base_addr;

    if (cmd == tlm::TLM_READ_COMMAND) {
        uint32_t value = 0;
        switch (offset) {
            case ACCEL_REG_CTRL: value = ctrl_register; break;
            case ACCEL_REG_SRC:  value = src_register; break;
            case ACCEL_REG_DST:  value = dst_register; break;
            case ACCEL_REG_NPIX: value = npix_register; break;
            default:
                SC_REPORT_ERROR("Accelerator", "Read from unmapped register offset");
                payload.set_response_status(tlm::TLM_ADDRESS_ERROR_RESPONSE);
                return;
        }
        std::memcpy(data, &value, sizeof(value));
    } else if (cmd == tlm::TLM_WRITE_COMMAND) {
        uint32_t value = 0;
        std::memcpy(&value, data, sizeof(value));

        switch (offset) {
            case ACCEL_REG_CTRL:
                if (value & ACCEL_CTRL_AP_START) {
                    std::ostringstream oss;
                    oss << "Start — src=0x" << std::hex << std::setw(8) << std::setfill('0')
                        << src_register << ", dst=0x" << std::setw(8) << dst_register
                        << ", pixels=" << std::dec << npix_register;
                    SC_REPORT_INFO("Accelerator", oss.str().c_str());

                    ctrl_register &= ~ACCEL_CTRL_AP_DONE;
                    sc_core::sc_spawn(
                        sc_bind(&Accelerator::process_image, this,
                                src_register, dst_register, npix_register)
                    );
                }
                break;
            case ACCEL_REG_SRC:  src_register = value; break;
            case ACCEL_REG_DST:  dst_register = value; break;
            case ACCEL_REG_NPIX: npix_register = value; break;
            default:
                SC_REPORT_ERROR("Accelerator", "Write to unmapped register offset");
                payload.set_response_status(tlm::TLM_ADDRESS_ERROR_RESPONSE);
                return;
        }
    } else {
        SC_REPORT_ERROR("Accelerator", "Unsupported TLM command");
        payload.set_response_status(tlm::TLM_COMMAND_ERROR_RESPONSE);
        return;
    }

    payload.set_response_status(tlm::TLM_OK_RESPONSE);
    delay += sc_core::sc_time(10, sc_core::SC_NS);
}

void Accelerator::process_image(uint32_t src_addr, uint32_t dst_addr, uint32_t pixel_count) {
    SC_REPORT_INFO("Accelerator", "Processing started");
    const uint32_t step = (pixel_count >= 10) ? (pixel_count / 10) : 0;
    for (uint32_t i = 0; i < pixel_count; ++i) {
        if (step > 0 && i > 0 && i % step == 0) {
            std::ostringstream oss;
            oss << "Progress: " << (static_cast<uint64_t>(i) * 100 / pixel_count) << "%";
            SC_REPORT_INFO("Accelerator", oss.str().c_str());
        }
        const uint64_t src_offset = static_cast<uint64_t>(src_addr) + (static_cast<uint64_t>(i) * 3);
        const uint64_t dst_offset = static_cast<uint64_t>(dst_addr) + i;

        unsigned char rgb_data[3];
        tlm::tlm_generic_payload read_payload;
        read_payload.set_command(tlm::TLM_READ_COMMAND);
        read_payload.set_address(src_offset);
        read_payload.set_data_ptr(rgb_data);
        read_payload.set_data_length(3);
        sc_core::sc_time delay = sc_core::SC_ZERO_TIME;
        init_socket->b_transport(read_payload, delay);

        uint8_t r = rgb_data[0];
        uint8_t g = rgb_data[1];
        uint8_t b = rgb_data[2];
        uint8_t gray = rgb_to_gray(r, g, b);

        unsigned char gray_data[1] = {gray};
        tlm::tlm_generic_payload write_payload;
        write_payload.set_command(tlm::TLM_WRITE_COMMAND);
        write_payload.set_address(dst_offset);
        write_payload.set_data_ptr(gray_data);
        write_payload.set_data_length(1);
        init_socket->b_transport(write_payload, delay);
    }
    SC_REPORT_INFO("Accelerator", "Processing complete");
    ctrl_register |= ACCEL_CTRL_AP_DONE;
}
