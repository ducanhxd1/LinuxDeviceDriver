#include <fcntl.h>
#include <linux/i2c-dev.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <unistd.h>

#define OLED_WIDTH 128
#define OLED_HEIGHT 32
#define OLED_PAGES (OLED_HEIGHT / 8)      /* 4 pages cho panel 128x32 */
#define FB_SIZE (OLED_WIDTH * OLED_PAGES) /* 512 byte */

static uint8_t framebuffer[FB_SIZE];

static int oled_send(int fd, const uint8_t *buffer, size_t length);
static int oled_init(int fd);
static int oled_clear_ram(int fd);
static int oled_flush(int fd);
static void fb_clear(void);
static void fb_set_pixel(int x, int y, int on);
static void fb_draw_char(int x, int y, char c);
static void fb_draw_string(int x, int y, const char *s);

/* ---------------------------------------------------------------------
 * Font 5x7 tối giản: chỉ định nghĩa vài ký tự đủ để demo (mở rộng thêm
 * theo nhu cầu). Mỗi ký tự là 5 cột, mỗi cột 1 byte, bit0 = hàng trên
 * cùng (giống layout GDDRAM theo page).
 * ------------------------------------------------------------------- */
typedef struct {
  char c;
  uint8_t cols[5];
} font_glyph_t;

static const font_glyph_t FONT_5x7[] = {
    {' ', {0x00, 0x00, 0x00, 0x00, 0x00}},
    {'0', {0x3E, 0x51, 0x49, 0x45, 0x3E}},
    {'1', {0x00, 0x42, 0x7F, 0x40, 0x00}},
    {'2', {0x42, 0x61, 0x51, 0x49, 0x46}},
    {'3', {0x21, 0x41, 0x45, 0x4B, 0x31}},
    {'4', {0x18, 0x14, 0x12, 0x7F, 0x10}},
    {'5', {0x27, 0x45, 0x45, 0x45, 0x39}},
    {'6', {0x3C, 0x4A, 0x49, 0x49, 0x30}},
    {'7', {0x01, 0x71, 0x09, 0x05, 0x03}},
    {'8', {0x36, 0x49, 0x49, 0x49, 0x36}},
    {'9', {0x06, 0x49, 0x49, 0x29, 0x1E}},
    {'A', {0x7E, 0x11, 0x11, 0x11, 0x7E}},
    {'C', {0x3E, 0x41, 0x41, 0x41, 0x22}},
    {'E', {0x7F, 0x49, 0x49, 0x49, 0x41}},
    {'I', {0x00, 0x41, 0x7F, 0x41, 0x00}},
    {'K', {0x7F, 0x08, 0x14, 0x22, 0x41}},
    {'N', {0x7F, 0x04, 0x08, 0x10, 0x7F}},
    {'O', {0x3E, 0x41, 0x41, 0x41, 0x3E}},
    {'P', {0x7F, 0x09, 0x09, 0x09, 0x06}},
    {'R', {0x7F, 0x09, 0x19, 0x29, 0x46}},
    {'T', {0x01, 0x01, 0x7F, 0x01, 0x01}},
    {':', {0x00, 0x36, 0x36, 0x00, 0x00}},
};
#define FONT_COUNT (sizeof(FONT_5x7) / sizeof(FONT_5x7[0]))

int main(void) {
  int fd = open("/dev/i2c-1", O_RDWR);
  if (fd == -1) {
    perror("open");
    return EXIT_FAILURE;
  }

  if (ioctl(fd, I2C_SLAVE, 0x3C) < 0) {
    perror("ioctl");
    close(fd);
    return EXIT_FAILURE;
  }

  if (oled_init(fd) == -1 || oled_clear_ram(fd) == -1) {
    close(fd);
    return EXIT_FAILURE;
  }

  /* Demo: vẽ chuỗi vào framebuffer rồi flush lên panel */
  fb_clear();
  fb_draw_string(0, 0, "PI4 OK");
  fb_draw_string(0, 2, "12:03");

  if (oled_flush(fd) == -1) {
    close(fd);
    return EXIT_FAILURE;
  }

  puts("Da flush framebuffer len OLED");
  close(fd);
  return EXIT_SUCCESS;
}

/* ---------------------------------------------------------------------
 * I2C helpers
 * ------------------------------------------------------------------- */
static int oled_send(int fd, const uint8_t *buffer, size_t length) {
  ssize_t data_send = write(fd, buffer, length);
  if (data_send < 0) {
    perror("write");
    return -1;
  } else if ((size_t)data_send != length) {
    fprintf(stderr, "Incomplete write: expected %zu, got %zd\n", length,
            data_send);
    return -1;
  }
  return 0;
}

