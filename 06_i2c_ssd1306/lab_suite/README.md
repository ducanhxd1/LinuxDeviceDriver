# SSD1306 128×32 — bộ lab từ userspace đến kernel

Bộ thực hành dành cho Pi 4 của bạn: ARM64, Debian 12, kernel đã báo
`6.12.96+rpt-rpi-v8`, OLED I2C tại địa chỉ **7-bit 0x3C**. Bạn đã kiểm chứng
module sáng toàn màn hình trong 2 giây với chương trình trước. **Bộ code mới
này chưa chạy trên Pi/OLED thật.** Không sửa file `../oled.c` và bài cũ.

Giả định: SSD1306 thật, panel 128×32, charge pump nội, COM sequential 0x02,
module có xử lý reset khi cấp nguồn. Không dành cho SH1106 hoặc panel 128×64.

## Mai bắt đầu ở đây

1. Chép **cả thư mục lab_suite** sang Pi (bỏ build/ và sản phẩm module host).
2. Mở terminal tại thư mục đó trên **Pi**, build lại từ source:

```sh
make clean
make CC=gcc
make test
./build/oled-lab --trace init
```

3. Đảm bảo chưa bind kernel driver vào 0x3C, chỉ chạy một chương trình OLED:

```sh
./build/oled-lab demo
```

Nếu bị thiếu quyền mở adapter, dùng `sudo ./build/oled-lab demo`.
Đừng dùng `I2C_SLAVE_FORCE` để vượt qua kernel driver đang chiếm thiết bị.

Demo kéo dài khoảng 25 giây (thời gian I2C cộng thêm): sáng toàn màn hình →
clear → fill RAM → các đường theo page → bàn cờ → viền/chéo → chữ OLED →
đảo màu → contrast thấp/cao → ô vuông chuyển động → clear và OFF.
Console ghi tên từng bước; bạn quan sát và đánh dấu trong
[checklist](docs/02-test-plan.md). Ctrl+C yêu cầu tắt panel và thoát 130 sau
giao dịch đang chạy; driver I2C có thể giữ syscall tới timeout khi bus có lỗi.

**Hoàn thành userspace trước**, sau đó làm [bài kernel](docs/03-kernel.md).
Không nạp module x86_64 build trên laptop lên Pi ARM64.

## Phần cứng và kiểm tra ban đầu

| Module OLED | Pi 4 (chân vật lý trên header 40 chân) |
|---|---|
| VCC | 3.3 V, chân 1, nếu đúng module hỗ trợ cấp 3.3 V |
| GND | Chân 6 |
| SDA | GPIO2, chân 3 |
| SCL | GPIO3, chân 5 |

Ngắt nguồn trước khi thay dây. Phần pull-up phải theo module OLED thực tế,
không suy từ module SHT31. SDA/SCL không được kéo lên 5 V.

```sh
uname -a
cat /etc/os-release
i2cdetect -l
ls -l /dev/i2c-1
```

Bạn đã thấy 0x3C phản hồi; không cần quét lại trong mỗi lần chạy. Khi driver
kernel đang bind, `UU` ở địa chỉ này có thể là bình thường.

## Cấu trúc và đường đi dữ liệu

```text
app/main.c         CLI, I2C transport, init, demo, sysfs backend
app/graphics.c     tọa độ → framebuffer, đường thẳng, mẫu, chữ 3x5
include/protocol.h byte init/window dùng chung
driver/oled_lab.c  I2C client driver học tập, mutex, sysfs, probe/remove
dts/               overlay tùy chọn cho Pi 4
tests/             kiểm tra framebuffer, byte stream, lỗi giả lập
docs/              lý thuyết, checklist và hướng dẫn kernel
```

Hai chế độ riêng, không chạy đồng thời:

```text
Userspace: app → open/ioctl/write → i2c-dev → adapter → SSD1306
Kernel:    app → sysfs → oled_lab → i2c_master_send → adapter → SSD1306
```

