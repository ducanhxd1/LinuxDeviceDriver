# Roadmap

## Giai đoạn 0 — Nền tảng Linux và C

- Shell, filesystem, permission, process, signal và service.
- GCC/Clang, Make, static/shared library và cross-compilation.
- Pointer, memory, bit manipulation, callback và concurrency trong C.
- Git, coding style, tài liệu và quy trình review.

## Giai đoạn 1 — Kernel module và character driver

- Kernel build system, module lifecycle, parameters và logging.
- Major/minor, VFS, file operations và device node.
- User/kernel copy, `f_pos`, ioctl, blocking I/O, poll và synchronization.
- Error handling, lifetime, reference counting và stress testing.

## Giai đoạn 2 — Hardware description và GPIO

- GPIO legacy API để hiểu code cũ.
- Device Tree syntax, phandle, compatible và overlays.
- Platform device/driver và probe/remove.
- GPIO descriptor API, IRQ, debounce và pin control.

## Giai đoạn 3 — Bus driver và SSD1306

- I2C protocol, Linux I2C subsystem và client driver.
- SPI protocol, Linux SPI subsystem và SPI device driver.
- SSD1306 command set, framebuffer, font rendering và transfer batching.
- So sánh thiết kế, hiệu năng và giới hạn của I2C với SPI.

## Giai đoạn 4 — Tích hợp hệ thống

- Build kernel, modules, Device Tree và root filesystem.
- Service/application user space giao tiếp với driver.
- Logging, observability, automated tests và fault injection.
- Đóng gói, triển khai, rollback và tài liệu vận hành.

## Tiêu chuẩn hoàn thành

- Giải thích được kiến trúc mà không phụ thuộc vào mã mẫu.
- Tự viết lại được phần cốt lõi từ một yêu cầu ngắn.
- Có test bình thường, biên và lỗi; không chỉ kiểm tra happy path.
- Không còn warning compiler đáng kể và không có kernel oops/leak đã biết.
- Có sơ đồ, hướng dẫn build/run và evidence tái hiện được kết quả.

