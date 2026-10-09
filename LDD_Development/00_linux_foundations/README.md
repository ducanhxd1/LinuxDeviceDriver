# 00 — Linux Foundations & Professional User-space C

Chương này là mẫu chuẩn cho cách chúng ta học các chương tiếp theo. Ta không chỉ
đọc lý thuyết: mỗi khái niệm phải được quan sát bằng command line, dùng trong mã
C, kiểm tra bằng test và ghi lại evidence.

## Kết quả đầu ra

- Điều hướng và thao tác filesystem an toàn bằng shell.
- Giải thích process, file descriptor, permission, signal và virtual filesystem.
- Dùng compiler, linker, Makefile, debugger và system-call tracer.
- Viết ứng dụng C có CLI, error handling, cleanup và test tự động.
- Cross-compile một ứng dụng cho Raspberry Pi 4 và xác minh binary.
- Thu thập bằng chứng để phân tích lỗi thay vì sửa theo phỏng đoán.

## Cấu trúc

```text
00_linux_foundations/
├── README.md
├── ROADMAP.md
├── CHECKLIST.md
├── notes/
├── exercises/
└── labs/
    ├── 01_system_inspector/
    ├── 02_safe_file_copy/
    ├── 03_process_signals/
    └── 04_cross_compile_pi4/
```

## Cách sử dụng chương mẫu

1. Đọc `notes/01_linux_and_shell.md`.
2. Chạy lại từng lệnh, không chỉ copy nguyên khối.
3. Làm `exercises/01_command_line.md`.
4. Học quy trình đầy đủ từ `labs/01_system_inspector`.
5. Tự làm các lab 02–04 theo cấu trúc: yêu cầu → thiết kế → code → test → evidence → reflection.
6. Cập nhật `CHECKLIST.md` và `../PROGRESS.md`.

## Phiên học đầu tiên

```sh
cd LDD_Development/00_linux_foundations/labs/01_system_inspector
make
make test
./build/sysinspect all
strace -e trace=openat,read,write,close ./build/sysinspect uptime
```

Không cần quyền root cho lab mẫu.
