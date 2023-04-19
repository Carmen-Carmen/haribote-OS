int api_openwin(char *buf, int xsize, int ysize, int col_inv, char *title);
void api_putstrwin(int win, int x, int y, int col, int len, char *s);
void api_boxfillwin(int win, int x0, int y0, int x1, int y1, int col);
void api_end(void);

char buf[150 * 50];

void HariMain(void) {
    int win;
    win = api_openwin(buf, 150, 50, -1, "hello2");
    api_boxfillwin(win, 8, 36, 141, 42, 3);   // 亮黄色
    api_putstrwin(win, 28, 28, 0, 13, "Hello, world!");
    api_end();
}
