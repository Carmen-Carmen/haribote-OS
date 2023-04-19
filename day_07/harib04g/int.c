// 处理中断相关
#include "bootpack.h"
#include <stdio.h>

// PIC的初始化
void init_pic(void) {
    io_out8(PIC0_IMR , 0xff  ); // 禁止PIC0所有中断
    io_out8(PIC1_IMR , 0xff  ); // 禁止PIC1所有中断

    io_out8(PIC0_ICW1, 0x11  ); // 边沿触发模式 edge trigger mode
    io_out8(PIC0_ICW2, 0x20  ); // PIC0的IRQ0-7由CPU的INT20-27接收
    io_out8(PIC0_ICW3, 1 << 2); // 00000100 PIC1由IRQ2连接到PIC0
    io_out8(PIC0_ICW4, 0x01  ); // 无缓冲区模式

    io_out8(PIC1_ICW1, 0x11  ); // 边沿触发模式
    io_out8(PIC1_ICW2, 0x28  ); // PIC1的IRQ8-15由CPU的INT28-2f接收
    io_out8(PIC1_ICW3, 2     ); // PIC1由IRQ2连接到PIC1
    io_out8(PIC1_ICW4, 0x01  ); // 无缓冲区模式

    io_out8(PIC0_IMR , 0xfb  ); // 11111011 PIC1以外的中断全部禁止
    io_out8(PIC1_IMR , 0xff  ); // 11111111 禁止PIC1所有中断

    return;
}

#define PORT_KEYDAT     0x0060

struct FIFO8 keyfifo;
struct FIFO8 mousefifo;

// 处理来自PS/2键盘的中断
void inthandler21(int *esp) {
    io_out8(PIC0_OCW2, 0x60 + 0x01);    // 告知PIC0，IRQ-01已经受理完毕，第二个参数是0x60 + IRQ号码
    // 上一行代码执行完后，PIC才会继续监视IRQ1中断是否发生；否则会认为这个中断请求还没被处理

    unsigned char data = NULL;
    data = io_in8(PORT_KEYDAT);         // 接收返回的信息
    
    // 向FIFO型缓冲区存数据
    fifo8_put(&keyfifo, data);

    return;
}

// 处理来自PS/2鼠标的中断
void inthandler2c(int *esp) {
    io_out8(PIC1_OCW2, 0x60 + 0x0c - 0x08);  // 先通知PIC1，IRQ-12已经受理完毕，IRQ-12是PIC1的第4号（IRQ-08开始的第4个）
    io_out8(PIC0_OCW2, 0x60 + 0x02);         // 再通知PIC0，IRQ-02已经受理完毕（PIC1连在PIC0的第2号上）

    unsigned char data = NULL;
    data = io_in8(PORT_KEYDAT);
    fifo8_put(&mousefifo, data);
    return;
}

// 处理IRQ7中断
// 因为对于一些机种，随着PIC的初始化会产生一次IRQ7中断，如果不对该中断处理程序执行STI（设置中断标志位，EFLAGS中），操作系统的启动会失败。这个中断处理只会执行一次。
void inthandler27(int *esp) {
    io_out8(PIC0_OCW2, 0x67);   // 通知PIC，IRQ-7已经处理好了
    return;
}
