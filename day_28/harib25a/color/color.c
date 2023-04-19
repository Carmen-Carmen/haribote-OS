#include "apilib.h"

void HariMain(void) {
    char *buf;
    int win, x, y, r, g, b;
    int xsize = 144, ysize = 164;

    api_initmalloc();
    buf = api_malloc(xsize * ysize);
    win = api_openwin(buf, xsize, ysize, -1, "color");

    for (y = 0; y < 128; y++) {
        for (x = 0; x < 128; x++) {
            r = x * 2;
            g = y * 2;
            b = 0;

            buf[(x + 8) + (y + 28) * 144] = 16 + (r / 43) + (g / 43) * 6 + (b / 43) * 36;   // 是直接对缓冲区操作，不是boxfill或者line，所以不会自动刷新哈
        }
    }
    api_refreshwin(win, 8, 28, 136, 156);
    api_getkey(1);  // 等待按下任意键
    api_end();
}
