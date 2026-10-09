#include "graphics.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>

int main(void)
{
    uint8_t fb[OLED_BYTES]={0};
    pixel(fb,10,9,true);
    assert(fb[138]==2);
    pixel(fb,10,8,true);
    assert(fb[138]==3);
    pixel(fb,10,9,false);
    assert(fb[138]==1); /* clearing a bit must preserve its neighbor */
    pixel(fb,127,31,true);
    assert(fb[511]==128);
    uint8_t before[OLED_BYTES]; memcpy(before,fb,sizeof fb);
    pixel(fb,-1,0,true); pixel(fb,128,0,true);
    pixel(fb,0,-1,true); pixel(fb,0,32,true);
    assert(!memcmp(before,fb,sizeof fb));
    memset(fb,0,sizeof fb);
    line(fb,31,31,0,0);
    unsigned lit=0;
    for (unsigned i=0;i<sizeof fb;++i)
        for (unsigned b=0;b<8;++b) lit += !!(fb[i] & (1u<<b));
    assert(lit==32);
    for (int i=0;i<32;++i) assert(fb[(i/8)*128+i] & (1u<<(i%8)));
    scene(fb,"clear",0);
    for (unsigned i=0;i<sizeof fb;++i) assert(fb[i]==0);
    scene(fb,"fill",0);
    for (unsigned i=0;i<sizeof fb;++i) assert(fb[i]==255);
    puts("PASS graphics: bit preservation, bounds, diagonal, clear/fill");
    return 0;
}
