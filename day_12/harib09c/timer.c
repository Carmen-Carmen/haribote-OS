#include "bootpack.h"

struct TIMERCTL timerctl;

void init_pit(void) {
    io_out8(PIT_CTRL, 0x34);
    io_out8(PIT_CNT0, 0x9c);    // 中断周期数低位
    io_out8(PIT_CNT0, 0x2e);    // 中断周期数高位
    timerctl.count = 0;         // 初始化TIMECTL中计数器部分
    timerctl.count = 0;
    return;
}

void inthandler20(int *esp) {
    io_out8(PIC0_OCW2, 0x60);   // 通知PIC0，IRQ-00的信号接收完毕
    timerctl.count++;           // 每次处理PIT中断时都让count++

    if (timerctl.timeout > 0) {
        timerctl.timeout--;
        if (timerctl.timeout == 0) {
            fifo8_put(timerctl.fifo, timerctl.data);
        }
    }
    return;
}

void settimer(unsigned int timeout, struct FIFO8 *fifo, unsigned char data) {
    int eflags;
    eflags = io_load_eflags();
    io_cli();
    timerctl.timeout = timeout;
    timerctl.fifo = fifo;
    timerctl.data = data;
    io_store_eflags(eflags);
    return;
}


