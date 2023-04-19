// 处理中断相关
#include "bootpack.h"

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
