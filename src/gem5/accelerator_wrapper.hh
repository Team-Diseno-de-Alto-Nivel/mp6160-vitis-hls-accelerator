#ifndef __GEM5_ACCELERATOR_WRAPPER_HH__
#define __GEM5_ACCELERATOR_WRAPPER_HH__

#include <tlm_utils/simple_initiator_socket.h>
#include <tlm_utils/simple_target_socket.h>

#include <string>

// Relative on purpose: gem5's generated param file does not inherit this module's
// SConscript CPPPATH, so a search-path include would build here and fail there.
#include "../model/accelerator/accelerator.h"
#include "base/types.hh"
#include "systemc/ext/core/sc_module_name.hh"
#include "systemc/ext/systemc"
#include "systemc/ext/tlm"
#include "systemc/tlm_port_wrapper.hh"

/*
 * Exposes the SystemC Accelerator — the same module the standalone sim in
 * ../model runs — to gem5 as a SimObject.
 *
 * The module itself needs no gem5 awareness: this wrapper only publishes its
 * two TLM sockets as gem5 ports, so the Python config can bind them to
 * Gem5ToTlmBridge32 (ARM64 core -> config registers) and TlmToGem5Bridge32
 * (accelerator -> gem5 memory).
 */
SC_MODULE(AcceleratorWrapper)
{
  public:
    AcceleratorWrapper(sc_core::sc_module_name name);

    gem5::Port &gem5_getPort(const std::string &if_name, int idx = -1) override;

  private:
    Accelerator accel;

    sc_gem5::TlmTargetWrapper<32> target_wrapper;
    sc_gem5::TlmInitiatorWrapper<32> initiator_wrapper;
};

#endif // __GEM5_ACCELERATOR_WRAPPER_HH__
