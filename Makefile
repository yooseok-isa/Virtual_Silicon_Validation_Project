# SPDX-License-Identifier: MIT

PROJECT_ROOT := $(CURDIR)
BUILD_DIR ?= $(PROJECT_ROOT)/build
CPP_BUILD_DIR ?= $(BUILD_DIR)/cpp-hal
CMAKE ?= cmake

.PHONY: all qemu-tests linux-driver cpp-hal vnpuctl clean help

all: qemu-tests linux-driver cpp-hal

qemu-tests:
	$(MAKE) -C qemu-device/tests

linux-driver:
	$(MAKE) -C linux-driver

cpp-hal:
	$(MAKE) -C cpp-hal

vnpuctl:
	$(MAKE) -C cpp-hal

clean:
	$(MAKE) -C qemu-device/tests clean
	$(MAKE) -C linux-driver clean
	$(MAKE) -C cpp-hal clean
	$(RM) -r $(CPP_BUILD_DIR)

help:
	@echo "Targets:"
	@echo "  make              Build QEMU tests, Linux driver, C++ HAL, and vnpuctl"
	@echo "  make qemu-tests   Build qemu-device/tests userspace tools"
	@echo "  make linux-driver Build linux-driver/vnpu-drv.ko and ioctl smoke test"
	@echo "  make cpp-hal      Build C++ HAL objects and tools/vnpuctl"
	@echo "  make vnpuctl      Build tools/vnpuctl"
	@echo "  make clean        Remove generated component build outputs"
