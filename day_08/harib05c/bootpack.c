// bootpack的主文件
#include <stdio.h>
#include "bootpack.h"   // 引入同一个文件夹内的头文件

extern struct FIFO8 keyfifo;    // 键盘缓冲区，注明是外部（int.c中）声明的结构体
extern struct FIFO8 mousefifo;  // 鼠标数据缓冲区

struct MOUSE_DEC {  // mouse decoded，存放解析鼠标传回信息的结构体
    unsigned char buf[3];
    unsigned char phase;
    int x, y, btn;          // 存放当前鼠标的x、y坐标以及按下按键的状态
};

void init_keyboard(void);
void enable_mouse(struct MOUSE_DEC *mdec);
int mouse_decode(struct MOUSE_DEC *mdec, unsigned char data);

void HariMain(void) {
    init_gdtidt();  // 初始化GDT和IDT，一定要在PIC之前初始化
    init_pic();     // 初始化PIC
    io_sti();       // IDT和PIC的初始化已经完成了，此时可以解除CPU对于中断的禁止（STI指令）
        
    init_palette(); // 初始化调色板
    
    struct BOOTINFO *binfo;
    binfo = (struct BOOTINFO *) 0x0ff0; // asmhead里第一条BOOT_INFO的内存地址
    init_screen8(binfo->vram, binfo->scrnx, binfo->scrny);

    // 显示鼠标
    int mx, my;
    char *mcursor = NULL;   // 避免"使用了可能未初始化的本地指针变量"
    mx = (binfo->scrnx - 16) / 2;
    my = (binfo->scrny - 16) / 2;
    init_mouse_cursor8(mcursor, COL8_008484);
    putblock8(binfo->vram, binfo->scrnx, 16, 16, mx, my, mcursor, 16);

    int i;                  // 存放从缓冲区中取出数据的变量
    // 用于显示键盘按键编码的变量
    char *keybuf = NULL;    // 在初始化缓冲区时真正赋予缓冲区内存空间
    fifo8_init(&keyfifo, 32, keybuf);   // "&"是取地址运算符，把结构体的内存地址作为参数传入

    // 用于显示鼠标信息的变量
    char *mousebuf = NULL;
    fifo8_init(&mousefifo, 128, mousebuf);  // 鼠标数据较多，因此初始化一个128字节的缓冲区
    struct MOUSE_DEC mdec;
    
    char *s = NULL;         // 字符串用指针

    // 显示鼠标坐标
    sprintf(s, "(%d, %d)", mx, my);
    putstring8_ascii(binfo->vram, binfo->scrnx, 0, 0, COL8_FFFFFF, s);

    // 允许来自键盘和鼠标的中断
    io_out8(PIC0_IMR, 0xf9);    // 允许PIC1和键盘的中断（11111001）
    io_out8(PIC1_IMR, 0xef);    // 允许鼠标的中断（11101111）

    // 初始化键盘并激活鼠标
    init_keyboard();
    enable_mouse(&mdec);

    while (1) {
        // 先关中断，再作处理，不然就乱了
        io_cli();
        
        if (fifo8_status(&keyfifo) + fifo8_status(&mousefifo) == 0) {
            io_stihlt();
        } else {
            if (fifo8_status(&keyfifo) != 0) {
                i = fifo8_get(&keyfifo); 
                io_sti();
                sprintf(s, "%02x", i);
                boxfill8(binfo->vram, binfo->scrnx, COL8_008484, 0, 16, 15, 31);
                putstring8_ascii(binfo->vram, binfo->scrnx, 0, 16, COL8_FFFFFF, s);
            } else if (fifo8_status(&mousefifo) != 0) {
                // 从缓冲区中取鼠标数据
                i = fifo8_get(&mousefifo);
                io_sti();
                
                if (mouse_decode(&mdec, i) == 1) {  // mouse_decode函数返回1，即phase3已经处理完，可以显示信息
                    // 收集齐鼠标的3Byte数据，将它们显示出来
                    sprintf(s, "[  %4d %4d]", mdec.x, mdec.y);    //第2个字符用于显示按下的鼠标按键
                    // 根据mdec.btn，对第一个字符进行文本替换
                    switch (mdec.btn) {
                        case 1: 
                            s[1] = 'L';
                            break;  // 不写break就顺路执行下去啦
                        case 2: 
                            s[1] = 'R'; 
                            break;
                        case 4: 
                            s[1] = 'M';
                            break;
                        default: 
                            s[1] = ' ';
                    }

                    boxfill8(binfo->vram, binfo->scrnx, COL8_008484, 32, 16, 32 + 8*13 - 1, 31);
                    putstring8_ascii(binfo->vram, binfo->scrnx, 32, 16, COL8_FFFFFF, s);
                }
            }
        }
    }

}

#define PORT_KEYDAT             0x0060
#define PORT_KEYSTA             0x0064
#define PORT_KEYCMD             0x0064
#define KEYSTA_SEND_NOTREADY    0x02
#define KEYCMD_WRITE_MODE       0x60
#define KBC_MODE                0x47

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
void init_keyboard(void) {
    wait_KBC_sendready();                       // 等KBC做好准备
    io_out8(PORT_KEYCMD, KEYCMD_WRITE_MODE);    // 向KBC，发送模式设定指令，即下一条信息是用来设定模式的
    
    wait_KBC_sendready();                       // 等KBC做好准备
    io_out8(PORT_KEYDAT, KBC_MODE);             // 向KBC的数据端口（？）传递0x47，即使用鼠标模式
    
    return;
}

// 鼠标控制电路包含在键盘控制电路中
#define KEYCMD_SENDTO_MOUSE     0xd4
#define MOUSECMD_ENABLE         0xf4

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
    } else if (mdec->phase == 1) {
    // 鼠标传回的第1字节（按键信息）
        if ((data & 0xc8) == 0x08) {
            // 第1字节应当是0x08才正确
            mdec->buf[0] = data;
            mdec->phase = 2;
        }
        return 0;
    } else if (mdec->phase == 2) {
    // 鼠标传回的第2字节（水平移动信息）
        mdec->buf[1] = data;
        mdec->phase = 3;
        return 0;
    } else if (mdec->phase == 3) {
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

