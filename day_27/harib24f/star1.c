#include "apilib.h"

void HariMain(void) {
    char *buf;
    int win;
    int xsize = 150;
    int ysize = 100;

    api_initmalloc();
    buf = api_malloc(xsize * ysize);
    win = api_openwin(buf, 150, 100, -1, "star1");
    api_boxfillwin(win, 6, 26, xsize - 6 - 1, ysize - 6 - 1, 0);  // 黑色
    api_point(win, xsize / 2, 28 + (ysize - 28) / 2, 3);              // 黄色
    api_end();
}
