#include "bootpack.h"

// 使鼠标可用
void enable_mouse(struct MOUSE_DEC *mdec) {
    wait_KBC_sendready();
    io_out8(PORT_KEYCMD, KEYCMD_SENDTO_MOUSE);  // 向KBC发送0xd4，即下一个数据是发送给鼠标的

    wait_KBC_sendready();
    io_out8(PORT_KEYDAT, MOUSECMD_ENABLE);      // 通过数据端口，向鼠标发送0xf4，激活鼠标

    return;
}

// 解析鼠标传回的数据，并赋值给MOUSE_DEC结构体变量（mouse decoded）
/*
    return: 0, 正常; 1, 已处理完phase3; -1, 出错
*/
int mouse_decode(struct MOUSE_DEC *mdec, unsigned char data) {
    if (mdec->phase == 0) {
    // 等待鼠标的0xfa状态
        if (data == 0xfa) {
            mdec->phase = 1;
        }
        return 0;
    }
    if (mdec->phase == 1) {
    // 鼠标传回的第1字节（按键信息）
        if ((data & 0xc8) == 0x08) {
            // 第1字节应当是0x08才正确
            mdec->buf[0] = data;
            mdec->phase = 2;
        }
        return 0;
    }
    if (mdec->phase == 2) {
    // 鼠标传回的第2字节（水平移动信息）
        mdec->buf[1] = data;
        mdec->phase = 3;
        return 0;
    }
    if (mdec->phase == 3) {
    // 鼠标传回的第3字节（垂直移动信息）
        mdec->buf[2] = data;
        mdec->phase = 1;

        // 鼠标按键状态放在buf[0]的低3位
        mdec->btn = mdec->buf[0] & 0x07;    // 0x07，即通过与0b00000111进行与运算，取出buf[0]的低3位

        mdec->x = mdec->buf[1];
        mdec->y = mdec->buf[2];
        // 解析x、y坐标，并将x和y的第8位及之后都设为1
        if ((mdec->buf[0] & 0x10) != 0) {   // 左右移动时buf[0]的高位是1
            mdec->x |= 0xffffff00;
        }
        if ((mdec->buf[0] & 0x20) != 0) {   // 上下移动是buf[0]的高位是2
            mdec->y |= 0xffffff00;
        }
        mdec->y = - mdec->y;                // 鼠标与屏幕的y方向相反，鼠标向上移动y值增大，屏幕则是左上方为(0, 0)

        return 1;
    }

    return -1;  // 只有出错了才有可能到这里
}

struct FIFO8 mousefifo;

// 处理来自PS/2鼠标的中断
void inthandler2c(int *esp) {
    io_out8(PIC1_OCW2, 0x60 + 0x0c - 0x08);  // 先通知PIC1，IRQ-12已经受理完毕，IRQ-12是PIC1的第4号（IRQ-08开始的第4个）
    io_out8(PIC0_OCW2, 0x60 + 0x02);         // 再通知PIC0，IRQ-02已经受理完毕（PIC1连在PIC0的第2号上）

    unsigned char data;
    data = io_in8(PORT_KEYDAT);
    fifo8_put(&mousefifo, data);
    return;
}
