ifneq ($(KERNELRELEASE),)
obj-m := ldd_hello.o
ldd_hello-y := src/ldd_hello.o
else
KDIR ?= /lib/modules/$(shell uname -r)/build
BUILD_DIR := build

.PHONY: all clean test
all:
	$(MAKE) -C $(KDIR) M=$(CURDIR) modules
	mkdir -p $(BUILD_DIR)
	cp ldd_hello.ko $(BUILD_DIR)/
test: all
	sh scripts/inspect.sh $(BUILD_DIR)/ldd_hello.ko
clean:
	$(MAKE) -C $(KDIR) M=$(CURDIR) clean
	rm -rf $(BUILD_DIR)
endif

