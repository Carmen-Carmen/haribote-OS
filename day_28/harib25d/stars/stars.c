#include "apilib.h"

int rand(void);

void HariMain(void) {
    char *buf;
    int win, i, x, y;
    int xsize = 150, ysize = 100;

    api_initmalloc();
    buf = api_malloc(xsize * ysize);
    win = api_openwin(buf, xsize, ysize, -1, "STARS");
    api_boxfillwin(win, 6, 26, xsize - 6 - 1, ysize - 6 - 1, 0);    // 黑色
    for (i = 0; i < 50; i++) {
        // 随机生成50个星星
        x = (rand() % (xsize - 6 * 2 - 1)) + 6;
        y = (rand() % (ysize - 26 - 6 - 1)) + 26;
        api_point(win, x, y, 3);                                    // 黄色
    }
    for (;;) {
	if (api_getkey(1) == 0x0a)
	    break;
    }
    
    api_end();
}
