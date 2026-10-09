.PHONY: all clean

CC := aarch64-linux-gnu-gcc

# Các cờ tối ưu và cảnh báo lỗi
CFLAGS := -O2 -Wall -Wextra

# Ten file output
TARGET := main_pi4

# File soure
SRCS := main.c

# Rules
all: $(TARGET)

$(TARGET): $(SRCS)
	$(CC) $(CFLAGS) $(SRCS) -o $(TARGET)

clean:
	rm -rf $(TARGET)
