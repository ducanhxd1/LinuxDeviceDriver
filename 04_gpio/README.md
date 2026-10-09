# GPIO Device Driver — Legacy API

Mục tiêu là đọc hiểu và bảo trì driver cũ sử dụng integer GPIO API, đồng thời
nhận diện hạn chế khiến code mới nên dùng descriptor-based API.

Lab: output LED; input button; IRQ; debounce; migration assessment.

Các step để xây dựng 1 driver control GPIO on/off cho Pi 4

## Mã nguồn cho kernel module
*   **Giao tiếp Device Node**: Tự động cấp phát Major/Minor và tạo file thiết bị tại `/dev/rpi_gpio`.
*   **Điều khiển Output (GPIO 21)**: Nhận tín hiệu `'1'` hoặc `'0'` từ User-space để bật/tắt một đèn LED vật lý.
*   **Đọc trạng thái Input (GPIO 20)**: Đọc mức điện áp vật lý (HIGH/LOW) từ một nút bấm hoặc dây nối đất và gửi ngược lên User-space.
*   **Xử lý lỗi an toàn**: Sử dụng cơ chế giải phóng bộ nhớ bằng nhãn `goto` tránh rò rỉ bộ nhớ hệ thống (Memory Leak).

---

## 🛠️ Sơ Đồ Kết Nối Phần Cứng (Hardware Pinout)

Để kiểm thử tính năng của Driver, bạn cần kết nối chân trên Header 40-pin của Raspberry Pi 4 như sau:

| Ngoại vi | Chân GPIO | Chân Vật Lý trên Board | Ghi chú |
| :--- | :---: | :---: | :--- |
| **Đèn LED (+)** | GPIO 21 | **Pin 40** | Nối tiếp qua điện trở 220Ω xuống GND |
| **Nút bấm / Dây test** | GPIO 20 | **Pin 38** | Chạm dây này vào Pin 39 (GND) để giả lập nhấn nút |
| **Mát (GND)** | GND | **Pin 39** | Chân đất chung để kiểm tra |

---

## Các Cách Kiểm Thử (Testing)

### Cấp quyền truy cập file thiết bị cho User thường
    sudo chmod 666 /dev/rpi_gpio

### 🔹 Cách 1: Điều khiển LED (Tính năng Ghi - Write)
Gửi dữ liệu từ Terminal xuống driver thông qua lệnh `echo`:
```bash
echo "1" > /dev/rpi_gpio    # LED tại chân 21 sẽ SÁNG (HIGH)
echo "0" > /dev/rpi_gpio    # LED tại chân 21 sẽ TẮT (LOW)
```

### 🔹 Cách 2: Đọc Nút Bấm thủ công (Tính năng Đọc - Read)
Sử dụng lệnh `cat` để lấy dữ liệu trạng thái hiện tại của chân GPIO 20:
```bash
# Khi để dây tự do (hoặc trạng thái lơ lửng/HIGH)
cat /dev/rpi_gpio
> 1

# Khi lấy dây từ chân 38 (GPIO 20) quẹt giữ vào chân 39 (GND)
cat /dev/rpi_gpio
> 0
```

### 🔹 Cách 3: Chạy ứng dụng tự động quét bằng C (User-space App)
Tạo file `test_button.c` ngoài môi trường Kernel, biên dịch và chạy:
```bash
gcc test_button.c -o test_button
./test_button
```
*Ứng dụng sẽ liên tục in ra trạng thái của nút bấm theo thời gian thực (mỗi 500ms) lên màn hình.*

---

## 🧹 Gỡ Bỏ Driver Khi Hoàn Thành
Khi không muốn thử nghiệm nữa, hãy giải phóng tài nguyên và gỡ Driver ra khỏi hệ thống một cách an toàn:
```bash
sudo rmmod rpi_gpio_driver
make clean
```

---

## Cần nhớ
1. **Kiến trúc Top-Down**: Tổ chức code rõ ràng từ Macros phần cứng ➡️ Callbacks xử lý ➡️ Init/Exit Module.
2. **Cầu nối User-Kernel**: Thành thục cơ chế trao đổi vùng nhớ an toàn bằng `copy_from_user` và `copy_to_user`.
3. **Thao tác thanh ghi**: Biết cách đổi địa chỉ vật lý của chip BCM2711 sang địa chỉ ảo bằng `ioremap` và thao tác bằng các hàm an toàn `ioread32`/`iowrite32`.

### Sử dụng các thư viện kernel

    - linux/module.h
    - linux/kernel.h
    - linux/fs.h
    - linux/io.h (chưa ioremap ánh xạ địa chỉ vật lý sang địa chỉ ảo)

### Khởi tạo module (module_init):

    - Đăng ký 1 Character Device (allow_chrdev_region, cdev_init, cdev_add)
    - Ánh xạ vùng nhớ GPIO bằng ioremap()
    - Cấu hình thanh ghi (input, output, ngắt, ...)

### Dọn dẹp module (module_exit):

    - Giải phóng vùng nhớ đã ánh xạ (iounmap)
    - Hủy đăng ký Character Device và trả lại major/minor number.

### Các hàm xử lý file (file_operations):

    - Hàm read(): Đọc trạng thái chân GPIO
    - Hàm write():: Ghi giá trị (bật/tắt) ra chân GPIO.

### Khai báo thông tin module:
    - MODULE_LICENSE("GPL"), MODULE_AUTHOR(...).



