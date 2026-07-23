# Vitis HLS 2024.1 batch flow for grayscale_accel.
# Usage: vitis_hls -f hls/scripts/run_hls.tcl
# Target: AMD Kria KV260 (K26 SOM, part xck26-sfvc784-2LV-c), 250 MHz.

# Absolute path to the input image, computed from this script's own location
# so it doesn't depend on Vitis's internal csim/cosim working directory depth.
set script_dir  [file dirname [file normalize [info script]]]
set repo_root   [file normalize [file join $script_dir .. .. ..]]
set input_image [file join $repo_root images input image.raw]

open_project -reset grayscale_accel_prj
set_top grayscale_accel

add_files       ../grayscale_accel.cpp -cflags "-std=c++17"
add_files -tb   ../tb/grayscale_accel_tb.cpp -cflags "-std=c++17"

open_solution -reset "solution1" -flow_target vivado

# AMD Kria KV260 carrier board -> Kria K26 SOM -> Zynq UltraScale+ XCK26
set_part {xck26-sfvc784-2LV-c}

create_clock -period 4 -name default
;# 250 MHz -> 4 ns period

config_interface -m_axi_addr64=0

# C simulation: run the pure-software model against the golden BT.601 check
csim_design -argv $input_image

# High-level synthesis: generates RTL + pipeline/dataflow reports
csynth_design

# C/RTL co-simulation: runs the same testbench through the generated RTL.
# NOTE: cosim itself still can't be verified end-to-end until the kernel body
# implements the pipeline (m_axi ports need depth= for cosim to run at all);
# the $input_image path above is directory-independent, so it isn't affected.
cosim_design -argv $input_image

# Export as a Vivado IP-catalog IP (drag into the KV260 block design)
export_design -rtl verilog -format ip_catalog -output grayscale_accel_hls_ip

exit
