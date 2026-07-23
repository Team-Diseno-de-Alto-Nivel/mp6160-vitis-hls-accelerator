#include "bus.h"

Bus::Bus(sc_core::sc_module_name name)
    : sc_module(name) {
    target_socket.register_b_transport(this, &Bus::b_transport);
    target_socket_accel.register_b_transport(this, &Bus::b_transport_accel);
}

void Bus::b_transport(
    tlm::tlm_generic_payload& payload,
    sc_core::sc_time& delay)
{
    uint64_t addr = payload.get_address();

    std::cout
        << "[BUS] CPU acceso a 0x"
        << std::hex
        << addr
        << std::dec
        << std::endl;

    if (addr <= 0x03FFFFFFULL)
    {
        std::cout << "[BUS] -> RAM" << std::endl;
        init_socket_ram->b_transport(
            payload,
            delay
        );
        std::cout << "[BUS] <- RAM" << std::endl;
    }
    else if (addr >= 0x10000000ULL && addr < 0x20000000ULL)
    {
        std::cout << "[BUS] -> Accelerator" << std::endl;
        init_socket_accel->b_transport(
            payload,
            delay
        );
        std::cout << "[BUS] <- Accelerator" << std::endl;
    }
    else if (addr >= 0x20000000ULL && addr < 0x30000000ULL)
    {
        std::cout << "[BUS] -> Disk" << std::endl;
        init_socket_disk->b_transport(
            payload,
            delay
        );
        std::cout << "[BUS] <- Disk" << std::endl;
    }
    else
    {
        payload.set_response_status(
            tlm::TLM_ADDRESS_ERROR_RESPONSE);
    }

    delay += sc_core::sc_time(
        5,
        sc_core::SC_NS
    );
}


void Bus::b_transport_accel(
    tlm::tlm_generic_payload& payload,
    sc_core::sc_time& delay)
{
    uint64_t addr = payload.get_address();

    std::cout
        << "[BUS] Accelerator acceso a 0x"
        << std::hex
        << addr
        << std::dec
        << std::endl;

    if (addr <= 0x03FFFFFFULL)
    {
        init_socket_ram->b_transport(
            payload,
            delay
        );
    }
    else
    {
        payload.set_response_status(
            tlm::TLM_ADDRESS_ERROR_RESPONSE
        );
    }

    delay += sc_core::sc_time(
        5,
        sc_core::SC_NS
    );
}