.PHONY: all model program hls run-model gem5 run-vp check clean

# Path to a gem5 *source tree* (not just a gem5.opt on PATH): the SystemC
# accelerator is compiled into gem5 via scons EXTRAS, so gem5 must be rebuilt.
GEM5_ROOT ?= $(HOME)/gem5
GEM5_BIN  := $(GEM5_ROOT)/build/ARM/gem5.opt
EXTRAS    := $(CURDIR)/virtual-prototype
DRIVER    := $(CURDIR)/virtual-prototype/program/build/driver
NPROC     := $(shell nproc 2>/dev/null || sysctl -n hw.ncpu 2>/dev/null || echo 4)

all: model program

model:
	$(MAKE) -C virtual-prototype/systemc-model

run-model:
	$(MAKE) -C virtual-prototype/systemc-model run

program:
	$(MAKE) -C virtual-prototype/program

hls:
	cd hls/scripts && vitis_hls -f run_hls.tcl

# ── Virtual prototype (gem5 + SystemC/TLM) ────────────────────────────────────

gem5:
	@test -f "$(GEM5_ROOT)/SConstruct" || { \
	  echo "GEM5_ROOT=$(GEM5_ROOT) is not a gem5 source tree."; \
	  echo "Point it at one:  make gem5 GEM5_ROOT=/path/to/gem5"; exit 1; }
	cd "$(GEM5_ROOT)" && scons build/ARM/gem5.opt EXTRAS=$(EXTRAS) -j$(NPROC)

# Runs from the repo root on purpose: the driver resolves images/input/image.raw
# relative to its working directory, which gem5 SE inherits from here.
run-vp: program
	@test -x "$(GEM5_BIN)" || { \
	  echo "$(GEM5_BIN) not found — run 'make gem5' first."; exit 1; }
	"$(GEM5_BIN)" virtual-prototype/gem5/configs/kv260_arm64.py --binary="$(DRIVER)"

# Compares whatever the model and the virtual prototype produced against the
# BT.601 reference computed on the host from the same input image.
check:
	python3 scripts/check_output.py

clean:
	$(MAKE) -C virtual-prototype/systemc-model clean
	$(MAKE) -C virtual-prototype/program clean
