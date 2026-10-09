CC = cc
CPPFLAGS = -D_POSIX_C_SOURCE=200809L -Iinclude -Iapp
CFLAGS = -std=c11 -O2 -g -Wall -Wextra -Wpedantic -Werror
.PHONY: all test clean overlay module
all: build/oled-lab
build/oled-lab: app/main.c app/graphics.c app/graphics.h include/protocol.h
	mkdir -p build
	$(CC) $(CPPFLAGS) $(CFLAGS) app/main.c app/graphics.c -o $@ $(LDFLAGS)
build/test-graphics: tests/graphics.c app/graphics.c app/graphics.h include/protocol.h
	mkdir -p build
	$(CC) $(CPPFLAGS) $(CFLAGS) tests/graphics.c app/graphics.c -o $@ $(LDFLAGS)
test: all build/test-graphics
	./build/test-graphics
	python3 tests/cli.py
overlay:
	mkdir -p build
	dtc -@ -I dts -O dtb -o build/oled-lab.dtbo dts/oled-lab-overlay.dts
module:
	@test -n "$(KDIR)" || (echo 'Supply KDIR for the target kernel'; exit 1)
	$(MAKE) -C "$(KDIR)" M="$(CURDIR)/driver" modules
clean:
	rm -rf build
