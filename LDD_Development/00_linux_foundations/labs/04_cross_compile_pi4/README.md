# Lab 04 — Cross-compile cho Raspberry Pi 4

Cross-compile `sysinspect` cho ARM64 và chạy trên Raspberry Pi 4.

```sh
make clean
make CC=aarch64-linux-gnu-gcc
file build/sysinspect
readelf -h build/sysinspect
```

Ghi `uname -m`, loader/dependencies và output trên target vào evidence. Giải
thích build/host/target, target triplet và lý do binary đúng kiến trúc vẫn có thể
không chạy do dynamic loader hoặc libc.

## Giải thích

- Build machine chạy compiler; host là hệ thống chạy toolchain; target ARM64
  chạy binary đầu ra.
- `aarch64-linux-gnu` biểu thị CPU AArch64, Linux và GNU user-space ABI.
- ELF đúng ARM64 vẫn có thể lỗi nếu dynamic interpreter không có trên target,
  hoặc libc không tương thích.

Chạy `make verify` để build riêng trong thư mục này và kiểm tra ELF.

