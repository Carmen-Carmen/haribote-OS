// bootpack的主文件
#include <stdio.h>
#include "bootpack.h"   // 引入同一个文件夹内的头文件

void HariMain(void) {
    init_gdtidt();  // 初始化GDT和IDT，一定要在PIC之前初始化
    init_pic();     // 初始化PIC
    io_sti();       // IDT和PIC的初始化已经完成了，此时可以解除CPU对于中断的禁止（STI指令）
    // 允许来自键盘和鼠标的中断
    io_out8(PIC0_IMR, 0xf9);    // 允许PIC1和键盘的中断（11111001）
    io_out8(PIC1_IMR, 0xef);    // 允许鼠标的中断（11101111）

    // 开启内存管理，初始化GDT和IDT之后就要开启，因为后面管理图层等需要用到MEMMAN结构体
    unsigned int memtotal;
    struct MEMMAN *memman = (struct MEMMAN *) MEMMAN_ADDR;
    
    memtotal = memtest(0x00400000, 0xbfffffff);     
    memman_init(memman);
    memman_free(memman, 0x00001000, 0x0009e000);     // 释放0x00001000 ~ 0x0009efff的内存区域
    memman_free(memman, 0x00400000, memtotal - 0x00400000);     // 释放0x00400000开始直到memtotal的内存

    // 图层管理变量声明
    struct SHTCTL *shtctl;
    struct SHEET *sht_back, *sht_mouse; // 底部图层和鼠标图层
    unsigned char *buf_back, buf_mouse[256];

    init_palette(); // 初始化调色板 
    struct BOOTINFO *binfo;
    binfo = (struct BOOTINFO *) 0x0ff0; // asmhead里第一条BOOT_INFO的内存地址

    char *s = NULL;         // 字符串用指针

    // 开启图层管理，之后所有与vram的操作都变成了写入所属图层的缓冲区，而不是直接写入binfo->vram
    shtctl = shtctl_init(memman, binfo->vram, binfo->scrnx, binfo->scrny);  // 初始化图层管理器
    sht_back = sheet_alloc(shtctl);
    sht_mouse = sheet_alloc(shtctl);
    buf_back = (unsigned char *) memman_alloc_4k(memman, binfo->scrnx * binfo->scrny);  // 给背景图层分配x * y字节大小的内存空间
    sheet_setbuf(sht_back, buf_back, binfo->scrnx, binfo->scrny, -1);   // 背景图层没有透明色
    sheet_setbuf(sht_mouse, buf_mouse, 16, 16, 99);                     // 鼠标图层透明色号99
    init_screen8(buf_back, binfo->scrnx, binfo->scrny);
    init_mouse_cursor8(buf_mouse, 99);  // 初始化鼠标，设定透明色号99
    sheet_slide(shtctl, sht_back, 0, 0);// 移动背景至(0, 0)
    // 显示鼠标（不再通过putblock8，而是通过图层来显示）
    int mx, my;
    mx = (binfo->scrnx - 16) / 2;
    my = (binfo->scrny - 28 - 16) / 2;
    sheet_slide(shtctl, sht_mouse, mx, my); // 移动鼠标至屏幕中央
    sheet_updown(shtctl, sht_back, 0);  // 显示背景
    sheet_updown(shtctl, sht_mouse, 1); // 显示鼠标
    // 显示鼠标坐标（左上角）
    sprintf(s, "(%d, %d)", mx, my);
    putstring8_ascii(buf_back, binfo->scrnx, 0, 0, COL8_FFFFFF, s);
    // 显示可用内存
    sprintf(s, "memory %d MB, free: %d KB", 
        memtotal / (1024 * 1024), memman_total(memman) / 1024);
    putstring8_ascii(buf_back, binfo->scrnx, 0, 32, COL8_FFFFFF, s);
    
    int i;                  // 存放从缓冲区中取出数据的变量

    // 用于显示键盘信息
    char keybuf[32];        // 在初始化缓冲区时真正赋予缓冲区内存空间
    fifo8_init(&keyfifo, 32, keybuf);   // "&"是取地址运算符，把结构体的内存地址作为参数传入

    // 用于显示鼠标信息
    char mousebuf[256];     // 应当用定长空数组的形式初始化缓冲空间，否则加载很慢
    fifo8_init(&mousefifo, 256, mousebuf);
    struct MOUSE_DEC mdec;

    // 初始化键盘，激活鼠标
    init_keyboard();
    enable_mouse(&mdec);
    
    sheet_refresh(shtctl);  // 刷新

    // 操作系统的主循环
    while (1) {
        // 先关中断，再作处理，不然就乱了
        io_cli();
        
        if (fifo8_status(&keyfifo) + fifo8_status(&mousefifo) == 0) {
            io_stihlt();    // 鼠标和键盘的缓冲区中都没有数据，开中断并halt
        } else {
            if (fifo8_status(&keyfifo) != 0) {
                // 键盘缓冲区有数据
                i = fifo8_get(&keyfifo); 
                io_sti();
                sprintf(s, "%02x", i);
                boxfill8(buf_back, binfo->scrnx, COL8_008484, 0, 16, 15, 31);
                putstring8_ascii(buf_back, binfo->scrnx, 0, 16, COL8_FFFFFF, s);
                sheet_refresh(shtctl);
            } else if (fifo8_status(&mousefifo) != 0) {
                // 鼠标缓冲区有数据
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

                    boxfill8(buf_back, binfo->scrnx, COL8_008484, 32, 16, 32 + 8*13 - 1, 31);
                    putstring8_ascii(buf_back, binfo->scrnx, 32, 16, COL8_FFFFFF, s);

                    // 鼠标的移动
                    // 计算当前坐标：mdec.x和mdec.y中存放的是鼠标相较于上一个位置的相对位置变化
                    mx += mdec.x;
                    my += mdec.y;
                    if (mx < 0) {
                        mx = 0;
                    }
                    if (my < 0) {
                        my = 0;
                    }
                    if (mx > binfo->scrnx - 16) {
                        mx = binfo->scrnx - 16;
                    }
                    if (my > binfo->scrny - 16) {
                        my = binfo->scrny - 16;
                    }
                    // 显示鼠标当前坐标
                    sprintf(s, "(%3d, %3d)", mx, my);
                    boxfill8(buf_back, binfo->scrnx, COL8_008484, 0, 0, 79, 15);             // 隐藏之前的鼠标坐标
                    putstring8_ascii(buf_back, binfo->scrnx, 0, 0, COL8_FFFFFF, s);          // 显示新的坐标
                    // 通过移动图层来移动鼠标到新的(mx, my)
                    sheet_slide(shtctl, sht_mouse, mx, my);
                }
            }
        }
    }
}
