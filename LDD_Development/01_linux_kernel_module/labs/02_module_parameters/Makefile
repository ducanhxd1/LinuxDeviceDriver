ifneq ($(KERNELRELEASE),)
obj-m := ldd_params.o
ldd_params-y := src/ldd_params.o
else
KDIR ?= /lib/modules/$(shell uname -r)/build
.PHONY: all clean test
all:
	$(MAKE) -C $(KDIR) M=$(CURDIR) modules
	mkdir -p build && cp ldd_params.ko build/
test: all
	modinfo -p build/ldd_params.ko | grep -q '^repeat:'
	modinfo -p build/ldd_params.ko | grep -q '^enabled:'
	modinfo -p build/ldd_params.ko | grep -q '^message:'
	@echo "PASS: parameter metadata"
clean:
	$(MAKE) -C $(KDIR) M=$(CURDIR) clean
	rm -rf build
endif

