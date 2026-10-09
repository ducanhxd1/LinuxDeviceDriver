CC ?= cc
CFLAGS := -std=c11 -D_POSIX_C_SOURCE=200809L -Wall -Wextra -Wpedantic -Werror -O2
BUILD_DIR := build
TARGETS := $(BUILD_DIR)/supervisor $(BUILD_DIR)/worker

.PHONY: all clean test
all: $(TARGETS)
$(BUILD_DIR)/supervisor: src/supervisor.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) $< -o $@
$(BUILD_DIR)/worker: src/worker.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) $< -o $@
$(BUILD_DIR):
	mkdir -p $@
test: $(TARGETS)
	sh tests/test_supervisor.sh ./$(BUILD_DIR)/supervisor ./$(BUILD_DIR)/worker
clean:
	rm -f $(TARGETS)
	rmdir $(BUILD_DIR) 2>/dev/null || true

