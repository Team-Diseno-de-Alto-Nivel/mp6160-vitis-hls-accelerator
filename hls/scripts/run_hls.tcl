# Vitis HLS 2024.1 batch flow for grayscale_accel.
# Usage: vitis_hls -f hls/scripts/run_hls.tcl
# Target: AMD Kria KV260 (K26 SOM, part xck26-sfvc784-2LV-c), 250 MHz.

open_project -reset grayscale_accel_prj
set_top grayscale_accel

add_files       ../src/grayscale_accel.cpp
add_files -tb   ../tb/grayscale_accel_tb.cpp

open_solution -reset "solution1"
set_part {xck26-sfvc784-2LV-c}
create_clock -period 4 -name default
;# 250 MHz -> 4 ns period

config_interface -m_axi_addr64=0

csim_design
csynth_design
cosim_design
export_design -rtl verilog -format ip_catalog

exit
