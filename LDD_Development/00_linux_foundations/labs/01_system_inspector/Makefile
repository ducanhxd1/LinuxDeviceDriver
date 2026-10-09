CC ?= cc
CPPFLAGS := -Iinclude
CFLAGS := -std=c11 -Wall -Wextra -Wpedantic -Werror -O2
LDFLAGS :=
BUILD_DIR := build
TARGET := $(BUILD_DIR)/sysinspect
SOURCES := src/main.c src/system_info.c
OBJECTS := $(SOURCES:src/%.c=$(BUILD_DIR)/%.o)

ifeq ($(SANITIZE),1)
CFLAGS += -O1 -g -fsanitize=address,undefined
LDFLAGS += -fsanitize=address,undefined
endif

.PHONY: all clean test

all: $(TARGET)

$(TARGET): $(OBJECTS)
	$(CC) $(OBJECTS) $(LDFLAGS) -o $@

$(BUILD_DIR)/%.o: src/%.c include/system_info.h | $(BUILD_DIR)
	$(CC) $(CPPFLAGS) $(CFLAGS) -c $< -o $@

$(BUILD_DIR):
	mkdir -p $@

test: $(TARGET)
	sh tests/test_cli.sh ./$(TARGET)

clean:
	rm -f $(OBJECTS) $(TARGET)
	rmdir $(BUILD_DIR) 2>/dev/null || true

