CC ?= cc
CFLAGS ?= -std=c11 -Wall -Wextra -O2
LDFLAGS ?=

BIN_DIR := bin
SRC_DIR := src

.PHONY: all clean

all: $(BIN_DIR)/vnpu-mmio-smoke

$(BIN_DIR):
	mkdir -p $@

$(BIN_DIR)/vnpu-mmio-smoke: $(SRC_DIR)/vnpu-mmio-smoke.c | $(BIN_DIR)
	$(CC) $(CFLAGS) -o $@ $< $(LDFLAGS)

clean:
	rm -rf $(BIN_DIR)
