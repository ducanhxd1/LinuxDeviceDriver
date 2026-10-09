ifneq ($(KERNELRELEASE),)
obj-m := ldd_memory.o
ldd_memory-y := src/ldd_memory.o
else
KDIR ?= /lib/modules/$(shell uname -r)/build
.PHONY: all clean test
all:
	$(MAKE) -C $(KDIR) M=$(CURDIR) modules
	mkdir -p build && cp ldd_memory.ko build/
test: all
	modinfo -p build/ldd_memory.ko | grep -q '^buffer_size:'
	nm -u build/ldd_memory.ko | grep -q 'kfree'
	nm -u build/ldd_memory.ko | grep -q 'proc_create'
	@echo "PASS: memory/procfs module metadata"
clean:
	$(MAKE) -C $(KDIR) M=$(CURDIR) clean
	rm -rf build
endif

