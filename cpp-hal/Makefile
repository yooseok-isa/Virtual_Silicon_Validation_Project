# SPDX-License-Identifier: MIT

CXX ?= g++
CXXFLAGS ?= -std=c++20 -Wall -Wextra -Wpedantic -g -O0
CPP_HAL_DIR := $(patsubst %/,%,$(dir $(abspath $(lastword $(MAKEFILE_LIST)))))
PROJECT_ROOT := $(abspath $(CPP_HAL_DIR)/..)

CPPFLAGS += -I$(CPP_HAL_DIR)/include -I$(PROJECT_ROOT)/linux-driver/include
LDFLAGS ?=
LDLIBS ?=
GTEST_LDLIBS ?= -lgtest_main -lgtest -pthread

TOOLS_DIR := $(PROJECT_ROOT)/tools
TARGET := $(TOOLS_DIR)/vnpuctl
TEST_TARGET := $(TOOLS_DIR)/mock-gtest

HAL_SRC := $(CPP_HAL_DIR)/src/vnpu-hal.cpp
CTL_SRC := $(CPP_HAL_DIR)/src/vnpuctl.cpp
TEST_SRC := $(CPP_HAL_DIR)/tests/mock-gtest.cpp
HAL_OBJ := $(CPP_HAL_DIR)/src/vnpu-hal.o
CTL_OBJ := $(CPP_HAL_DIR)/src/vnpuctl.o
TEST_OBJ := $(CPP_HAL_DIR)/tests/mock-gtest.o

.PHONY: all mock-gtest test clean help

all: $(TARGET)

mock-gtest: $(TEST_TARGET)

test: $(TEST_TARGET)
	$(TEST_TARGET)

$(TARGET): $(HAL_OBJ) $(CTL_OBJ) | $(TOOLS_DIR)
	$(CXX) $(LDFLAGS) $^ $(LDLIBS) -o $@

$(TEST_TARGET): $(HAL_OBJ) $(TEST_OBJ) | $(TOOLS_DIR)
	$(CXX) $(LDFLAGS) $^ $(LDLIBS) $(GTEST_LDLIBS) -o $@

$(HAL_OBJ): $(HAL_SRC) $(CPP_HAL_DIR)/include/vnpu/vnpu-hal.hpp $(PROJECT_ROOT)/linux-driver/include/vnpu_uapi.h
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -c $< -o $@

$(CTL_OBJ): $(CTL_SRC) $(CPP_HAL_DIR)/include/vnpu/vnpu-hal.hpp
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -c $< -o $@

$(TEST_OBJ): $(TEST_SRC) $(CPP_HAL_DIR)/include/vnpu/vnpu-hal.hpp
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -c $< -o $@

$(TOOLS_DIR):
	mkdir -p $@

clean:
	$(RM) $(HAL_OBJ) $(CTL_OBJ) $(TEST_OBJ) $(TARGET) $(TEST_TARGET)

help:
	@echo "Targets:"
	@echo "  make        Build C++ HAL objects and $(TOOLS_DIR)/vnpuctl"
	@echo "  make mock-gtest Build $(TOOLS_DIR)/mock-gtest"
	@echo "  make test   Build and run $(TOOLS_DIR)/mock-gtest"
	@echo "  make clean  Remove C++ HAL/CLI objects and binary"
