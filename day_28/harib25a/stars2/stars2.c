#include "apilib.h"

int rand(void);

void HariMain(void) {
    char *buf;
    int win, i, x, y, timer;
    int xsize = 150, ysize = 100;

    api_initmalloc();
    buf = api_malloc(xsize * ysize);
    win = api_openwin(buf, xsize, ysize, -1, "STARS2");
    api_boxfillwin(win, 6, 26, xsize - 6 - 1, ysize - 6 - 1, 0);    // 黑色
    timer = api_alloctimer();
    api_inittimer(timer, 128);

    // for (i = 0; i < 10; i++) {
    //     // 先随机生成10个星星
    //     x = (rand() % (xsize - 6 * 2 - 1)) + 6;
    //     y = (rand() % (ysize - 26 - 6 - 1)) + 26;
    //     api_point(win + 1, x, y, 3);                                // 黄色; 传入的地址是win + 1，在hrb_api中就不会自动刷新了
    // }
    // api_refreshwin(win, 6, 26, xsize - 6, ysize - 6);  // 手动刷新
    // prev_x = x;
    // prev_y = y;

    for (;;) {
        api_settimer(timer, 50);
        if (api_getkey(1) != 128) {
            break;
        } else {
            // 擦掉上一批星星
            // api_point(win + 1, prev_x, prev_y, 0 /* black */);
            api_boxfillwin(win, 6, 26, xsize - 6 - 1, ysize - 6 - 1, 0);    // 黑色
            api_refreshwin(win, 6, 26, xsize - 6, ysize - 6);  // 手动刷新


            // 绘制下一批星星
            for (i = 0; i < 50; i++) {
                x = (rand() % (xsize - 6 * 2 - 1)) + 6;
                y = (rand() % (ysize - 26 - 6 - 1)) + 26;
                api_point(win + 1, x, y, 3);
            }
            api_refreshwin(win, 6, 26, xsize - 6, ysize - 6);  // 手动刷新
        }
    }
    
    api_end();
}
