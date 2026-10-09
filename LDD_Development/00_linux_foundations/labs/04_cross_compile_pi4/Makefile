CROSS_COMPILE ?= aarch64-linux-gnu-
CC := $(CROSS_COMPILE)gcc
CFLAGS := -std=c11 -Wall -Wextra -Wpedantic -Werror -O2
CPPFLAGS := -I../01_system_inspector/include
BUILD_DIR := build
TARGET := $(BUILD_DIR)/sysinspect-arm64
SOURCES := ../01_system_inspector/src/main.c ../01_system_inspector/src/system_info.c

.PHONY: all clean verify
all: $(TARGET)
$(TARGET): $(SOURCES) | $(BUILD_DIR)
	$(CC) $(CPPFLAGS) $(CFLAGS) $(SOURCES) -o $@
$(BUILD_DIR):
	mkdir -p $@
verify: $(TARGET)
	file $(TARGET)
	readelf -h $(TARGET) | grep -E 'Class:|Machine:'
	readelf -l $(TARGET) | grep 'interpreter'
clean:
	rm -f $(TARGET)
	rmdir $(BUILD_DIR) 2>/dev/null || true

