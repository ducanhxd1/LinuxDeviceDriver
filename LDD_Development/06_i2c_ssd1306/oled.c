#include <fcntl.h>
#include <linux/i2c-dev.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#include <unistd.h>

static int oled_send(int fd, const uint8_t *buffer, size_t length);
static int oled_init(int fd);
static int oled_clear(int fd);

int main(void) {
  int fd;
  fd = open("/dev/i2c-1", O_RDWR);

  if (fd == -1) {
    perror("open");
    return -1;
  }

  int res = ioctl(fd, I2C_SLAVE, 0x3C);

  if (res < 0) {
    perror("ioctl");
    fprintf(stdout, "system call ioctl called error: %d\n", res);
    close(fd);
    return EXIT_FAILURE;
  } else {
    fprintf(stdout, "ioctl called success: %d\n", res);
  }

  if (oled_init(fd) == -1 || oled_clear(fd) == -1) {
    close(fd);
    return EXIT_FAILURE;
  }

  const uint8_t test_on[] = {0x00, 0xA5, 0xAF};

  if (oled_send(fd, test_on, sizeof(test_on)) == -1) {
    close(fd);
    return EXIT_FAILURE;
  }

  puts("All pixels ON for 2 seconds");
  sleep(2);

  const uint8_t resume_ram[] = {0x00, 0xA4};

  if (oled_send(fd, resume_ram, sizeof(resume_ram)) == -1) {
    close(fd);
    return EXIT_FAILURE;
  }

  puts("Back to cleared RAM");
  close(fd);
  return EXIT_SUCCESS;

  // send payload
  // uint8_t payload[] = {0x00, 0xAE};
  // ssize_t payload_written = write(fd, payload, sizeof(payload));

  // send command dua tren datasheet ssd1306
  // uint8_t set_mux[] = {0x00, 0xA8, 0x1F}; // set_mux oled 128x32
  // uint8_t enable_charge_pump[] = {0x00, 0x8D, 0x14}; // enable_charge_pump

  /*
  if (payload_written == -1) {
    perror("write");
    close(fd);
    return EXIT_FAILURE;
  } else if (payload_written != (ssize_t)sizeof(payload)) {
    fprintf(stderr, "Incomplete write: expected %zu bytes, got %zd\n",
            sizeof(payload, payload_written));
    close(fd);
    return EXIT_FAILURE;
  }
  printf("Send %zd bytes\n", payload_written);
  */

  // int resOled = oled_send(fd, set_mux, sizeof(set_mux));
  //
  // if (resOled == -1) {
  //   close(fd);
  //   return EXIT_FAILURE;
  // }
  //
  // close(fd);
  // return 0;
}

static int oled_send(int fd, const uint8_t *buffer, size_t length) {
  ssize_t data_send = write(fd, buffer, length);
  if (data_send < 0) {
    perror("write");
    return -1;
  } else if ((size_t)data_send != length) {
    fprintf(stderr, "length & data_send %zu, %zd", length, data_send);
    return -1;
  } else
    return 0;
}

static int oled_init(int fd) {
  const uint8_t commands[] = {
      0x00, // Control byte: toàn bộ phần sau là command

      0xAE,       // Display OFF trong khi cấu hình
      0xD5, 0x80, // Clock divide ratio và oscillator frequency
      0xA8, 0x1F, // Multiplex ratio: 32 hàng
      0xD3, 0x00, // Display offset: 0
      0x40,       // Display start line: 0

      0x8D, 0x14, // Bật charge pump nội
      0x20, 0x00, // Horizontal addressing mode

      0xA1,       // Segment remap
      0xC8,       // COM scan direction: remapped
      0xDA, 0x02, // COM configuration cho panel 128x32 thông dụng

      0x81, 0x8F, // Contrast
      0xD9, 0xF1, // Pre-charge period cho charge pump nội
      0xDB, 0x40, // VCOMH configuration

      0xA4, // Hiển thị theo nội dung RAM
      0xA6, // Normal display, không đảo màu
      0x2E  // Tắt scrolling
  };

  // Chưa bật ON: xóa RAM trước để tránh hiện dữ liệu rác.
  return oled_send(fd, commands, sizeof commands);
}

static int oled_clear(int fd) {
  const uint8_t window[] = {
      0x00, 0x21, 0x00, 0x7F, // Cột 0..127
      0x22, 0x00, 0x03        // Page 0..3, mỗi page cao 8 pixel
  };

  if (oled_send(fd, window, sizeof window) == -1)
    return -1;

  // Một control byte + 16 byte pixel bằng 0.
  uint8_t packet[17] = {0};
  packet[0] = 0x40;

  // 128 * 32 / 8 = 512 byte; gửi 32 lần, mỗi lần 16 byte pixel.
  for (size_t offset = 0; offset < 512; offset += 16) {
    if (oled_send(fd, packet, sizeof packet) == -1)
      return -1;
  }

  return 0;
}
