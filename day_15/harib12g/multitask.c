// 多任务相关
#include "bootpack.h"

struct TIMER *mt_timer;
int mt_tr;

void mt_init(void) {
    mt_timer = timer_alloc();
    timer_settime(mt_timer, 2);
    mt_tr = 3 * 8;
    return;
}

void mt_taskswitch(void) {
    // 根据当前mt_tr的值计算出下一个值，并替换
    if (mt_tr == 3 * 8) {
        mt_tr = 4 * 8;
    } else {
        mt_tr = 3 * 8;
    }

    timer_settime(mt_timer, 2); // 0.02秒后超时
    farjmp(0, mt_tr);           // 进行任务切换
    return;

}
