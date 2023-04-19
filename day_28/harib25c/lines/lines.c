#include "apilib.h"

void HariMain(void) {
    char *buf;
    int win, i;
    
    int xsize = 160, ysize = 100;

    api_initmalloc();
    buf = api_malloc(xsize * ysize);
    win = api_openwin(buf, 160, 100, -1, "LINES");
    for (i = 0; i < 8; i++) {
        api_linewin(win + 1,  8, 26, 77, i * 9 + 26, i);
        api_linewin(win + 1, 88, 26, i * 9 + 88, 89, i);
    }
    api_refreshwin(win, 6, 26, 154, 90);
    for (;;) {
        if (api_getkey(1) == 0x0a) {
            break;
        }
    }
    api_closewin(win);
    api_end();
}
