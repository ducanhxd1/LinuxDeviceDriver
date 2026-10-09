# Checklist hoàn thành chương 00

## Command line và Linux

- [ ] Dùng path, glob, quoting, pipe và redirection đúng.
- [ ] Giải thích inode, permission, process, file descriptor và exit status.
- [ ] Tra cứu process trong `/proc/<pid>` và thiết bị trong `/dev`, `/sys`.
- [ ] Viết pipeline lọc log và kiểm tra chính xác exit status.

## Lập trình C

- [ ] Build sạch với `-Wall -Wextra -Wpedantic -Werror`.
- [ ] Mọi syscall được kiểm tra lỗi và tài nguyên được cleanup đúng ownership.
- [ ] Xử lý partial read/write và `EINTR` ở nơi cần thiết.
- [ ] Dùng `gdb`, sanitizers và `strace` để giải thích hành vi.

## Đầu ra

- [ ] System Inspector: build/test thành công và hiểu từng module.
- [ ] Safe File Copy: test file rỗng, lớn, binary và lỗi permission.
- [ ] Process & Signals: shutdown sạch bằng SIGINT/SIGTERM.
- [ ] Cross-compile Pi 4: binary ARM64 chạy trên target.
- [ ] Có reflection và evidence cho từng lab.

