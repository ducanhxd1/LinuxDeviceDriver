# Tìm hiểu về Device Files trong Linux

## Thiết bị Block và Character

## Major & Minor Number

### Device file nằm ở đâu và tại sao chúng là file "ảo"?

* Thư mục `/dev` chứa các file đặc biệt gọi là **device node**.
```
ls /dev
ls /dev | wc -l
```

* `b` = block device, `c` = character device
```
ls -lh /dev/mmcblk0 /dev/tty0 /dev/gpiochip0
```

* Xem minor number của các thiết bị
```
ls -lh /dev/tty{1,2,3,4,5}
```

* Xem danh sách các major number đã được đăng ký với kernel
```
cat /proc/devices
```

### Tên file không quan trọng

**Character Device: Ví dụ với cổng Serial**

* Bật cổng Serial
```
sudo raspi-config
```

* Cài đặt `screen`
```
sudo apt install screen -y
```

* Mở cổng serial bằng `screen` (nối tắt chân UART TX và RX để test)
```
sudo screen /dev/ttyS0 9600
```
*Khi gõ bất kỳ ký tự nào, nó sẽ được echo lại — chứng tỏ giao tiếp serial đang hoạt động.*  
<small>Thoát khỏi screen: nhấn `Ctrl + A`, sau đó nhấn `D`.</small>

* Tạo device node mới tên `my_serial`
```
sudo mknod my_serial c 4 64
```

* Mở lại cổng serial bằng node mới `my_serial`
```
sudo screen my_serial 9600
```
<small>Hoạt động y hệt như khi dùng `ttyS0`.</small>

**Tên file không quan trọng — chỉ có Major + Minor number mới quyết định thiết bị nào được dùng.**

---

**Block Device: Ví dụ với thẻ nhớ SD Card**

* Dump 512 bytes đầu tiên của `mmcblk0`
```
sudo hexdump -C /dev/mmcblk0 -n 512 | head
```

* Tạo device node của riêng mình
```shell
sudo mknod my_mblk b 179 0 # 179, 0 là Major và Minor number giống với mmcblk0
```
```
ls -lh my_mblk
```

* Đọc 512 bytes đầu tiên từ device node mới `my_mblk`
```
sudo hexdump -C my_mblk -n 512 | head
```