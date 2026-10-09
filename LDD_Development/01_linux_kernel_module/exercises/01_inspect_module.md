# Bài tập inspect module

Sau khi build Lab 01, trả lời bằng output thực tế:

1. `file build/ldd_hello.ko` cho biết ELF class và kiến trúc nào?
2. `modinfo` cho biết license, author, description và vermagic gì?
3. Tìm symbol init/exit và các symbol chưa resolve bằng `nm`.
4. Dùng `readelf -S` tìm section chứa module metadata.
5. So sánh `uname -r` với trường vermagic.
6. Sau khi load, tìm module trong `lsmod` và `/sys/module/ldd_hello`.
7. Đọc `/proc/sys/kernel/tainted`; giải thích license ảnh hưởng thế nào.

Ghi command, output liên quan và diễn giải vào `labs/01_hello_module/evidence.md`.

