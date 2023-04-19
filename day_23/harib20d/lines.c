int api_openwin(char *buf, int xsize, int ysize, int col_inv, char *title);
void api_boxfillwin(int win, int x0, int y0, int x1, int y1, int color);
void api_initmalloc(void);
char *api_malloc(int size);
void api_point(int win, int x, int y, int col);
void api_refreshwin(int win, int x0, int y0, int x1, int y1);
void api_linewin(int win, int x0, int y0, int x1, int y1, int col);
void api_end(void);

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
    api_end();
}
