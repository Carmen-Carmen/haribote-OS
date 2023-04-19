#include "bootpack.h"

struct TIMERCTL timerctl;

void init_pit(void) {
    io_out8(PIT_CTRL, 0x34);
    io_out8(PIT_CNT0, 0x9c);    // 中断周期数低位
    io_out8(PIT_CNT0, 0x2e);    // 中断周期数高位
    timerctl.count = 0;         // 初始化TIMECTL中计数器部分
    timerctl.next_time = 0xffffffff; // 没有正在运行的timer，距离下一个超时无穷远
    timerctl.using = 0;

    int i;
    for (i = 0; i < MAX_TIMER; i++) {
        timerctl.timers0[i].flags = 0;  // 初始化时将所有timers0中所有timer设定为“未使用”
    }
    return;
}

struct TIMER *timer_alloc(void) {
    int i;
    // 遍历找到未使用的定时器
    for (i = 0; i < MAX_TIMER; i++) {
        if (timerctl.timers0[i].flags == 0) {
            timerctl.timers0[i].flags = TIMER_FLAGS_ALLOC;  // 已分配，未运行
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
    timerctl.using++;
    
    // 当前只有1个timer
    if (timerctl.using == 1) {
        timerctl.t0 = timer;
        timer->next = 0;
        timerctl.next_time = timer->timeout;
        io_store_eflags(e);
        return;
    }

    struct TIMER *t, *s;
    t = timerctl.t0;
    // 当前timer应插到t之前
    if (timer->timeout <= t->timeout) {
        timerctl.t0 = timer;
        timer->next = t;
        timerctl.next_time = timer->timeout;
        io_store_eflags(e);
        return;
    }

    // 插到s和t之间
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

    // 插到最后
    // 此时s指向最后一个timer
    s->next = timer;
    timer->next = 0;

    io_store_eflags(e);

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
    // 遍历链表中所有timer
    int i;  // 记录超时timer数量
    for (i = 0; i < timerctl.using; i++) {
        if (timer->timeout > timerctl.count) {
            break;
        }
        // 将超时的timer信息压入缓冲区
        timer->flags = TIMER_FLAGS_ALLOC;
        fifo32_put(timer->fifo, timer->data);
        timer = timer->next;    // 向后遍历链表
    }

    // 此时有i个timer超时
    timerctl.using -= i;
    // 经过处理的timer指针就是下一个要超时的
    timerctl.t0 = timer;

    // 判断是否还有在运行的timer
    if (timerctl.using > 0) {
        timerctl.next_time = timerctl.t0->timeout;
    } else {
        timerctl.next_time = 0xffffffff;
    }
        
    return;
}


