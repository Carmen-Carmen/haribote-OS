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

// 编码显示的部分会大大降低中断处理函数的执行速度，因此为了在其他地方处理，建立了键盘缓冲区KEYBUF
struct KEYBUF keybuf;   // 键盘数据的缓冲区

// 处理来自PS/2键盘的中断
void inthandler21(int *esp) {
    struct BOOTINFO *binfo = (struct BOOTINFO *) ADR_BOOTINFO;

    io_out8(PIC0_OCW2, 0x60 + 0x01);    // 告知PIC0，IRQ-01已经受理完毕，第二个参数是0x60 + IRQ号码
    // 上一行代码执行完后，PIC才会继续监视IRQ1中断是否发生；否则会认为这个中断请求还没被处理

    unsigned char *data = NULL;
    data = io_in8(PORT_KEYDAT);         // 接收返回的信息
    
    if (keybuf.flag == 0) {
        keybuf.data = data;             
        keybuf.flag = 1;                // 表示缓冲区有数据了
    }

    return;
}

// 处理来自PS/2鼠标的中断
void inthandler2c(int *esp) {
    struct BOOTINFO *binfo = (struct BOOTINFO *) ADR_BOOTINFO;
    boxfill8(binfo->vram, binfo->scrnx, COL8_000000, 0, 0, 32 * 8 - 1, 15);
    putstring8_ascii(binfo->vram, binfo->scrnx, 0, 0, COL8_FFFFFF, "INT 2C (IRQ-12): PS/2 mouse");
    while (1) { io_hlt(); }
}

// 处理IRQ7中断
// 因为对于一些机种，随着PIC的初始化会产生一次IRQ7中断，如果不对该中断处理程序执行STI（设置中断标志位，EFLAGS中），操作系统的启动会失败。这个中断处理只会执行一次。
void inthandler27(int *esp) {
    io_out8(PIC0_OCW2, 0x67);   // 通知PIC，IRQ-7已经处理好了
    return;
}
