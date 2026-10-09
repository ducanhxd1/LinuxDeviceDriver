# Lab 03 — Process & Signals

Viết parent tạo worker bằng `fork()`. Worker dùng `exec()` chạy chương trình
ghi heartbeat. Parent bắt SIGINT/SIGTERM, yêu cầu worker dừng và gọi `waitpid()`.

Tiêu chí: không zombie; handler chỉ dùng thao tác async-signal-safe; shutdown có
timeout; test normal exit, SIGTERM và exec failure.

## Build và chạy

```sh
make
./build/supervisor ./build/worker
# Nhấn Ctrl+C để shutdown sạch.
make test
```

Signal handler chỉ đặt cờ `sig_atomic_t`. `kill()` và `waitpid()` được thực hiện
trong flow chính.

