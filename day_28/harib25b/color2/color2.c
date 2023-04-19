#include "apilib.h"

unsigned char rgb2pal(int r, int g, int b, int x, int y);

void HariMain(void) {
    char *buf;
    int win, x, y;
    int xsize = 144, ysize = 164;

    api_initmalloc();
    buf = api_malloc(xsize * ysize);
    win = api_openwin(buf, xsize, ysize, -1, "color2");
    for (y = 0; y < 128; y++) {
        for (x = 0; x < 128; x++) {
            buf[(x + 8) + (y + 28) * xsize] = rgb2pal(x * 2, y * 2, 0, x, y);
        }
    }
    api_refreshwin(win, 8, 28, 136, 156);
    api_getkey(1);
    api_end();

}

/*
    rgb to palette
    通过让2种颜色交替排列，使看上去向2种颜色混合在一起，达到展现出更多颜色
    这个函数根据传入的目标颜色的rgb，以传入的x和y坐标计算出在该坐标应当打印什么颜色
*/
unsigned char rgb2pal(int r, int g, int b, int x, int y) {
    static int table[4] = { 3, 1, 0, 2 };
    int i;
    x &= 1;                 // x和y判断是奇数还是偶数
    y &= 1;
    i = table[x + y * 2];   // 用来生成中间色的常量
    r = (r * 21) / 256;     // r为0~20
    g = (g * 21) / 256;
    b = (b * 21) / 256;
    r = (r + i) / 4;        // r为0~5
    g = (g + i) / 4;
    b = (b + i) / 4;

    return 16 + r + g * 6 + b * 36;
}
