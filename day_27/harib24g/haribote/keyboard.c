#include "bootpack.h"

struct FIFO32 *keyfifo;
int keydata0;           // 256～512的数据用于表示键盘信息，keydata0就是256

// 等到键盘控制器（KeyBoard Controller, KBC）能够进行通信
void wait_KBC_sendready(void) {
    while (1) {
        // io_in8(PORT_KEYSTA) != KEYSTA_SEND_NOTREADY，即准备好了
        if ((io_in8(PORT_KEYSTA) & KEYSTA_SEND_NOTREADY) == 0) {
            break;
        }
    }
}


// 初始化键盘控制器
void init_keyboard(struct FIFO32 *fifo, int data0) {
    // 将传入的FIFO缓冲区信息存入全局变量keyfifo和keydata0
    keyfifo = fifo;
    keydata0 = data0;

    // initialize keyboard
    wait_KBC_sendready();                       // 等KBC做好准备
    io_out8(PORT_KEYCMD, KEYCMD_WRITE_MODE);    // 向KBC，发送模式设定指令，即下一条信息是用来设定模式的
    
    wait_KBC_sendready();                       // 等KBC做好准备
    io_out8(PORT_KEYDAT, KBC_MODE);             // 向KBC的数据端口（？）传递0x47，即使用鼠标模式
    
    return;
}

// 处理来自PS/2键盘的中断
void inthandler21(int *esp) {
    io_out8(PIC0_OCW2, 0x60 + 0x01);    // 告知PIC0，IRQ-01已经受理完毕，第二个参数是0x60 + IRQ号码
    // 上一行代码执行完后，PIC才会继续监视IRQ1中断是否发生；否则会认为这个中断请求还没被处理

    int data;
    data = io_in8(PORT_KEYDAT);         // 接收返回的信息
    
    // 向FIFO型缓冲区存数据
    fifo32_put(keyfifo, data + keydata0);  // data + 256

    return;
}
