ifneq ($(KERNELRELEASE),)
obj-m := ldd_provider.o ldd_consumer.o
ldd_provider-y := src/ldd_provider.o
ldd_consumer-y := src/ldd_consumer.o
else
KDIR ?= /lib/modules/$(shell uname -r)/build
.PHONY: all clean test
all:
	$(MAKE) -C $(KDIR) M=$(CURDIR) modules
	mkdir -p build
	cp ldd_provider.ko ldd_consumer.ko build/
test: all
	modinfo -F depends build/ldd_consumer.ko | grep -q 'ldd_provider'
	nm -u build/ldd_consumer.ko | grep -q 'ldd_add'
	nm build/ldd_provider.ko | grep -q 'ldd_add'
	@echo "PASS: exported symbol dependency"
clean:
	$(MAKE) -C $(KDIR) M=$(CURDIR) clean
	rm -rf build
endif

