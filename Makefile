.PHONY: all model program hls run-model clean

all: model program

model:
	$(MAKE) -C virtual-prototype/systemc-model

run-model:
	$(MAKE) -C virtual-prototype/systemc-model run

program:
	$(MAKE) -C virtual-prototype/program

hls:
	cd hls/scripts && vitis_hls -f run_hls.tcl

clean:
	$(MAKE) -C virtual-prototype/systemc-model clean
	$(MAKE) -C virtual-prototype/program clean
