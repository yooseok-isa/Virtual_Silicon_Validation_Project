# SPDX-License-Identifier: MIT

CXX ?= g++
CXXFLAGS ?= -std=c++20 -Wall -Wextra -Wpedantic -g -O0
CPPFLAGS += -Iinclude -I../linux-driver/include
LDFLAGS ?=
LDLIBS ?=

TOOLS_DIR := ../tools
TARGET := $(TOOLS_DIR)/vnpuctl

HAL_SRC := src/vnpu-hal.cpp
CTL_SRC := src/vnpuctl.cpp
HAL_OBJ := src/vnpu-hal.o
CTL_OBJ := src/vnpuctl.o

.PHONY: all clean help

all: $(TARGET)

$(TARGET): $(HAL_OBJ) $(CTL_OBJ) | $(TOOLS_DIR)
	$(CXX) $(LDFLAGS) $^ $(LDLIBS) -o $@

$(HAL_OBJ): $(HAL_SRC) include/vnpu/vnpu-hal.hpp ../linux-driver/include/vnpu_uapi.h
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -c $< -o $@

$(CTL_OBJ): $(CTL_SRC) include/vnpu/vnpu-hal.hpp
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -c $< -o $@

$(TOOLS_DIR):
	mkdir -p $@

clean:
	$(RM) $(HAL_OBJ) $(CTL_OBJ) $(TARGET)

help:
	@echo "Targets:"
	@echo "  make        Build src/vnpu-hal.o, src/vnpuctl.o, and ../tools/vnpuctl"
	@echo "  make clean  Remove C++ HAL/CLI objects and binary"
