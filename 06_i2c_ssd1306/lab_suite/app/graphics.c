#include "graphics.h"
#include <stdlib.h>
#include <string.h>

void pixel(uint8_t *fb, int x, int y, bool on)
{
    if (x < 0 || x >= OLED_W || y < 0 || y >= OLED_H)
        return;
    unsigned index = (unsigned)(y / 8) * OLED_W + (unsigned)x;
    uint8_t mask = (uint8_t)(1u << (y % 8));
    if (on) fb[index] |= mask;
    else fb[index] &= (uint8_t)~mask;
}

/* Bresenham; callers supply display-sized coordinates. */
void line(uint8_t *fb, int x0, int y0, int x1, int y1)
{
    int dx = abs(x1-x0), sx = x0 < x1 ? 1 : -1;
    int dy = -abs(y1-y0), sy = y0 < y1 ? 1 : -1;
    int error = dx + dy;
    for (;;) {
        pixel(fb, x0, y0, true);
        if (x0 == x1 && y0 == y1) break;
        int twice = 2 * error;
        if (twice >= dy) { error += dy; x0 += sx; }
        if (twice <= dx) { error += dx; y0 += sy; }
    }
}

/* Small original 3x5 glyphs, rows encoded in low three bits. */
static void label(uint8_t *fb)
{
    static const uint8_t glyphs[][5] = {
        {7,5,5,5,7}, /* O */ {4,4,4,4,7}, /* L */
        {7,4,6,4,7}, /* E */ {6,5,5,5,6}  /* D */
    };
    for (int g=0; g<4; ++g)
        for (int y=0; y<5; ++y)
            for (int x=0; x<3; ++x)
                if (glyphs[g][y] & (1u << (2-x)))
                    for (int yy=0; yy<3; ++yy)
                        for (int xx=0; xx<3; ++xx)
                            pixel(fb, 40+g*12+x*3+xx, 8+y*3+yy, true);
}

void scene(uint8_t *fb, const char *name, unsigned frame)
{
    memset(fb, 0, OLED_BYTES);
    if (!strcmp(name, "fill")) memset(fb, 0xff, OLED_BYTES);
    else if (!strcmp(name, "pages")) {
        for (unsigned p=0; p<4; ++p)
            memset(fb+p*OLED_W, (int)(1u << p), OLED_W);
    } else if (!strcmp(name, "checker")) {
        for (int y=0; y<OLED_H; ++y)
            for (int x=0; x<OLED_W; ++x)
                pixel(fb,x,y,((x/4+y/4)%2)==0);
    } else if (!strcmp(name, "text")) label(fb);
    else if (!strcmp(name, "animate")) {
        int x = (int)(frame % (2 * (OLED_W-8)));
        if (x > OLED_W-8) x = 2*(OLED_W-8)-x;
        for (int yy=12; yy<20; ++yy)
            for (int xx=x; xx<x+8; ++xx) pixel(fb,xx,yy,true);
    } else if (!strcmp(name, "border")) {
        line(fb,0,0,127,0); line(fb,127,0,127,31);
        line(fb,127,31,0,31); line(fb,0,31,0,0);
        line(fb,0,0,31,31);
    }
}
