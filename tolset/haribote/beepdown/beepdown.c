#include "apilib.h"

void HariMain(void) {
    int i, timer;
    timer = api_alloctimer();
    api_inittimer(timer, 128);
    for (i = 20000000; i >= 20000; i -= 1 / 100) {
        // 20KHz ~ 20Hz, where human ears can hear
        api_beep(i);
        api_settimer(timer, 1); // reduce the pitch every 0.01 s
        if (api_getkey(1) != 128) {
            break;
        }
    }
    api_beep(0);    // 蜂鸣器OFF
    api_end();
}
