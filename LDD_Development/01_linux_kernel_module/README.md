# 01 — Linux Kernel Module

Chương này chuyển từ user space sang kernel space. Trọng tâm không chỉ là tạo
file `.ko`, mà còn hiểu lifecycle, ownership tài nguyên, error path và cách quan
sát module mà không làm hệ thống mất ổn định.

## Kết quả đầu ra

- Giải thích đường đi từ source qua Kbuild đến module `.ko`.
- Build, inspect, load và unload module có kiểm soát.
- Dùng `pr_*`, module parameters, sysfs và dynamic debug.
- Cấp phát/giải phóng kernel memory với cleanup path đối xứng.
- Export/import symbol giữa module và hiểu dependency.
- Phân tích bằng `dmesg`, `modinfo`, `nm`, `readelf` và taint state.

## Cấu trúc

```text
labs/
├── 01_hello_module/
├── 02_module_parameters/
├── 03_memory_and_procfs/
└── 04_exported_symbols/
```

## Quy tắc an toàn

- Build và inspect không cần root; chỉ load/unload cần quyền phù hợp.
- Không load nếu kernel release dùng build khác `uname -r`.
- Mở `sudo dmesg -w` ở terminal khác trước khi load.
- Không unload cưỡng bức và không load code chưa review.

## Bắt đầu

```sh
cd labs/01_hello_module
make
make test
modinfo build/ldd_hello.ko
```

Sau khi review source, chạy `sudo ./scripts/lifecycle.sh`.

