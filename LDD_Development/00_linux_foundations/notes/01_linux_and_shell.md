# Linux và shell — ghi chú nền tảng

## Mô hình cần nhớ

Shell là chương trình user space: nó đọc command, thực hiện expansion, khởi chạy
chương trình và trả exit status. Kernel quản lý process, memory, filesystem và
thiết bị; nhiều trạng thái được trình bày dưới dạng file.

```text
terminal → shell → system calls → kernel → filesystem/process/device
```

## Ba câu hỏi trước một command

1. Command sẽ đọc từ đâu?
2. Command sẽ ghi stdout và stderr đi đâu?
3. Làm sao biết command thành công (`$?`)?

```sh
rg -n "error|warning" app.log > findings.txt 2> search-errors.txt
status=$?
printf 'rg status: %d\n' "$status"
```

`>` ghi đè, `>>` nối thêm, `2>` chuyển stderr, `|` nối stdout bên trái
với stdin bên phải. Nháy kép vẫn expansion biến; nháy đơn giữ nguyên nội dung.

## File descriptor

Mỗi process có bảng file descriptor. Theo quy ước: 0 là stdin, 1 là stdout và
2 là stderr. `open()` trả descriptor; `read()`, `write()` thao tác lên nó;
`close()` kết thúc ownership.

## Virtual filesystem

- `/proc`: trạng thái process và kernel.
- `/sys`: mô hình thiết bị, driver, bus và kernel object.
- `/dev`: device nodes dùng major/minor để đi tới driver.

## Nguyên tắc an toàn

- Quote biến đường dẫn: `"$path"`.
- Kiểm tra mục tiêu trước lệnh xóa hoặc ghi đè.
- Lưu command, version và log tối thiểu đủ để tái hiện.

