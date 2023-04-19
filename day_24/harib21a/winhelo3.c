int api_openwin(char *buf, int xsize, int ysize, int col_inv, char *title);
void api_putstrwin(int win, int x, int y, int col, int len, char *str);
void api_boxfillwin(int win, int x0, int y0, int x1, int y1, int col);
void api_initmalloc(void);
char *api_malloc(int size);
void api_end(void);

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
