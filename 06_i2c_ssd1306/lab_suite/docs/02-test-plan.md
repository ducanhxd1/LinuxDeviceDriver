# Checklist cho buổi test trên Pi

Ghi ngày, `uname -a`, phiên bản code, nguồn/module và hình/video. Kết quả dưới đây
là **mong đợi**, chưa phải kết quả đã quan sát với bộ lab này.

## A. Test không cần OLED

```sh
make clean
make CC=gcc
make test
./build/oled-lab --trace pages > build/pages.trace
```

- [ ] Compiler không cảnh báo/lỗi.
- [ ] Hai nhóm test in PASS.
- [ ] Trace đầu tiên là init bắt đầu `00 AE`.
- [ ] Giao dịch dữ liệu bắt đầu `40`, 16 byte sau là pixel.
- [ ] Có window cột 0..127, page 0..3.

Giả lập lỗi có chủ đích:

```sh
./build/oled-lab --trace --fail-at 3 pages
echo $?
```

- [ ] Exit 1, báo I/O error, không báo thành công giả.
- [ ] Hiểu rằng EIO này được phần mềm chèn, không kiểm chứng adapter thật.

## B. Test userspace trên OLED

```sh
./build/oled-lab demo
```

| Bài | Kết quả mong đợi | Kết quả thực tế |
|---|---|---|
| all | Toàn panel sáng 2 giây, bypass RAM | |
| clear | Tối nhưng panel vẫn ON | |
| fill | Toàn panel sáng bằng 512 byte RAM FF | |
| pages | 4 đường ngang, mỗi page có một bit/hàng sáng khác nhau | |
| checker | Bàn cờ ô 4×4 | |
| border | Viền sát 4 mép và đường chéo trong vùng 32×32 | |
| text | Chữ OLED, phóng 3 lần từ glyph 3×5 | |
| invert | Nền/chữ đảo màu | |
| contrast | Độ sáng thấp rồi cao, không đổi pixel | |
| animate | Ô vuông 8×8 đi ngang rồi quay lại | |
| kết thúc | Clear rồi OFF | |

Các bài có thể chạy riêng: `./build/oled-lab border` v.v. Bài riêng giữ hình,
demo thì dọn cuối. Kiểm tra tọa độ tương đối và đủ số hàng/cột; hướng trái/phải
phụ thuộc cách đặt module và segment/COM mapping.

Thử lệnh không sửa RAM:

```sh
./build/oled-lab border
./build/oled-lab off
./build/oled-lab on
./build/oled-lab invert
./build/oled-lab normal
```

- [ ] OFF rồi ON giữ hình (nếu module chưa mất nguồn).
- [ ] Invert rồi normal phục hồi hình.
- [ ] Ctrl+C khi animate: app dừng, cố gửi OFF, exit 130.
- [ ] Giải thích được khác biệt `all` và `fill`.

## C. Debug theo bằng chứng

| Triệu chứng | Bước kiểm tra đầu tiên |
|---|---|
| Không có /dev/i2c-1 | i2cdetect -l, cấu hình I2C, i2c-dev |
| Permission denied | ls -l /dev/i2c-1, quyền user; chưa kết luận bus lỗi |
| Device/resource busy | Có driver bind hoặc bản lab khác giữ flock? |
| Remote I/O error / EIO | Địa chỉ, nguồn, GND, dây; mã lỗi tùy adapter |
| all sáng nhưng fill sai | Window, horizontal mode, data control 40, số byte |
| Một nửa panel hoặc lệch hàng | Kích thước thật, multiplex, COM configuration |
| Hình xoay/lật | A0/A1, C0/C8; chưa phải lỗi framebuffer |
| Không thấy thay đổi sau set_pixel | Đã flush chưa? Có đang A5 hoặc OFF không? |
| Có vệt/rác khi truyền lỗi | Frame cập nhật một phần; sửa bus rồi init/full flush |
| Busy sau nạp kernel module | Chuyển sang --sysfs hoặc cleanup driver; không force |

Nếu muốn test khi không có thiết bị: tắt nguồn Pi, tháo OLED, cấp nguồn lại rồi
chạy init. Không rút dây live để tạo lỗi. Không đoán tên lỗi phải cố định; ghi
nguyên errno và log. Sau đó tắt nguồn, nối lại đúng dây và test phục hồi.

## D. Biên bản ngắn

```text
Bài:
Giả thuyết:
Lệnh:
Return code:
Kết quả trên panel:
Khác dự kiến:
Phép kiểm tra tiếp theo:
```

Đừng thay cùng lúc nhiều tham số khi debug. Giữ một ảnh/frame chuẩn để so sánh.

## E. Sau userspace

Làm bài kernel kế tiếp; chạy lại đúng demo bằng --sysfs. Thử input lỗi và
unbind/rebind theo tài liệu. Không coi compile module là pass phần cứng.
