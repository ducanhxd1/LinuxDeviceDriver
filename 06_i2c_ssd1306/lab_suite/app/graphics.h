#ifndef GRAPHICS_H
#define GRAPHICS_H
#include <stdbool.h>
#include <stdint.h>
#include "protocol.h"
void pixel(uint8_t *fb, int x, int y, bool on);
void line(uint8_t *fb, int x0, int y0, int x1, int y1);
void scene(uint8_t *fb, const char *name, unsigned frame);
#endif
