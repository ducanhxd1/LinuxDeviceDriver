# Kernel module fundamentals

## Mental model

Module là object code được linker kernel ghép động vào kernel đang chạy. Nó
không phải process, không có address space riêng và lỗi truy cập memory có thể
làm hỏng toàn hệ thống.

```text
source.c → compiler/Kbuild → module.ko → loader → kernel address space
                                      init() → active → exit()
```

`module_init()` đăng ký entry point khi load; `module_exit()` đăng ký cleanup
khi unload. Mọi tài nguyên init đã sở hữu phải được exit trả lại.

## Kbuild external module

```make
obj-m += ldd_hello.o
make -C /lib/modules/$(shell uname -r)/build M=$(CURDIR) modules
```

`-C` chuyển make vào kernel build tree; `M=` nói nơi chứa external module. Kbuild
tạo `.ko` cùng các file trung gian và metadata version/symbol.

## Load tools

- `insmod path.ko`: load đúng file, không tự giải dependency.
- `rmmod name`: yêu cầu unload module đang chạy.
- `modprobe name`: tìm trong module tree, xử lý dependency và configuration.
- `depmod`: tạo dependency/index cho module tree.

## Logging

Dùng `pr_info`, `pr_warn`, `pr_err` thay vì output user-space. Không spam trong
hot path; dùng rate-limited variants khi dữ liệu ngoài có thể kích hoạt liên tục.

## Error path

Nếu bước C thất bại sau A và B, giải phóng B rồi A. Kernel thường dùng label và
`goto` để giữ một cleanup path rõ ràng, tránh duplicate cleanup và double free.

