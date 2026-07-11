BUILD_DIR := build

.PHONY: all configure clean

all: configure
	$(MAKE) -C $(BUILD_DIR)

configure:
	cmake -S . -B $(BUILD_DIR) -DCMAKE_TOOLCHAIN_FILE=$(CURDIR)/aarch64-toolchain.cmake

clean:
	rm -rf $(BUILD_DIR)
