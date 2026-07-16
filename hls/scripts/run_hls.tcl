# Vitis HLS 2024.1 batch flow for grayscale_accel.
# Usage: vitis_hls -f hls/scripts/run_hls.tcl
# Target: AMD Kria KV260 (K26 SOM, part xck26-sfvc784-2LV-c), 250 MHz.

open_project -reset grayscale_accel_prj
set_top grayscale_accel

add_files       ../src/grayscale_accel.cpp -cflags "-std=c++14"
add_files -tb   ../tb/grayscale_accel_tb.cpp -cflags "-std=c++14"

open_solution -reset "solution1" -flow_target vivado

# AMD Kria KV260 carrier board -> Kria K26 SOM -> Zynq UltraScale+ XCK26
set_part {xck26-sfvc784-2LV-c}

create_clock -period 4 -name default
;# 250 MHz -> 4 ns period

config_interface -m_axi_addr64=0

# C simulation: run the pure-software model against the golden BT.601 check
csim_design -argv {../../../../../../images/input/input_1080p.raw}

# High-level synthesis: generates RTL + pipeline/dataflow reports
csynth_design

# C/RTL co-simulation: runs the same testbench through the generated RTL
# NOTE: -argv path depth assumed same as csim_design (verified for csim's
# build/ dir); re-verify once the kernel body (depth= on m_axi pragmas) is
# implemented and cosim_design can actually run to completion.
cosim_design -argv {../../../../../../images/input/input_1080p.raw}

# Export as a Vivado IP-catalog IP (drag into the KV260 block design)
export_design -rtl verilog -format ip_catalog -output grayscale_accel_hls_ip

exit
