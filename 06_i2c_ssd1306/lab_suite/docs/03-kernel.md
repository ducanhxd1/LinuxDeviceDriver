# Kernel driver và Device Tree — chạy sau userspace

## Mục tiêu và ABI

Driver này là **I2C client driver**, điều khiển SSD1306 qua adapter có sẵn.
Nó không thay thế driver controller bcm2835 và không bit-bang GPIO2/3.
API đối chiếu Linux 6.12: probe(client), remove(client) trả void,
i2c_master_send(), i2c_check_functionality().

Driver tạo ba file trong `/sys/bus/i2c/devices/1-003c/`:

| File | Protocol |
|---|---|
| command | Ghi chuỗi init/on/off/all/ram/invert/normal, newline tùy chọn |
| contrast | Số 0..255, dạng thập phân hoặc hex 0x.. |
| frame | Một lần write đúng 512 byte, offset 0; không có control byte |

Đây là ABI sysfs học tập, chỉ root được ghi. Frame đã theo page layout, driver
tự thêm control/window. Không đọc lại được GDDRAM qua ABI này; phần buffer trong
driver chỉ là bản sao frame ghi thành công gần nhất. Nếu I2C lỗi giữa chừng,
panel có thể chứa một phần frame mới. Không có rollback.

Driver dùng mutex bảo vệ cả frame và command, tránh hai sysfs callback xen
transaction. Nó không giữ độc quyền toàn bộ một buổi demo; chỉ chạy một app
điều khiển panel tại một thời điểm để tránh thay đổi trạng thái giữa các lệnh.

## 1. Build đúng kernel trên Pi

Chạy ở thư mục lab_suite trên **Pi**, không phải laptop:

```sh
uname -r
ls -ld /lib/modules/$(uname -r)/build
make module KDIR=/lib/modules/$(uname -r)/build
modinfo driver/oled_lab.ko
```

Nếu thiếu thư mục build, tìm package headers phù hợp **đúng phiên bản đang
chạy** hoặc dùng prepared build tree của kernel đó, có config/generated headers
và Module.symvers khi cần. Dùng `apt-cache search linux-headers` để khảo sát;
không cài ngẫu nhiên headers generic, không ép vermagic và không tự nâng kernel.
Module host đã build thử 7.0.0-31 không dùng trên Pi 6.12 ARM64.

Makefile nhận diện const sysfs ABI từ headers để hỗ trợ khác biệt phiên bản;
việc build trên Pi vẫn là bước bắt buộc. Nếu lỗi, lưu nguyên log.

## 2. Chọn MỘT cách tạo I2C client

Để test lần đầu, dùng **cách A**. Cách B dành cho bài Device Tree sau đó.
Không tạo hai client cùng địa chỉ hoặc để driver SSD1306 khác cùng điều khiển.

### A. Tạo device động, không đổi boot config

Trước tiên tắt panel bằng ứng dụng cũ và kết thúc các app userspace:

```sh
./build/oled-lab off
ls /sys/bus/i2c/devices/1-003c
```

Nếu đường dẫn **đã tồn tại**, đọc `name` và `readlink .../driver` để biết ai đang
quản lý. Dừng bước tạo mới và xử lý đúng owner; không xóa device không biết nguồn.
Nếu không tồn tại, nạp và tạo device:

```sh
sudo insmod driver/oled_lab.ko
printf 'oled_lab 0x3c\n' | sudo tee /sys/bus/i2c/devices/i2c-1/new_device
readlink /sys/bus/i2c/devices/1-003c/driver
sudo dmesg | tail -30
ls -l /sys/bus/i2c/devices/1-003c/{command,contrast,frame}
```

Kỳ vọng driver link trỏ tới oled_lab, log `128x32 lab ready`, ba attributes có
mặt. Device node tồn tại **không đảm bảo probe thành công**; kiểm tra link/log.
Khi chạy lại, không insmod/new_device lần hai nếu chúng đã tồn tại.

Chạy app qua driver:

```sh
sudo ./build/oled-lab --sysfs /sys/bus/i2c/devices/1-003c demo
```

Các bài riêng cũng dùng được:

```sh
sudo ./build/oled-lab --sysfs /sys/bus/i2c/devices/1-003c border
printf 'invert\n' | sudo tee /sys/bus/i2c/devices/1-003c/command
printf 'normal\n' | sudo tee /sys/bus/i2c/devices/1-003c/command
printf '32\n' | sudo tee /sys/bus/i2c/devices/1-003c/contrast
```

Lưu ý `sudo echo ... > file` không nâng quyền phép chuyển hướng của shell;
dùng tee như trên. Không dùng --bus trong lúc driver đã bind.

### B. Device Tree overlay, tùy chọn sau khi cleanup cách A

