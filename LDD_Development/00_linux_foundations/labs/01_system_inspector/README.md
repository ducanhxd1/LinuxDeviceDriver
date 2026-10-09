# Lab 01 — System Inspector

Lab mẫu hoàn chỉnh: đọc `/proc/uptime` và `/proc/meminfo` để trình bày uptime
và memory.

## Kiến thức luyện tập

- Chia chương trình thành header/source và CLI rõ ràng.
- Dùng `fopen`, `fgets`, `sscanf`, `fclose` và error handling.
- Build dependency bằng Makefile.
- Test output và error path bằng shell script.
- Quan sát syscall bằng `strace`.

## Build và chạy

```sh
make
./build/sysinspect uptime
./build/sysinspect memory
./build/sysinspect all
make test
```

Build với sanitizer: `make clean && make SANITIZE=1 && make test`.

## Bài mở rộng tự làm

Thêm command `kernel` đọc `/proc/version`, cập nhật usage và test. Không dùng
`system()`, `popen()` hoặc shell command trong code C.

