# Lab 01 — Hello Kernel Module

Lab mẫu cho toàn bộ workflow: source → Kbuild → inspect → load → observe → unload.

## Build và kiểm tra không cần root

```sh
make
make test
```

## Lifecycle trên máy thực hành

Review `src/ldd_hello.c` và `scripts/lifecycle.sh`, mở `sudo dmesg -w`, rồi chạy:

```sh
sudo ./scripts/lifecycle.sh
```

Script kiểm tra vermagic, từ chối đè module đã load và dùng trap để cleanup.

## Bài mở rộng

Thêm parameter chỉ-đọc `greeting` và log giá trị trong init. Test chuỗi rỗng và
chuỗi có khoảng trắng; quan sát representation dưới `/sys/module/ldd_hello/`.

