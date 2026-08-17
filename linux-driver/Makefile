# SPDX-License-Identifier: GPL-2.0

obj-m += vnpu-drv.o

KDIR ?= $(HOME)/src/buildroot/output/build/linux-6.18.7
ARCH ?= x86_64
CROSS_COMPILE ?= $(HOME)/src/buildroot/output/host/bin/x86_64-buildroot-linux-gnu-
USER_CC ?= $(if $(CROSS_COMPILE),$(CROSS_COMPILE)gcc,gcc)
USER_CFLAGS ?= -Wall -Wextra -g -O0
PWD := $(CURDIR)

SMOKE_SRC := tests/vnpu-ioctl-smoke.c
SMOKE_BIN := tests/vnpu-ioctl-smoke

.PHONY: all modules tests clean help check-kdir check-toolchain

all: modules tests

modules: check-kdir check-toolchain
	$(MAKE) -C $(KDIR) M=$(PWD) ARCH=$(ARCH) CROSS_COMPILE=$(CROSS_COMPILE) modules

tests: $(SMOKE_BIN)

$(SMOKE_BIN): $(SMOKE_SRC) vnpu-uapi.h
	$(USER_CC) $(USER_CFLAGS) -o $@ $<

clean: check-kdir
	$(MAKE) -C $(KDIR) M=$(PWD) ARCH=$(ARCH) CROSS_COMPILE=$(CROSS_COMPILE) clean
	$(RM) $(SMOKE_BIN)

check-kdir:
	@test -f "$(KDIR)/Makefile" || { \
		echo "error: KDIR does not point to a kernel build tree: $(KDIR)"; \
		echo "usage: make KDIR=/path/to/kernel/build"; \
		exit 1; \
	}

check-toolchain:
	@if [ -n "$(CROSS_COMPILE)" ] && [ ! -x "$(CROSS_COMPILE)gcc" ]; then \
		echo "error: CROSS_COMPILE gcc not found: $(CROSS_COMPILE)gcc"; \
		echo "usage: make CROSS_COMPILE=/path/to/toolchain-prefix-"; \
		echo "       make CROSS_COMPILE=   # use host compiler"; \
		exit 1; \
	fi

help:
	@echo "Targets:"
	@echo "  make              Build vnpu-drv.ko and tests/vnpu-ioctl-smoke"
	@echo "  make modules      Build vnpu-drv.ko as an out-of-tree kernel module"
	@echo "  make tests        Build userspace ioctl smoke test"
	@echo "  make clean        Remove generated module and test build files"
	@echo ""
	@echo "Variables:"
	@echo "  KDIR=<path>       Kernel build tree, default: $(KDIR)"
	@echo "  ARCH=<arch>       Kernel architecture, default: $(ARCH)"
	@echo "  CROSS_COMPILE=    Toolchain prefix, default: $(CROSS_COMPILE)"
	@echo "  USER_CC=<cc>      Userspace test compiler, default: $(USER_CC)"
