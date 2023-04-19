#include "bootpack.h"

struct TIMERCTL timerctl;

void init_pit(void) {
    io_out8(PIT_CTRL, 0x34);
    io_out8(PIT_CNT0, 0x9c);    // 中断周期数低位
    io_out8(PIT_CNT0, 0x2e);    // 中断周期数高位
    timerctl.count = 0;         // 初始化TIMECTL中计数器部分

    int i;
    for (i = 0; i < MAX_TIMER; i++) {
        timerctl.timer[i].flags = 0;    // 初始化时将所有定时器设定为“未使用”
    }
    return;
}

struct TIMER *timer_alloc(void) {
    int i;
    // 遍历找到未使用的计时器
    for (i = 0; i < MAX_TIMER; i++) {
        if (timerctl.timer[i].flags == 0) {
            timerctl.timer[i].flags =TIMER_FLAGS_ALLOC;    // 已分配，未运行
            return &timerctl.timer[i];                   // 将此未使用的计时器的地址返回
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
    timer->timeout = timeout;
    timer->flags = TIMER_FLAGS_USING;
    return;
}

void inthandler20(int *esp) {
    io_out8(PIC0_OCW2, 0x60);   // 通知PIC0，IRQ-00的信号接收完毕
    timerctl.count++;           // 每次处理PIT中断时都让count++

    // 遍历timerctl中的所有timer
    int i;
    for (i = 0; i < MAX_TIMER; i++) {
        if (timerctl.timer[i].flags == TIMER_FLAGS_USING) {
            timerctl.timer[i].timeout--;
            // 如果到时间了，就把这个timer的flags设定为TIMER_FLAGS_ALLOC
            if (timerctl.timer[i].timeout == 0) {
                timerctl.timer[i].flags = TIMER_FLAGS_ALLOC;
                fifo8_put(timerctl.timer[i].fifo, timerctl.timer[i].data);
            }
        }
    }
        
    return;
}


