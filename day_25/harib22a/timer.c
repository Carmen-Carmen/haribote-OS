// timer.c

#include <stdio.h>
#include "bootpack.h"

struct TIMERCTL timerctl;

void init_pit(void) {
    io_out8(PIT_CTRL, 0x34);
    io_out8(PIT_CNT0, 0x9c);    // 中断周期数低位
    io_out8(PIT_CNT0, 0x2e);    // 中断周期数高位
    timerctl.count = 0;         // 初始化TIMECTL中计数器部分

    int i;
    for (i = 0; i < MAX_TIMER; i++) {
        timerctl.timers0[i].flags = 0;  // 初始化时将所有timers0中所有timer设定为“未使用”
    }

    // 分配一个timeout = 0xffffffff的"哨兵"timer，简化settime中链表操作
    struct TIMER *sentry;
    sentry = timer_alloc();
    sentry->timeout = 0xffffffff;
    sentry->flags = TIMER_FLAGS_USING;
    sentry->next = 0;   // 哨兵一定是链表中最后一个元素
    timerctl.t0 = sentry;
    timerctl.next_time = sentry->timeout;
    
    return;
}

struct TIMER *timer_alloc(void) {
    int i;
    // 遍历找到未使用的定时器
    for (i = 0; i < MAX_TIMER; i++) {
        if (timerctl.timers0[i].flags == 0) {
            timerctl.timers0[i].flags = TIMER_FLAGS_ALLOC;  // 已分配，未运行
            timerctl.timers0[i].flags2 = 0;                 // 先设置为不用随应用程序结束而自动取消
            return &timerctl.timers0[i];                    // 将此未使用的定时器的地址返回
        }
    }

    // 没找到
    return 0;
}

void timer_free(struct TIMER *timer) {
    timer->flags = 0;
    return;
}

void timer_init(struct TIMER *timer, struct FIFO32 *fifo, int data) {
    timer->fifo = fifo;
    timer->data = data;
    return;
}

void timer_settime(struct TIMER *timer, unsigned int timeout) {
    timer->timeout = timeout + timerctl.count;
    timer->flags = TIMER_FLAGS_USING;
    int e = io_load_eflags();
    io_cli();
    
    // 链表中一定有sentry存在，当前加入的timer不会是唯一一个
    // sentry->timeout = 0xffffffff，当前加入的timer不会是最后一个
    // 只剩以下2种可能:

    struct TIMER *t, *s;
    t = timerctl.t0;
    // 1. 当前timer应插到t之前
    if (timer->timeout <= t->timeout) {
        timerctl.t0 = timer;
        timer->next = t;
        timerctl.next_time = timer->timeout;
        io_store_eflags(e);
        return;
    }

    // 2. 插到s和t之间
    while (1) {
        // 双指针: s指向t前一个
        s = t;
        t = t->next;

        if (t == 0) {
            // 走到头了
            break;
        }

        // 找到s->timeout <= timer.timeout <= t.timeout，并插入
        if (timer->timeout <= t->timeout) {
            s->next = timer;
            timer->next = t;
            io_store_eflags(e);
            return;
        }
    }
    return;
}

void inthandler20(int *esp) {
    io_out8(PIC0_OCW2, 0x60);   // 通知PIC0，IRQ-00的信号接收完毕
    timerctl.count++;           // 每次处理PIT中断时都让count++

    // 当前时刻是否已经到达下一个超时的timer（一般来说都没到
    if (timerctl.next_time > timerctl.count) {
        return;
    }

    struct TIMER *timer;        // timer指针，经过处理后应指向当前未超时的timer中timeout最小的那个
    timer = timerctl.t0;        // t0是链表头
    
    // 任务切换相关
    char ts = 0;

    // 遍历链表中所有timer
    for (;;) {
        if (timer->timeout > timerctl.count) {
            break;
        }
        // 将超时的timer信息压入缓冲区
        timer->flags = TIMER_FLAGS_ALLOC;
        if (timer != task_timer) {
            // 普通定时器超时，向缓冲区中写数据
            fifo32_put(timer->fifo, timer->data);
        } else {
            // 多任务切换专用定时器超时
            ts = 1;
            // 不能在此处调用mt_taskswitch，因为即使中断处理还没完成时，eflags中的IF也有可能会被重设回1，此时若产生了下一个中断就会出错
        }
        timer = timer->next;    // 向后遍历链表
    }

    // 经过处理的timer指针就是下一个要超时的
    timerctl.t0 = timer;

    // 将next_time设为链表头的timeout
    timerctl.next_time = timerctl.t0->timeout;

    // 一定要等到所有中断处理都完成之后，再考虑调用mt_taskswitch
    if (ts != 0) {
        // ts == 1就是在遍历中发现ms_timer超时了
        task_switch();
    }

    return;
}

// 每隔一年执行的调整时刻
void reset_timerctl_count(void) {
    int t0 = timerctl.count;
    io_cli();
    timerctl.count -= t0;
    int i;
    for (i = 0; i < MAX_TIMER; i++) {
        if (timerctl.timers0[i].flags == TIMER_FLAGS_USING) {
            if (timerctl.timers0[i].timeout != 0xffffffff) {
                timerctl.timers0[i].timeout -= t0;
            }
        }
    }
    io_sti();
    return;
}

void set490(struct FIFO32 *fifo, int mode) {
    int i;
    struct TIMER *timer;
    if (mode != 0) {
        for (i = 0; i < 490; i++) {
            timer = timer_alloc();
            timer_init(timer, fifo, 1024 + i);
            timer_settime(timer, 50 * 24 * 60 * 60 * 100);  // 设定50天后超时
        }
    }

    return;
}

char * get_ontime() {
    unsigned int    hour, 
                    minute, 
                    second = timerctl.count / 100;
    static char s[30];

    hour = second / 3600;
    minute = (second - hour * 3600) / 60;
    second -= (hour * 3600 + minute * 60);
    
    sprintf(s, "%02d:%02d:%02d", hour, minute ,second);

    return s;
}

int timer_cancel(struct TIMER *timer) {
    int e;
    struct TIMER *t;
    e = io_load_eflags();
    io_cli();

    if (timer->flags == TIMER_FLAGS_USING) {    // 是否是正在运行的timer
        if (timer == timerctl.t0) {
            // 如果要取消的timer是链表头
            t = timer->next;

            timerctl.t0 = t;
            timerctl.next_time = t->timeout;
        } else {
            // 如果要取消的timer在链表中间
            // 先遍历链表，找到要取消的timer的前一个元素
            t = timerctl.t0;
            for (;;) {
                if (t->next == timer) {
                    break;
                }
                t = t->next;
            }
            // 将前一个元素的next指向next->next
            t->next = timer->next;
        }
        timer->flags = TIMER_FLAGS_ALLOC;
        io_store_eflags(e);
        return 1;       // 取消timer成功
    }
    io_store_eflags(e);
    return 0;           // 传入的timer不需要被取消（不在链表中）
}

// 取消所有需要在应用程序结束时自动取消的timer的方法
void timer_cancelall(struct FIFO32 *fifo) {
    int e, i;
    struct TIMER *t;
    e = io_load_eflags();
    io_cli();               // 在设置过程中禁止改变定时器状态
    for (i = 0; i < MAX_TIMER; i++) {
        t = &timerctl.timers0[i];
        if (t->flags != 0 && t->flags2 != 0 &&  // 该timer在运行且为需要自动取消的
            t->fifo == fifo) {
            timer_cancel(t);
            timer_free(t);
        }
    }
    io_store_eflags(e);
    return;
}
