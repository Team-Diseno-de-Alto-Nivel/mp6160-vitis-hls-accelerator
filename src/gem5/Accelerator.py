"""SimObject declaration for the SystemC accelerator inside gem5.

Compiled into gem5 via `scons EXTRAS=<repo>/src ...` — see
../README.md. The C++ side wraps the very same Accelerator module the
standalone SystemC sim uses; nothing about the model is duplicated here.
"""

from m5.objects.SystemC import SystemC_ScModule
from m5.objects.Tlm import (
    TlmInitiatorSocket,
    TlmTargetSocket,
)
from m5.params import *
from m5.proxy import *


class GrayscaleAccelerator(SystemC_ScModule):
    type = "GrayscaleAccelerator"
    cxx_class = "AcceleratorWrapper"
    cxx_header = "gem5/accelerator_wrapper.hh"

    system = Param.System(Parent.any, "system")

    # Config registers, written by the ARM64 core through a Gem5ToTlmBridge32.
    tlm_target = TlmTargetSocket(32, "AXI4-Lite control registers")
    # The accelerator's own reads/writes of the image, sent back into gem5's
    # memory system through a TlmToGem5Bridge32.
    tlm_initiator = TlmInitiatorSocket(32, "DMA access to gem5 memory")
