# ------------------------------------------------------------
# Project: sensor-stream-aggregator
# ------------------------------------------------------------

CC        := gcc
CFLAGS    := -Wall -Wextra -O2 -std=c11 -I./src
BUILD_DIR := build
SRC_DIR   := src

# Common source
COMMON_OBJ := $(BUILD_DIR)/client_common.o

# Default target
all: probe client1 client2

# Build directory rule
$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

# Compile common module
$(COMMON_OBJ): $(SRC_DIR)/client_common.c $(SRC_DIR)/client_common.h | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

# Probe application
PROBE_OBJ := $(BUILD_DIR)/probe.o
PROBE_BIN := $(BUILD_DIR)/probe

$(PROBE_OBJ): $(SRC_DIR)/probe.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

probe: $(PROBE_BIN)

$(PROBE_BIN): $(PROBE_OBJ) $(COMMON_OBJ)
	$(CC) $(CFLAGS) $(PROBE_OBJ) $(COMMON_OBJ) -o $@


# Client1 application
CLIENT1_OBJ := $(BUILD_DIR)/client1.o
CLIENT1_BIN := $(BUILD_DIR)/client1

$(CLIENT1_OBJ): $(SRC_DIR)/client1.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

client1: $(CLIENT1_BIN)

$(CLIENT1_BIN): $(CLIENT1_OBJ) $(COMMON_OBJ)
	$(CC) $(CFLAGS) $(CLIENT1_OBJ) $(COMMON_OBJ) -o $@

# Client2 Application
CLIENT2_OBJ := $(BUILD_DIR)/client2.o
CLIENT2_BIN := $(BUILD_DIR)/client2

$(CLIENT2_OBJ): $(SRC_DIR)/client2.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(CLIENT2_BIN): $(CLIENT2_OBJ) $(COMMON_OBJ)
	$(CC) $(CFLAGS) $(CLIENT2_OBJ) $(COMMON_OBJ) -o $@

client2: $(CLIENT2_BIN)

# Housekeeping
clean:
	rm -rf $(BUILD_DIR)

.PHONY: all clean probe client1 client2
