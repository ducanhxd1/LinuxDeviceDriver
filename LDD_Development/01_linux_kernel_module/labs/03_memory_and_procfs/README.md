# Lab 03 — Kernel Memory & procfs

Module cấp phát buffer bằng `kzalloc`, tạo `/proc/ldd_memory` bằng `proc_create`
và giải phóng theo thứ tự ngược. Parameter `buffer_size` giới hạn 16–4096 byte.

```sh
make && make test
sudo insmod build/ldd_memory.ko buffer_size=128
cat /proc/ldd_memory
sudo rmmod ldd_memory
test ! -e /proc/ldd_memory
```

Thử kích thước ngoài giới hạn và kiểm tra `/proc/ldd_memory` không tồn tại sau
init failure.