Không tạo `/dev/oled`, không có ioctl riêng hoặc DRM trong bài này.
Giao diện sysfs là ABI riêng cho học tập; hướng sản phẩm Linux là dùng subsystem
hiển thị phù hợp, tham khảo driver DRM `ssd130x` của kernel.

## Chạy từng bài

```sh
./build/oled-lab init
./build/oled-lab all
./build/oled-lab ram
./build/oled-lab pages
./build/oled-lab checker
./build/oled-lab border
./build/oled-lab text
./build/oled-lab invert
./build/oled-lab normal
./build/oled-lab contrast
./build/oled-lab animate
./build/oled-lab clear
./build/oled-lab off
```

`on/off/invert/normal/ram` chỉ gửi command, không init/xóa RAM. Các bài khác tự
init và clear trước. `all` giữ toàn màn hình sáng cho tới lệnh tiếp theo;
demo tự dọn cuối bài. Đóng fd không tự tắt panel. `text` chỉ có bốn glyph O/L/E/D
tự định nghĩa để học font; chưa có ASCII/Unicode đầy đủ.

## Trace và debug không có phần cứng

```sh
./build/oled-lab --trace pages > build/pages.trace
./build/oled-lab --trace --fail-at 3 pages
echo $?
```

Trace in từng payload I2C, không in byte address/ACK vì đó là phần adapter xử lý.
`--fail-at 3` giả lập EIO trước giao dịch thứ ba, exit 1 là **kết quả mong đợi**.
Trace bỏ delay, không đo FPS, không giả lập tín hiệu điện hoặc ACK thật.

Trên Pi, có thể tự chạy:

```sh
strace -e trace=openat,ioctl,write,close ./build/oled-lab border
```

Đọc syscall và return value, phân biệt write vào stdout với write vào adapter.
App dừng ngay khi I/O lỗi; frame có thể đã cập nhật một phần, không rollback.
Khắc phục bus rồi chạy lại bài để init và gửi full frame.

## Build và giới hạn kiểm chứng

App yêu cầu C11, POSIX/Linux, gcc/cc, make; tests dùng Python 3. Overlay cần dtc.
`make CC=aarch64-linux-gnu-gcc` hỗ trợ cross build khi toolchain/ABI đúng;
không chạy `make test` với binary ARM trên host x86.

Đã kiểm tra trên host: build với `-Werror`, kiểm thử CPU/CLI, cú pháp script,
compile overlay. Module được compile kiểm tra bằng headers host 7.0.0-31,
**không nạp**, không phải bằng chứng tương thích runtime Pi 6.12.
API probe/remove và nhánh sysfs legacy được đối chiếu với source Linux 6.12.
Tất cả test phần cứng, bind/unbind và module build đúng kernel Pi còn chờ bạn.

Không gọi bộ này là toàn bộ Linux graphics: chưa có DRM/KMS, suspend/resume,
regulator/reset GPIO, DMA, framebuffer mmap, tự động phục hồi bus hoặc dirty
rectangle. Animation dùng full frame và delay tương đối, không phải realtime.

## Tài liệu và thứ tự đọc

1. [Command/data/framebuffer](docs/01-protocol.md)
2. [Checklist test và tìm lỗi](docs/02-test-plan.md)
3. [Kernel driver, Device Tree, cleanup](docs/03-kernel.md)

Nguồn chính: [SSD1306 datasheet, Solomon Systech](https://www.sunrom.com/download/SSD1306.pdf),
[Linux 6.12 userspace I2C](https://docs.kernel.org/6.12/i2c/dev-interface.html),
[Linux 6.12 I2C client drivers](https://docs.kernel.org/6.12/i2c/writing-clients.html),
[Adafruit SSD1306 initialization reference](https://github.com/adafruit/Adafruit_SSD1306/blob/master/Adafruit_SSD1306.cpp).

## Cleanup

Userspace: `./build/oled-lab off`, rồi `make clean` nếu muốn xóa bản build app.
Kernel: làm theo phần cleanup trong bài kernel, đúng cách đã tạo device.
Không tự sửa boot config, cài service hoặc nạp module khi chạy `make`.
