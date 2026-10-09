# Roadmap chương 01

## Phiên 1 — Kernel space và module lifecycle

- User/kernel boundary, privilege, kernel stack và lỗi kernel nghiêm trọng.
- `module_init`, `module_exit`, metadata và log level.
- Build, inspect và chạy Lab 01.

## Phiên 2 — Kbuild và ELF

- `obj-m`, external module build, `vermagic`, symbol table và relocation.
- Dùng `modinfo`, `readelf`, `nm`, `file`; phân biệt `insmod` và `modprobe`.

## Phiên 3 — Parameters, sysfs và logging

- Scalar/array parameter, permission, validation và runtime visibility.
- `pr_info`, `pr_warn`, `pr_err`, rate limiting và dynamic debug.
- Làm Lab 02.

## Phiên 4 — Memory và error paths

- `kmalloc/kfree`, GFP flags, size/overflow và ownership.
- Cleanup theo thứ tự ngược; `goto` trong kernel code.
- Làm Lab 03 và kiểm tra `/proc/ldd_memory`.

## Phiên 5 — Symbol và dependency

- `EXPORT_SYMBOL_GPL`, `Module.symvers`, reference và thứ tự unload.
- Làm Lab 04.

## Phiên 6 — Debug và phân tích

- Kernel log, taint, `lsmod`, `/sys/module`, trace và lỗi thường gặp.
- Hoàn thiện evidence, reflection và checklist.

