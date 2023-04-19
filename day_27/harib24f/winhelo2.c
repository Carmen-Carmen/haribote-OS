#include "apilib.h"

char buf[150 * 50];

void HariMain(void) {
    int win;
    win = api_openwin(buf, 150, 50, -1, "hello2");
    api_boxfillwin(win, 8, 36, 141, 42, 3);   // 亮黄色
    api_putstrwin(win, 28, 28, 0, 13, "Hello, world!");
    api_end();
}
