#include <fcntl.h>
#include <linux/i2c-dev.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#include <unistd.h>

static int oled_send(int fd, const uint8_t *buffer, size_t length);

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

  // send payload
  // uint8_t payload[] = {0x00, 0xAE};
  // ssize_t payload_written = write(fd, payload, sizeof(payload));

  // send command dua tren datasheet ssd1306
  uint8_t set_mux[] = {0x00, 0xA8, 0x1F}; // set_mux oled 128x32
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

  int resOled = oled_send(fd, set_mux, sizeof(set_mux));

  if (resOled == -1) {
    close(fd);
    return EXIT_FAILURE;
  }

  close(fd);
  return 0;
}

static int oled_send(int fd, const uint8_t *buffer, size_t length) {
  size_t data_send = write(fd, buffer, length);
  if (data_send == -1) {
    perror("write");
    return -1;
  } else if ((size_t)data_send != length) {
    fprintf(stderr, "length & data_send %zu, %zd", length, data_send);
    return -1;
  } else
    return 0;
}
