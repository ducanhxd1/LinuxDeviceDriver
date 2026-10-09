# Thỏa thuận làm việc chung

## Vai trò

- Bạn sở hữu quyết định kỹ thuật và trực tiếp thực hành trên phần cứng.
- AI hỗ trợ giải thích, đặt câu hỏi, thiết kế lab, review code và phân tích log.
- Mọi nội dung do AI đề xuất phải được kiểm tra bằng tài liệu chính thức,
  compiler, test hoặc kết quả quan sát trên hệ thống thật.

## Chu trình cho mỗi thay đổi

1. Viết mục tiêu và tiêu chí chấp nhận.
2. Chia thay đổi thành bước nhỏ có thể test.
3. Implement và lưu chính xác lệnh đã chạy.
4. Review warning, log kernel, cleanup path và concurrency.
5. Cập nhật tài liệu cùng với code.

## Quy ước mã nguồn

- Tuân thủ Linux kernel coding style cho kernel code.
- Mỗi module phải có `MODULE_LICENSE`, mô tả và lifecycle rõ ràng.
- Kiểm tra giá trị trả về; dùng mã lỗi âm chuẩn trong kernel.
- Tránh magic number; ghi rõ đơn vị và giới hạn buffer.
- Commit nhỏ, một mục đích, nội dung message diễn tả lý do thay đổi.

## Khi yêu cầu AI hỗ trợ

Cung cấp mục tiêu, board/kernel version, kiến trúc CPU, phần cứng nối dây,
Device Tree liên quan, lệnh build, log đầy đủ và điều đã thử. Không đưa secret,
private key hoặc thông tin đăng nhập vào prompt hay repository.

