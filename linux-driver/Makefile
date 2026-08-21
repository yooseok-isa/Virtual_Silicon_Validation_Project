# SPDX-License-Identifier: GPL-2.0

obj-m += vnpu-drv.o
vnpu-drv-y := src/vnpu_drv.o
ccflags-y += -I$(src)/include

KDIR ?= $(HOME)/src/buildroot/output/build/linux-6.18.7
ARCH ?= x86_64
CROSS_COMPILE ?= $(HOME)/src/buildroot/output/host/bin/x86_64-buildroot-linux-gnu-
USER_CC ?= $(if $(CROSS_COMPILE),$(CROSS_COMPILE)gcc,gcc)
USER_CFLAGS ?= -Wall -Wextra -g -O0
PWD := $(CURDIR)

SMOKE_SRC := tests/vnpu-ioctl-smoke.c
SMOKE_BIN := tests/vnpu-ioctl-smoke

.PHONY: all modules stage-module-artifacts tests clean help check-kdir check-toolchain

all: modules tests

modules: check-kdir check-toolchain
	$(MAKE) -C $(KDIR) M=$(PWD) ARCH=$(ARCH) CROSS_COMPILE=$(CROSS_COMPILE) modules
	$(MAKE) -C $(CURDIR) stage-module-artifacts

stage-module-artifacts:
	@for artifact in vnpu-drv.o vnpu-drv.mod vnpu-drv.mod.c vnpu-drv.mod.o; do \
		if [ -e "$$artifact" ]; then mv -f "$$artifact" src/; fi; \
	done

tests: $(SMOKE_BIN)

$(SMOKE_BIN): $(SMOKE_SRC) include/vnpu_uapi.h
	$(USER_CC) $(USER_CFLAGS) -Iinclude -o $@ $<

clean: check-kdir
	$(MAKE) -C $(KDIR) M=$(PWD) ARCH=$(ARCH) CROSS_COMPILE=$(CROSS_COMPILE) clean
	$(RM) $(SMOKE_BIN)
	$(RM) vnpu-drv.ko vnpu-drv.o vnpu-drv.mod vnpu-drv.mod.c vnpu-drv.mod.o
	$(RM) .vnpu-drv.ko.cmd .vnpu-drv.o.cmd .vnpu-drv.mod.cmd .vnpu-drv.mod.o.cmd
	$(RM) src/vnpu-drv.o src/vnpu-drv.mod src/vnpu-drv.mod.c src/vnpu-drv.mod.o
	$(RM) vnpu_drv.ko vnpu_drv.o vnpu_drv.mod vnpu_drv.mod.c vnpu_drv.mod.o
	$(RM) .vnpu_drv.ko.cmd .vnpu_drv.o.cmd .vnpu_drv.mod.cmd .vnpu_drv.mod.o.cmd

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
