int api_openwin(char *buf, int xsize, int ysize, int col_inv, char *title);
void api_boxfillwin(int win, int x0, int y0, int x1, int y1, int color);
void api_putstrwin(int win, int x, int y, int col, int len, char *s);
void api_initmalloc(void);
char *api_malloc(int size);
void api_point(int win, int x, int y, int col);
void api_refreshwin(int win, int x0, int y0, int x1, int y1);
void api_linewin(int win, int x0, int y0, int x1, int y1, int col);
int api_getkey(int mode);
void api_closewin(win);
void api_end(void);

void HariMain(void) {
    char *buf;
    int win, i, x, y;

    int xsize = 160, ysize = 100;

    api_initmalloc();
    buf = api_malloc(xsize * ysize);
    win = api_openwin(buf, xsize, ysize, -1, "WALK");
    api_boxfillwin(win, 4, 24, xsize - 4 - 1, ysize - 4 - 1, 0);    // 黑色背景

    x = 76;
    y = 56;

    api_putstrwin(win, x, y, 3 /* yellow */, 1, "*");   // 打印一个黄色的*

    for (;;) {
        i = api_getkey(1);
        api_putstrwin(win, x, y, 0, 1, "*");            // 用黑色擦除
        // 计算坐标
        if (i == '4' && x > 4  ) x -= 8;    // 向左
        if (i == '6' && x < 148) x += 8;    // 向右
        if (i == '8' && y > 24 ) y -= 8;    // 向下
        if (i == '2' && y < 80 ) y += 8;    // 向上
        if (i == 0x0a) break;               // 回车键结束
        api_putstrwin(win, x, y, 3, 1, "*");
    }

    api_closewin(win);
    api_end();
}
