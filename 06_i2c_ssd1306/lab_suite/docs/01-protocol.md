# Command, data và framebuffer

## 1. Hai lớp địa chỉ

I2C slave address là 7 bit: `0x3C`. Byte trên dây cho thao tác ghi là `0x78`.
App truyền 0x3C vào `I2C_SLAVE`, không đặt 0x78 trong payload.

```text
START → address+W → ACK → control → ACK → nội dung → ACK → STOP
```

ACK không có nghĩa là màn hình đã hiển thị đúng. `write()` đủ byte chỉ xác nhận
adapter báo hoàn tất giao dịch, không xác nhận hình quan sát được.

## 2. Control byte

```text
bit7  bit6    bit5..0
 Co   D/C#    000000
```

- `0x00`: Co=0, command stream đến hết giao dịch.
- `0x40`: Co=0, pixel-data stream đến hết giao dịch.
- Co=1 cho phép xen thêm control byte sau một byte nội dung; lab không dùng.

`{0x00, 0xA8, 0x1F}` gồm control, command, tham số. Tham số cũng thuộc command
stream. Một control 0x00 có thể mở đầu nhiều command và tham số: decoder biết
lệnh nào cần bao nhiêu byte. Không chèn 0x00 trước mỗi command trong cùng stream.
Mỗi lần write mới lại phải bắt đầu bằng control byte.

`strlen()` không dùng cho payload nhị phân. Tại nơi có mảng thật, dùng sizeof;
qua hàm, truyền kèm length. Kết quả write có kiểu ssize_t, có thể là -1.

## 3. Đọc chuỗi init

| Byte command và tham số | Vai trò |
|---|---|
| AE / AF | Display OFF / ON, không xóa RAM |
| D5 80 | Clock divide/oscillator |
| A8 1F | Multiplex: 31+1=32 hàng |
| D3 00 | Offset 0 |
| 40 | Start line 0; ở đây là command, không phải control byte |
| 8D 14 | Charge pump nội |
| 20 00 | Horizontal addressing |
| A1, C8 | Ánh xạ segment và hướng quét COM |
| DA 02 | COM sequential cho module 128x32 giả định |
| 81 xx | Contrast; không phải phần trăm tuyến tính hay tắt nguồn |
| D9 F1, DB 40 | Pre-charge/VCOMH theo cấu hình module tham chiếu |
| A4 / A5 | Hiển thị RAM / ép toàn màn hình sáng, không sửa RAM |
| A6 / A7 | Normal / inverse, không sửa RAM |
| 2E | Tắt scroll |

Các giá trị nguồn/COM không dùng chung cho mọi module. Đối chiếu datasheet và
mạch module trước khi đổi. Nếu muốn thử hardware scroll, đọc lệnh 26/27, 2F/2E,
các dummy byte, page range và interval; chưa có test scroll trong bản này.

## 4. Pixel → byte

Mỗi page có 128 byte, mỗi byte là 8 pixel dọc tại cùng cột.

```text
index = (y / 8) * 128 + x
mask = 1 << (y % 8)
on:  fb[index] |= mask
off: fb[index] &= ~mask
```

Ví dụ (10,9): index 138, mask 0x02. Xóa bit phải giữ các bit lân cận.
Vẽ vào framebuffer host không gửi I2C. Flush đặt window về cột 0..127, page
0..3, rồi gửi 512 byte. Ở horizontal mode, cột tăng; hết cột thì sang page sau.

Gửi theo 32 packet, mỗi packet `0x40 + 16 byte`. Control byte không chiếm một
pixel RAM. Không gửi tiếp phần còn thiếu sau short write như gửi file thường;
giao dịch mới có cấu trúc mới, trạng thái địa chỉ RAM có thể đã thay đổi.

## 5. Đọc code theo đường đi

1. `pixel()` và tests/graphics.c: bit mask, bounds, giữ pixel lân cận.
2. `scene()`: tạo framebuffer, chưa có I2C.
3. `flush_frame()`: window và chia packet.
4. `send_wire()` → `write_exact()`: lỗi syscall, short write.
5. `initialize()` và demo(): trạng thái panel.
6. Kernel `oled_frame()` và mutex: cùng giao thức, khác API Linux.

## 6. Bài tập sau khi chạy bản mẫu

- Tự tính framebuffer cho pixel (0,7), (0,8), (127,31), kiểm tra bằng assert.
- Thêm hình chữ nhật rỗng và đặc. Không viết I2C bên trong hàm vẽ.
- Thêm glyph số 0..9, in giá trị cảm biến giả lập.
- Đo elapsed time bằng CLOCK_MONOTONIC, tính bytes/frame và thời gian flush.
- Thử chỉ gửi một page; giải thích vì sao phải đặt lại window.
- Tách dirty regions; cân nhắc nhiều transaction nhỏ so với full frame.
- Không đổi tốc độ I2C trước khi có số đo và kiểm tra điện trở/dây.
