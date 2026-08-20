CXX ?= g++

TOOL_DIR := $(abspath $(dir $(lastword $(MAKEFILE_LIST))))
CPP_HAL_DIR := $(abspath $(TOOL_DIR)/..)
PROJECT_DIR := $(abspath $(CPP_HAL_DIR)/..)
BUILD_DIR ?= $(TOOL_DIR)/build
UAPI_DIR ?= $(PROJECT_DIR)/linux-driver

TARGET ?= $(BUILD_DIR)/vnpuctl
SOURCES := $(TOOL_DIR)/vnpuctl.cpp $(CPP_HAL_DIR)/src/vnpu-hal.cpp
OBJECTS := $(patsubst $(TOOL_DIR)/%.cpp,$(BUILD_DIR)/tools/%.o,$(filter $(TOOL_DIR)/%.cpp,$(SOURCES))) \
           $(patsubst $(CPP_HAL_DIR)/src/%.cpp,$(BUILD_DIR)/src/%.o,$(filter $(CPP_HAL_DIR)/src/%.cpp,$(SOURCES)))

CPPFLAGS += -I$(BUILD_DIR) -I$(CPP_HAL_DIR) -I$(UAPI_DIR)
CXXFLAGS += -std=c++20 -Wall -Wextra -Wpedantic -g -O0
LDFLAGS +=
LDLIBS +=

.PHONY: all objects syntax clean print-config

all: $(TARGET)

$(TARGET): $(OBJECTS)
	@mkdir -p $(@D)
	$(CXX) $(LDFLAGS) $^ $(LDLIBS) -o $@

objects: $(OBJECTS)

syntax: $(BUILD_DIR)/include/vnpu-hal
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -fsyntax-only $(TOOL_DIR)/vnpuctl.cpp

$(BUILD_DIR)/tools/%.o: $(TOOL_DIR)/%.cpp $(BUILD_DIR)/include/vnpu-hal
	@mkdir -p $(@D)
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -c $< -o $@

$(BUILD_DIR)/src/%.o: $(CPP_HAL_DIR)/src/%.cpp $(BUILD_DIR)/include/vnpu-hal
	@mkdir -p $(@D)
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -c $< -o $@

$(BUILD_DIR)/include/vnpu-hal: $(CPP_HAL_DIR)/include/vnpu-hal.hpp
	@mkdir -p $(@D)
	cp $< $@

print-config:
	@echo "CXX=$(CXX)"
	@echo "CPPFLAGS=$(CPPFLAGS)"
	@echo "CXXFLAGS=$(CXXFLAGS)"
	@echo "UAPI_DIR=$(UAPI_DIR)"
	@echo "TARGET=$(TARGET)"

clean:
	$(RM) -r $(BUILD_DIR)
