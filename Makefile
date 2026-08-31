# SPDX-License-Identifier: MIT

PROJECT_ROOT := $(patsubst %/,%,$(dir $(abspath $(lastword $(MAKEFILE_LIST)))))
BUILD_DIR ?= $(PROJECT_ROOT)/build
CPP_BUILD_DIR ?= $(BUILD_DIR)/cpp-hal
CPP_HAL_DIR := $(PROJECT_ROOT)/cpp-hal
CMAKE ?= cmake

.PHONY: all qemu-tests linux-driver cpp-hal vnpuctl mock-gtest clean help

all: qemu-tests linux-driver cpp-hal

qemu-tests:
	$(MAKE) -C qemu-device/tests

linux-driver:
	$(MAKE) -C linux-driver

cpp-hal:
	$(MAKE) -C $(CPP_HAL_DIR)

vnpuctl:
	$(MAKE) -C $(CPP_HAL_DIR)

mock-gtest:
	$(MAKE) -C $(CPP_HAL_DIR) mock-gtest

clean:
	$(MAKE) -C $(PROJECT_ROOT)/qemu-device/tests clean
	$(MAKE) -C $(PROJECT_ROOT)/linux-driver clean
	$(MAKE) -C $(CPP_HAL_DIR) clean
	$(RM) -r $(CPP_BUILD_DIR)

help:
	@echo "Targets:"
	@echo "  make              Build QEMU tests, Linux driver, C++ HAL, and vnpuctl"
	@echo "  make qemu-tests   Build qemu-device/tests userspace tools"
	@echo "  make linux-driver Build linux-driver/vnpu-drv.ko and ioctl smoke test"
	@echo "  make cpp-hal      Build C++ HAL objects and tools/vnpuctl"
	@echo "  make vnpuctl      Build tools/vnpuctl"
	@echo "  make mock-gtest   Build tools/mock-gtest"
	@echo "  make clean        Remove generated component build outputs"
