KDIR ?= /lib/modules/$(shell uname -r)/build
PWD  := $(shell pwd)


obj-m += driver_gpio.o

CC := gcc
APP = gpio_app
SRC = user_main.c

all: driver user_app

driver:
	make -C $(KDIR) M=$(PWD) modules

user_app:
	$(CC) -o $(APP) $(SRC)

clean:
	make -C $(KDIR) M=$(PWD) clean
	rm -rf $(APP)

