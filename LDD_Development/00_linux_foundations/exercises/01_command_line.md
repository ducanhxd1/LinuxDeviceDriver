# Bài tập command line

Tự làm trong thư mục tạm do `mktemp -d` tạo ra.

1. Tạo cây `src/`, `include/`, `build/` chỉ bằng một command.
2. Tạo ba file C, tìm dòng chứa `TODO` kèm số dòng.
3. Chuyển stdout và stderr vào hai file khác nhau.
4. Tạo hard link và symbolic link, so sánh inode bằng `ls -li`.
5. Chạy `sleep 300` ở background; xem `/proc/<pid>/status`, gửi SIGTERM.
6. Tìm năm process dùng nhiều memory nhất mà không dùng GUI.
7. Giải thích vì sao `cd /tmp | pwd` không đổi directory shell hiện tại.

## Tự kiểm tra

- Có quote mọi biến chứa path không?
- Command nào là shell builtin?
- Pipeline có che lỗi bên trái không? Thử `set -o pipefail`.

