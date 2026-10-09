# Lab 02 — Safe File Copy

Tự viết `safecp SOURCE DESTINATION` bằng `open/read/write/close`.

## Yêu cầu

- Không dùng stdio, `system()` hoặc command `cp`.
- Xử lý partial write và `EINTR`.
- Không âm thầm ghi đè destination; thêm `--force` nếu muốn.
- Giữ permission cơ bản và cleanup trên mọi error path.

Test file rỗng, text, binary chứa byte 0, file lớn, source không tồn tại,
destination tồn tại và lỗi permission. So sánh bằng `cmp`.

## Build và chạy

```sh
make
./build/safecp source.bin destination.bin
./build/safecp --force source.bin destination.bin
make test
```

`--force` cho phép thay nội dung file thường đã tồn tại nhưng vẫn từ chối khi
source và destination cùng inode.

