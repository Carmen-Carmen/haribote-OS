#include "apilib.h"

void HariMain(void) {
    char buf[150 * 50];	// 从栈内存中分配

    int win;
    win = api_openwin(buf, 150, 50, -1, "hello2");
    api_boxfillwin(win, 8, 36, 141, 42, 3);   // 亮黄色
    api_putstrwin(win, 28, 28, 0, 13, "Hello, world!");
    for (;;) {
	if (api_getkey(1) == 0x0a)
	    break;
    }
    api_end();
}
