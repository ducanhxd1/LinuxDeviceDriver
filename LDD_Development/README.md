# LDD Development

Đây là workspace học tập và phát triển chung dành cho Linux, Embedded Linux và
Linux Device Driver. Nội dung được tổ chức theo hướng: học lý thuyết vừa đủ,
thực hành có kiểm chứng, ghi lại bằng chứng và hoàn thành một dự án tích hợp.

## Mục tiêu

- Làm chủ Linux và command line.
- Viết ứng dụng Linux bằng C theo phong cách chuyên nghiệp.
- Hiểu thiết kế hệ thống và kiến trúc Embedded Linux.
- Debug, tracing và phân tích lỗi từ user space đến kernel space.
- Phát triển Embedded Linux từ ứng dụng đến Linux kernel và device driver.
- Ứng dụng AI có kiểm soát trong thiết kế, lập trình, review và debug.

## Lộ trình chính

| Giai đoạn | Chủ đề | Kết quả đầu ra |
|---|---|---|
| 01 | Linux Kernel Module | Tự build, load, cấu hình và debug kernel module |
| 02 | Linux Character Device Driver | Driver hỗ trợ open/read/write/ioctl/poll và concurrency |
| 03 | GPIO Device Driver Legacy | Hiểu GPIO API cũ và lý do cần chuyển sang descriptor API |
| 04 | Device Tree & Overlays | Viết DTS/overlay và bind thiết bị với platform driver |
| 05 | GPIO Descriptor Based | Driver GPIO dùng gpiod, IRQ, debounce và sysfs phù hợp |
| 06 | I2C SSD1306 OLED Driver | Driver I2C điều khiển màn hình SSD1306 |
| 07 | SPI SSD1306 OLED Driver | Driver SPI điều khiển màn hình SSD1306 |
| 08 | Dự án tích hợp | Một hệ thống Embedded Linux hoàn chỉnh, có test và tài liệu |

Các thư mục nền tảng `00` và công cụ `90` hỗ trợ xuyên suốt lộ trình.

## Cách học trong mỗi chủ đề

1. Đọc `README.md` và xác định kiến thức cần đạt.
2. Ghi câu hỏi và kết quả tìm hiểu trong thư mục chủ đề hoặc `notes/`.
3. Viết mã trong thư mục chủ đề; mỗi lab giải quyết một mục tiêu nhỏ.
4. Build với cảnh báo nghiêm ngặt và kiểm tra log kernel.
5. Lưu lệnh, kết quả, lỗi và cách sửa vào `evidence/`.
6. Chỉ đánh dấu hoàn thành khi đáp ứng [CHECKLIST.md](CHECKLIST.md).

## Quy tắc an toàn

- Không chạy module chưa review trên máy chứa dữ liệu quan trọng.
- Ưu tiên máy ảo, QEMU hoặc Raspberry Pi dành riêng cho thực hành.
- Luôn xem `dmesg -w` khi nạp/gỡ driver.
- Mọi đường lỗi phải giải phóng tài nguyên theo thứ tự ngược lúc cấp phát.
- Không dereference trực tiếp con trỏ user space; dùng các API `copy_*_user`.
- Không ghi trực tiếp vào thiết bị block nếu chưa xác nhận chính xác thiết bị.

## Bắt đầu

Mở [ROADMAP.md](ROADMAP.md), hoàn thành phần chuẩn bị môi trường, rồi bắt đầu
với `01_linux_kernel_module`.
