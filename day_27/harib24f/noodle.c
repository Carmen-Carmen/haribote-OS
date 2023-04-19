#include <stdio.h>
#include <string.h>
#include "apilib.h"

void HariMain(void) {
    char *buf, s[12];
    int win, timer, sec = 0, min = 0, hour = 0;
    int xsize = 150, ysize = 100;
    api_initmalloc();
    buf = api_malloc(xsize * ysize);
    win = api_openwin(buf, xsize, ysize, -1, "NOODLE");
    timer = api_alloctimer();
    api_inittimer(timer, 128);  // 识别这个应用程序中timer的data是128
    
    sprintf(s, "close with any key");
    api_putstrwin(win, 3, 58, 0, strlen(s), s);
    for (;;) {
        sprintf(s, "%5d:%02d:%02d", hour, min, sec);
        api_boxfillwin(win, 28, 27, 115, 41, 7 /* white */);
        api_putstrwin(win, 28, 27, 0 /* black */, 11, s);

        api_settimer(timer, 100);   // 1秒刷新一次
        if (api_getkey(1) != 128) {
            // 如果传入了非该应用程序的timer的数据
            // 即按下任意键关闭窗口
            break;
        }
        sec++;
        if (sec == 60) {
            sec = 0;
            min++;
            if (min == 60) {
                min = 0;
                hour++;
            }
        }
    }

    api_end();
}
