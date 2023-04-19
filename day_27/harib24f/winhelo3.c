#include "apilib.h"

void HariMain(void) {
    char *buf;
    int win;

    int xsize = 200, 
        ysize = 150;

    api_initmalloc();
    buf = api_malloc(xsize * ysize);
    win = api_openwin(buf, xsize, ysize, -1, "Hello3");
    api_boxfillwin(win, 8, 36, 141, 42, 6); // 浅蓝色
    api_putstrwin(win, 28, 28, 0, 14, "Hello3, world!");
    api_end();
}
