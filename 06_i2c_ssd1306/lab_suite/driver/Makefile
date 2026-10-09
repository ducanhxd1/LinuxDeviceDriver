obj-m += oled_lab.o
ccflags-y += -I$(src)/../include
# Detect const-qualified sysfs ABI from the build headers (including backports).
ccflags-y += $(shell grep -q 'const struct bin_attribute.*const.*bin_attrs' $(srctree)/include/linux/sysfs.h && echo -DOLED_CONST_SYSFS)
