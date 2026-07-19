#include "cpu.h"

CPU::CPU(sc_core::sc_module_name name,
         uint64_t disk_base_addr,
         uint64_t ram_base_addr,
         uint64_t accel_base_addr)
    : sc_core::sc_module(name)
    , init_socket("init_socket")
    , disk_base_addr(disk_base_addr)
    , ram_base_addr(ram_base_addr)
    , accel_base_addr(accel_base_addr) {
    SC_THREAD(run);
}

void CPU::run() {
    std::vector<uint8_t> input_image(INPUT_IMAGE_BYTES);
    std::vector<uint8_t> output_image(OUTPUT_IMAGE_BYTES);

    // Read RGB RAW image from the disk
    load_image_from_disk(input_image);

    // Store image in RAM
    store_image_to_ram(input_image);

    // Configure accelerator
    configure_accelerator(static_cast<uint32_t>(ram_base_addr + INPUT_IMAGE_RAM_ADDR),
                          static_cast<uint32_t>(ram_base_addr + OUTPUT_IMAGE_RAM_ADDR),
                          static_cast<uint32_t>(PIXEL_COUNT));

    // Start it and wait for completion
    start_accelerator();
    wait_accelerator_ready();

    // Read the processed image from RAM
    read_result_from_ram(output_image);

    // Save result to disk
    save_result_to_disk(output_image);

    sc_core::sc_stop();
}

void CPU::transport(uint64_t addr, unsigned char* data, unsigned int size, tlm::tlm_command cmd) {
    tlm::tlm_generic_payload trans;
    sc_core::sc_time delay = sc_core::sc_time(10, sc_core::SC_NS);

    trans.set_command(cmd);
    trans.set_address(addr);
    trans.set_data_ptr(data);
    trans.set_data_length(size);
    trans.set_streaming_width(size);
    trans.set_byte_enable_ptr(nullptr);
    trans.set_dmi_allowed(false);
    trans.set_response_status(tlm::TLM_INCOMPLETE_RESPONSE);

    init_socket->b_transport(trans, delay);

    if (trans.is_response_error()) {
        SC_REPORT_ERROR("CPU", "TLM transaction returned error response");
    }
}

void CPU::load_image_from_disk(std::vector<uint8_t>& buffer) {
    transport(disk_base_addr + DISK_INPUT_ADDR,
              buffer.data(),
              static_cast<unsigned int>(buffer.size()),
              tlm::TLM_READ_COMMAND);
}

void CPU::store_image_to_ram(const std::vector<uint8_t>& buffer) {
    transport(ram_base_addr + INPUT_IMAGE_RAM_ADDR,
              const_cast<unsigned char*>(buffer.data()),
              static_cast<unsigned int>(buffer.size()),
              tlm::TLM_WRITE_COMMAND);
}

void CPU::write_accel_register(uint64_t offset, uint32_t value) {
    transport(accel_base_addr + offset,
              reinterpret_cast<unsigned char*>(&value),
              sizeof(value),
              tlm::TLM_WRITE_COMMAND);
}

uint32_t CPU::read_accel_register(uint64_t offset) {
    uint32_t value = 0;
    transport(accel_base_addr + offset,
              reinterpret_cast<unsigned char*>(&value),
              sizeof(value),
              tlm::TLM_READ_COMMAND);
    return value;
}

void CPU::configure_accelerator(uint32_t src_addr, uint32_t dst_addr, uint32_t pixel_count) {
    write_accel_register(ACCEL_REG_SRC, src_addr);
    write_accel_register(ACCEL_REG_DST, dst_addr);
    write_accel_register(ACCEL_REG_NPIX, pixel_count);
}

void CPU::start_accelerator() {
    write_accel_register(ACCEL_REG_CTRL, ACCEL_CTRL_AP_START);
}

void CPU::wait_accelerator_ready() {
    const sc_core::sc_time poll_interval = sc_core::sc_time(100, sc_core::SC_NS);

    while ((read_accel_register(ACCEL_REG_CTRL) & ACCEL_CTRL_AP_DONE) == 0) {
        sc_core::wait(poll_interval);
    }
}

void CPU::read_result_from_ram(std::vector<uint8_t>& buffer) {
    transport(ram_base_addr + OUTPUT_IMAGE_RAM_ADDR,
              buffer.data(),
              static_cast<unsigned int>(buffer.size()),
              tlm::TLM_READ_COMMAND);
}

void CPU::save_result_to_disk(const std::vector<uint8_t>& buffer) {
    transport(disk_base_addr + DISK_OUTPUT_ADDR,
              const_cast<unsigned char*>(buffer.data()),
              static_cast<unsigned int>(buffer.size()),
              tlm::TLM_WRITE_COMMAND);
}
