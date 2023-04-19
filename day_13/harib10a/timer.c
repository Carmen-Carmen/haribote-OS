#include "bootpack.h"

struct TIMERCTL timerctl;

void init_pit(void) {
    io_out8(PIT_CTRL, 0x34);
    io_out8(PIT_CNT0, 0x9c);    // 中断周期数低位
    io_out8(PIT_CNT0, 0x2e);    // 中断周期数高位
    timerctl.count = 0;         // 初始化TIMECTL中计数器部分
    timerctl.next = 0xffffffff; // 没有正在运行的timer，距离下一个超时无穷远
    timerctl.using = 0;

    int i;
    for (i = 0; i < MAX_TIMER; i++) {
        timerctl.timers0[i].flags = 0;  // 初始化时将所有timers0中所有timer设定为“未使用”
    }
    return;
}

struct TIMER *timer_alloc(void) {
    int i;
    // 遍历找到未使用的计时器
    for (i = 0; i < MAX_TIMER; i++) {
        if (timerctl.timers0[i].flags == 0) {
            timerctl.timers0[i].flags = TIMER_FLAGS_ALLOC;  // 已分配，未运行
            return &timerctl.timers0[i];                    // 将此未使用的计时器的地址返回
        }
    }

    // 没找到
    return 0;
}

void timer_free(struct TIMER *timer) {
    timer->flags = 0;
    return;
}

void timer_init(struct TIMER *timer, struct FIFO8 *fifo, unsigned char data) {
    timer->fifo = fifo;
    timer->data = data;
    return;
}

void timer_settime(struct TIMER *timer, unsigned int timeout) {
    timer->timeout = timeout + timerctl.count;
    timer->flags = TIMER_FLAGS_USING;
    int e = io_load_eflags();
    io_cli();

    // 遍历timers数组，找到插入位置
    int i;
    for (i = 0; i < timerctl.using; i++) {
        if (timerctl.timers[i]->timeout >= timer->timeout) {
            break;
        }
    }
    // i后所有timer向后移1位
    timerctl.using++;
    int j;
    for (j = timerctl.using - 1; j > i; j--) { 
        timerctl.timers[j] = timerctl.timers[j - 1];
    }
    timerctl.timers[i] = timer;
    timerctl.next = timerctl.timers[0]->timeout;    // 更新next
    io_store_eflags(e);

    return;
}

void inthandler20(int *esp) {
    io_out8(PIC0_OCW2, 0x60);   // 通知PIC0，IRQ-00的信号接收完毕
    timerctl.count++;           // 每次处理PIT中断时都让count++

    // 当前时刻是否已经到达下一个超时的timer（一般来说都没到
    if (timerctl.next > timerctl.count) {
        return;
    }

    // 遍历timerctl中的所有 正在运行 的timer
    int i;
    for (i = 0; i < timerctl.using; i++) {
        // 因为遍历的是运行中的timer，所以不能再通过flags来判断
        // 遍历的数组是timers数组，其中timer按照timeout顺序排列
        if (timerctl.timers[i]->timeout > timerctl.count) {
            // 最小的timer都没超时，就跳出循环
            break;
        }
        // 找到超时的timer中timeout最小的那个
        timerctl.timers[i]->flags = TIMER_FLAGS_ALLOC;
        fifo8_put(timerctl.timers[i]->fifo, timerctl.timers[i]->data);
    }

    // 此时有i个timer超时
    timerctl.using -= i;
    // 将i个timer移动到数组头部
    int j;
    for (j = 0; j < timerctl.using; j++) {
        timerctl.timers[j] = timerctl.timers[i + j];
    }
    // 判断是否还有在运行的timer
    if (timerctl.using > 0) {
        timerctl.next = timerctl.timers[0]->timeout;
    } else {
        timerctl.next = 0xffffffff;
    }
        
    return;
}