Overlay chỉ cho Pi 4 với label i2c1, địa chỉ 0x3c, GPIO pinmux mặc định đã bật.
Compatible `learning,ssd1306-128x32` là binding riêng của bài, không phải binding
upstream Solomon. Do dùng tên riêng, nó không tự bind vào DRM driver có sẵn.

```sh
make overlay
sudo insmod driver/oled_lab.ko
sudo dtoverlay -d "$PWD/build" oled-lab
dtoverlay -l
readlink /sys/bus/i2c/devices/1-003c/driver
```

Nếu dtoverlay không có, xem công cụ của image đang chạy; cách A vẫn dùng được.
Không cần sửa config.txt để làm bài. Không load overlay lần hai khi đã tồn tại.
Test bằng --sysfs giống cách A. Dtc build chỉ kiểm tra cú pháp/fixup, không chứng
minh label target đã resolve trên DT đang chạy; xem dmesg nếu overlay thất bại.

## 3. Test input lỗi (có chủ đích)

Khi driver đã bind:

```sh
printf 'unknown\n' | sudo tee /sys/bus/i2c/devices/1-003c/command
printf '256\n' | sudo tee /sys/bus/i2c/devices/1-003c/contrast
```

Kỳ vọng báo lỗi; các input không hợp lệ phải bị từ chối trước khi gửi I2C.
Test frame 511/512/513 byte bằng một lần os.write:

```sh
sudo python3 - <<'PY'
import os
path = '/sys/bus/i2c/devices/1-003c/frame'
for length in (511, 512, 513):
    fd = os.open(path, os.O_WRONLY)
    try:
        try:
            n = os.write(fd, bytes(length))
        except OSError as error:
            print(length, 'rejected:', error)
            assert length != 512
        else:
            print(length, 'accepted:', n)
            assert length == 512 and n == 512
    finally:
        os.close(fd)
PY
```

512 byte zero xóa RAM. 511 và 513 phải thất bại. Một write 0 byte có thể được
VFS xử lý trước callback: không coi nó là full frame thành công.

## 4. Unbind/rebind (giữ client, đổi trạng thái driver)

Dừng demo trước, rồi chạy:

```sh
printf '1-003c\n' | sudo tee /sys/bus/i2c/drivers/oled_lab/unbind
printf '1-003c\n' | sudo tee /sys/bus/i2c/drivers/oled_lab/bind
```

Kỳ vọng OFF khi remove, sysfs attributes bị gỡ rồi tạo lại khi probe thành công.
Client vẫn còn nên đây không phải cleanup đầy đủ để trở về userspace.

## 5. Cleanup theo đúng cách tạo device

**Cách A** — chỉ xóa client oled_lab do bạn tạo bằng new_device:

```sh
cat /sys/bus/i2c/devices/1-003c/name
printf '0x3c\n' | sudo tee /sys/bus/i2c/devices/i2c-1/delete_device
sudo rmmod oled_lab
```

**Cách B** — xem danh sách và gỡ đúng overlay oled-lab:

```sh
dtoverlay -l
sudo dtoverlay -r oled-lab
sudo rmmod oled_lab
```

Không dùng delete_device cho node do Device Tree tạo. Xác nhận 1-003c không còn
rồi mới chạy lại `./build/oled-lab border`. Đừng chạy cleanup của cả A và B.
Muốn xóa build module trên Pi:

```sh
make -C /lib/modules/$(uname -r)/build M="$PWD/driver" clean
```

## 6. Đọc driver để hiểu lifecycle

1. id_table/of_match_table nối client với driver như thế nào?
2. probe kiểm tra adapter, cấp phát devm, init panel rồi mới tạo sysfs.
3. callback validate input, khóa mutex, gửi giao dịch, trả errno.
4. remove gỡ sysfs, đợi callbacks, gửi OFF; devm thu hồi bộ nhớ sau đó.
5. shutdown cố gửi OFF; chưa có suspend/resume và reset/regulator management.

Vì sao dùng mutex thay spinlock? I2C transfer có thể sleep. Vì sao khóa cả frame?
Khóa từng packet vẫn cho phép một command khác thay đổi addressing giữa frame.

Để tiến tới sản phẩm: đọc `drivers/gpu/drm/solomon/ssd130x*` trong đúng source
kernel, so sánh binding, reset, power management và DRM/KMS. Đừng quảng bá private
sysfs demo như một display driver production hoàn chỉnh.

Nguồn: [I2C client driver Linux 6.12](https://docs.kernel.org/6.12/i2c/writing-clients.html),
[sysfs.h Linux 6.12](https://github.com/torvalds/linux/blob/v6.12/include/linux/sysfs.h),
[Raspberry Pi overlays](https://www.raspberrypi.com/documentation/computers/configuration.html).
