.PHONY: all model program hls hls-host prepare run-model gem5 run-vp check clean

# Path to a gem5 *source tree* (not just a gem5.opt on PATH): the SystemC
# accelerator is compiled into gem5 via scons EXTRAS, so gem5 must be rebuilt.
GEM5_ROOT ?= $(HOME)/gem5
GEM5_BIN  := $(GEM5_ROOT)/build/ARM/gem5.opt
EXTRAS    := $(CURDIR)/src
DRIVER    := $(CURDIR)/src/program/build/driver
NPROC     := $(shell nproc 2>/dev/null || sysctl -n hw.ncpu 2>/dev/null || echo 4)

all: model program

model:
	$(MAKE) -C src/model

# Generate images/input/image.raw from the tracked images/input/image.jpg.
# The .raw is git-ignored (the pipeline regenerates it), so every run-* target
# depends on this to guarantee the input exists on a fresh clone or in CI.
# Only regenerates when missing, so an existing image.raw needs no Pillow; to
# force a rebuild (e.g. after replacing image.jpg), delete images/input/image.raw.
prepare:
	@test -f images/input/image.raw || python3 scripts/prepare_input.py

run-model: prepare
	$(MAKE) -C src/model run

program:
	$(MAKE) -C src/program

hls:
	cd src/hls/scripts && vitis_hls -f run_hls.tcl

# Functional check of the HLS kernel on the host, WITHOUT Vitis: compiles the
# kernel (hls::stream falls back to a host shim) together with the co-simulation
# testbench and runs it against the real 1080p image, validating RGB->gray
# byte-for-byte against the BT.601 golden. Does NOT replace csynth/cosim
# (timing, AXI, RTL), which still need Vitis HLS.
hls-host: prepare
	mkdir -p src/hls/build
	g++ -std=c++17 -O2 -Wall -Wno-unknown-pragmas -Wno-unused-label \
	    src/hls/grayscale_accel.cpp src/hls/tb/grayscale_accel_tb.cpp \
	    -o src/hls/build/hls_host
	./src/hls/build/hls_host images/input/image.raw images/output/output_hls.raw

# ── Virtual prototype (gem5 + SystemC/TLM) ────────────────────────────────────

gem5:
	@test -f "$(GEM5_ROOT)/SConstruct" || { \
	  echo "GEM5_ROOT=$(GEM5_ROOT) is not a gem5 source tree."; \
	  echo "Point it at one:  make gem5 GEM5_ROOT=/path/to/gem5"; exit 1; }
	cd "$(GEM5_ROOT)" && scons build/ARM/gem5.opt EXTRAS=$(EXTRAS) -j$(NPROC)

# Runs from the repo root on purpose: the driver resolves images/input/image.raw
# relative to its working directory, which gem5 SE inherits from here.
run-vp: program prepare
	@test -x "$(GEM5_BIN)" || { \
	  echo "$(GEM5_BIN) not found — run 'make gem5' first."; exit 1; }
	"$(GEM5_BIN)" src/gem5/configs/kv260_arm64.py --binary="$(DRIVER)"

# Compares whatever the model and the virtual prototype produced against the
# BT.601 reference computed on the host from the same input image.
check:
	python3 scripts/check_output.py

clean:
	$(MAKE) -C src/model clean
	$(MAKE) -C src/program clean
