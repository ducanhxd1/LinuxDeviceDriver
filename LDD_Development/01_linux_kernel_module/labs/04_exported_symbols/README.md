# Lab 04 — Exported Symbols

`ldd_provider` export hàm GPL `ldd_add()`. `ldd_consumer` gọi hàm đó trong init.
Kbuild chung đảm bảo consumer nhận CRC/symbol metadata từ provider.

```sh
make && make test
sudo insmod build/ldd_provider.ko
sudo insmod build/ldd_consumer.ko left=20 right=22
sudo rmmod ldd_consumer
sudo rmmod ldd_provider
```

Không thể unload provider khi consumer còn giữ dependency. Hãy quan sát cột
`Used by` trong `lsmod` và thử load consumer trước provider để hiểu lỗi symbol.

