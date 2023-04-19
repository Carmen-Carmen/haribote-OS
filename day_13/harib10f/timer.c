#include "bootpack.h"

struct TIMERCTL timerctl;

void init_pit(void) {
    io_out8(PIT_CTRL, 0x34);
    io_out8(PIT_CNT0, 0x9c);    // 中断周期数低位
    io_out8(PIT_CNT0, 0x2e);    // 中断周期数高位
    timerctl.count = 0;         // 初始化TIMECTL中计数器部分
    timerctl.next = 0xffffffff; // 没有正在运行的timer，距离下一个超时无穷远

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
    timer->timeout = timeout + timerctl.count; // 改变timemout的定义为：予定时刻，即timerctl.count到达timeout时超时
    timer->flags = TIMER_FLAGS_USING;
    if (timerctl.next > timer->timeout) {
        // 如果新设定的计时器timeout比timerctl中存储的下一个到达的超时还小，就要更新timerctl.next
        timerctl.next = timer->timeout;
    }
    return;
}

void inthandler20(int *esp) {
    io_out8(PIC0_OCW2, 0x60);   // 通知PIC0，IRQ-00的信号接收完毕
    timerctl.count++;           // 每次处理PIT中断时都让count++

    // 当前时刻是否已经到达下一个超时的timer（一般来说都没到
    if (timerctl.next > timerctl.count) {
        return;
    }
    timerctl.next = 0xffffffff; // 将timerctl.next设为最大值，因为后续循环中需要将timerctl.next设为最小的timer.timeout

    // 遍历timerctl中的所有timer
    int i;
    for (i = 0; i < MAX_TIMER; i++) {
        if (timerctl.timer[i].flags == TIMER_FLAGS_USING) {
            if (timerctl.timer[i].timeout <= timerctl.count) { 
                // 有timer超时
                timerctl.timer[i].flags = TIMER_FLAGS_ALLOC;
                fifo8_put(timerctl.timer[i].fifo, timerctl.timer[i].data);
            } else {
                // 还没有超时
                // 遍历所有没超时的timer，找到其中最小的timeout并赋给timerctl.next
                if (timerctl.next > timerctl.timer[i].timeout) {
                    timerctl.next = timerctl.timer[i].timeout;  // 更新timerctl.next，即下一个近期超时的timer
                }
            }
        }
    }
        
    return;
}


