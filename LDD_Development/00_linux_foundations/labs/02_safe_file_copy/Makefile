CC ?= cc
CFLAGS := -std=c11 -D_POSIX_C_SOURCE=200809L -Wall -Wextra -Wpedantic -Werror -O2
BUILD_DIR := build
TARGET := $(BUILD_DIR)/safecp

.PHONY: all clean test
all: $(TARGET)
$(TARGET): src/safecp.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) $< -o $@
$(BUILD_DIR):
	mkdir -p $@
test: $(TARGET)
	sh tests/test_safecp.sh ./$(TARGET)
clean:
	rm -f $(TARGET)
	rmdir $(BUILD_DIR) 2>/dev/null || true

