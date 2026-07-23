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

# NOTE: the design file is synthesized in C++14, not C++17. Vitis HLS 2024.1's
# csynth front-end mis-parses the C++17-guarded blocks of gcc 8.3.0's STL headers
# (bundled with Vitis) and aborts with "[HLS 207-2916] C++ requires a type
# specifier for all declarations" pointing into bits/stl_pair.h. csim uses the
# full clang and is unaffected, so it passes under C++17 while csynth fails.
# The kernel needs nothing above C++14, so C++14 sidesteps the parser bug.
add_files       ../grayscale_accel.cpp -cflags "-std=c++14"
add_files -tb   ../tb/grayscale_accel_tb.cpp -cflags "-std=c++14"

open_solution -reset "solution1" -flow_target vivado

# AMD Kria KV260 carrier board -> Kria K26 SOM -> Zynq UltraScale+ XCK26
set_part {xck26-sfvc784-2LV-c}

create_clock -period 4 -name default
;# 250 MHz -> 4 ns period

config_interface -m_axi_addr64=0

# Lets the automatic port width resizing requested by max_widen_bitwidth in
# grayscale_accel.cpp actually kick in: without an alignment guarantee Vitis has
# to assume the base addresses are unaligned and falls back to narrow accesses.
# Safe by construction -- every buffer base in src/common/memory_map.h is 1 MiB
# aligned (RAM_IMG_IN 0x0, RAM_IMG_OUT 0x600000, GEM5_IMG_IN 0x80000000,
# GEM5_IMG_OUT 0x80600000), well beyond the 64 B asserted here.
config_interface -m_axi_alignment_byte_size=64

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
