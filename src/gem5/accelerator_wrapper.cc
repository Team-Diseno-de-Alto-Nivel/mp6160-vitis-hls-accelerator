#include "accelerator_wrapper.hh"

#include "params/GrayscaleAccelerator.hh"

AcceleratorWrapper::AcceleratorWrapper(sc_core::sc_module_name name)
    : sc_core::sc_module(name),
      accel("accelerator", ACCEL_BASE),
      target_wrapper(accel.target_socket, std::string(name) + ".tlm_target",
                     gem5::InvalidPortID),
      initiator_wrapper(accel.init_socket,
                        std::string(name) + ".tlm_initiator",
                        gem5::InvalidPortID)
{
}

gem5::Port &
AcceleratorWrapper::gem5_getPort(const std::string &if_name, int idx)
{
    if (if_name == "tlm_target")
        return target_wrapper;
    if (if_name == "tlm_initiator")
        return initiator_wrapper;
    return sc_core::sc_module::gem5_getPort(if_name, idx);
}

namespace gem5
{

/*
 * Every instantiable gem5 SimObject has to supply this: scons generates
 * param_GrayscaleAccelerator.cc from Accelerator.py, which binds
 * GrayscaleAcceleratorParams::create() into Python, but only declares it. Without
 * a definition everything compiles and the failure only shows up at link time as
 *   undefined reference to `gem5::GrayscaleAcceleratorParams::create() const'
 * The params struct lives in namespace gem5, so the definition has to as well.
 */
AcceleratorWrapper *
GrayscaleAcceleratorParams::create() const
{
    return new AcceleratorWrapper(name.c_str());
}

} // namespace gem5