static int oled_init(int fd) {
  const uint8_t commands[] = {
      0x00,       /* Control byte: toan bo phan sau la command */
      0xAE,       /* Display OFF */
      0xD5, 0x80, /* Clock divide / oscillator freq */
      0xA8, 0x1F, /* Multiplex ratio: 32 hang */
      0xD3, 0x00, /* Display offset */
      0x40,       /* Display start line */
      0x8D, 0x14, /* Charge pump enable */
      0x20, 0x00, /* Horizontal addressing mode */
      0xA1,       /* Segment remap */
      0xC8,       /* COM scan direction remapped */
      0xDA, 0x02, /* COM pin config cho 128x32 */
      0x81, 0x8F, /* Contrast */
      0xD9, 0xF1, /* Pre-charge period */
      0xDB, 0x40, /* VCOMH config */
      0xA4,       /* Hien thi theo RAM */
      0xA6,       /* Normal display */
      0x2E,       /* Tat scrolling */
      0xAF,       /* Display ON -- code cu chua bat, them vao day */
  };
  return oled_send(fd, commands, sizeof commands);
}

/* Xoa toan bo GDDRAM (512 byte = 0) truoc khi flush framebuffer thuc */
static int oled_clear_ram(int fd) {
  const uint8_t window[] = {
      0x00, 0x21, 0x00,          OLED_WIDTH - 1, /* Column 0..127 */
      0x22, 0x00, OLED_PAGES - 1                 /* Page 0..3 */
  };
  if (oled_send(fd, window, sizeof window) == -1)
    return -1;

  uint8_t packet[17] = {0};
  packet[0] = 0x40; /* control byte: data */
  for (size_t offset = 0; offset < FB_SIZE; offset += 16) {
    if (oled_send(fd, packet, sizeof packet) == -1)
      return -1;
  }
  return 0;
}

/* Day toan bo framebuffer[] len GDDRAM */
static int oled_flush(int fd) {
  const uint8_t window[] = {0x00, 0x21, 0x00,          OLED_WIDTH - 1,
                            0x22, 0x00, OLED_PAGES - 1};
  if (oled_send(fd, window, sizeof window) == -1)
    return -1;

  uint8_t packet[17];
  packet[0] = 0x40;
  for (size_t offset = 0; offset < FB_SIZE; offset += 16) {
    memcpy(&packet[1], &framebuffer[offset], 16);
    if (oled_send(fd, packet, sizeof packet) == -1)
      return -1;
  }
  return 0;
}

/* ---------------------------------------------------------------------
 * Framebuffer helpers (logic thuan, khong dung I2C)
 * ------------------------------------------------------------------- */
static void fb_clear(void) { memset(framebuffer, 0, FB_SIZE); }

/* x: 0..127, y: 0..31. Framebuffer layout giong GDDRAM: moi byte la 1
 * cot trong 1 page (8 hang doc), page = y / 8, bit = y % 8. */
static void fb_set_pixel(int x, int y, int on) {
  if (x < 0 || x >= OLED_WIDTH || y < 0 || y >= OLED_HEIGHT)
    return;
  int page = y / 8;
  int bit = y % 8;
  size_t idx = (size_t)page * OLED_WIDTH + x;
  if (on)
    framebuffer[idx] |= (1 << bit);
  else
    framebuffer[idx] &= ~(1 << bit);
}

static const font_glyph_t *font_lookup(char c) {
  for (size_t i = 0; i < FONT_COUNT; i++)
    if (FONT_5x7[i].c == c)
      return &FONT_5x7[i];
  return &FONT_5x7[0]; /* fallback: khoang trang */
}

/* Ve 1 ky tu 5x7 tai vi tri (x, page_y). page_y la so page (0..3), moi
 * ky tu cao 1 page (8 pixel doc) de don gian hoa layout. */
static void fb_draw_char(int x, int page_y, char c) {
  const font_glyph_t *g = font_lookup(c);
  int y0 = page_y * 8;
  for (int col = 0; col < 5; col++) {
    uint8_t colbits = g->cols[col];
    for (int row = 0; row < 7; row++) {
      int on = (colbits >> row) & 0x01;
      fb_set_pixel(x + col, y0 + row, on);
    }
  }
}

static void fb_draw_string(int x, int page_y, const char *s) {
  int cursor = x;
  for (; *s != '\0'; s++) {
    fb_draw_char(cursor, page_y, *s);
    cursor += 6; /* 5 cot ky tu + 1 cot khoang cach */
    if (cursor >= OLED_WIDTH)
      break;
  }
}
