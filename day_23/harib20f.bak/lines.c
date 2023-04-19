#include <stdio.h>

int api_openwin(char *buf, int xsize, int ysize, int col_inv, char *title);
void api_initmalloc(void);
char *api_malloc(int size);
void api_refreshwin(int win, int x0, int y0, int x1, int y1);
void api_linewin(int win, int x0, int y0, int x1, int y1, int col);
int api_getkey(int mode);
void api_closewin(int win);
void api_end(void);
void api_putstr0(char *s);
void farjmp(int eip, int cs);

void HariMain(void) {
    char *buf;
    int win, i;
    char s[128];
    
    int xsize = 160, ysize = 100;

    // farjmp(0, 3);

    api_initmalloc();
    buf = api_malloc(xsize * ysize);
    win = api_openwin(buf, xsize, ysize, -1, "LINES");

    // for (i = 0; i < 8; i++) {
    //     api_linewin(win + 1,  8, 26, 77, i * 9 + 26, i);
    //     api_linewin(win + 1, 88, 26, i * 9 + 88, 89, i);
    // }
    // api_refreshwin(win, 6, 26, 154, 90);
    for (;;) {
        if (api_getkey(1) == 0x0a) {
            break;
        }
    }
    api_closewin(win);
    api_end();
}
