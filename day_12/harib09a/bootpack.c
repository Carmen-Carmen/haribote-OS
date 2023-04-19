// bootpack的主文件
#include <stdio.h>
#include <string.h>
#include "bootpack.h"   // 引入同一个文件夹内的头文件

void make_window8(unsigned char *buf, int xsize, int ysize, char *title);

void HariMain(void) {
    init_gdtidt();  // 初始化GDT和IDT，一定要在PIC之前初始化
    init_pic();     // 初始化PIC
    io_sti();       // IDT和PIC的初始化已经完成了，此时可以解除CPU对于中断的禁止（STI指令）
    init_pit();
    // 允许来自键盘和鼠标的中断
    io_out8(PIC0_IMR, 0xf8);    // 允许PIC0、PIC1和键盘的中断（11111000）
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
    struct SHEET    *sht_back, 
                    *sht_mouse, 
                    *sht_win;
    unsigned char   *buf_back, 
                    buf_mouse[MOUSE_XSIZE * MOUSE_YSIZE], 
                    *buf_win;

    init_palette(); // 初始化调色板 
    struct BOOTINFO *binfo;
    binfo = (struct BOOTINFO *) 0x0ff0; // asmhead里第一条BOOT_INFO的内存地址

    char *s = NULL;         // 字符串用指针

    // 计数器
    int counter = 0;

    // 开启图层管理，之后所有与vram的操作都变成了写入所属图层的缓冲区，而不是直接写入binfo->vram
    shtctl = shtctl_init(memman, binfo->vram, binfo->scrnx, binfo->scrny);  // 初始化图层管理器
    sht_back = sheet_alloc(shtctl);
    sht_mouse = sheet_alloc(shtctl);
    sht_win = sheet_alloc(shtctl);  // 为窗口分配图层
    buf_back = (unsigned char *) memman_alloc_4k(memman, binfo->scrnx * binfo->scrny);  // 给背景图层分配x * y字节大小的内存空间
    buf_win = (unsigned char *) memman_alloc_4k(memman, 160 * 84);
    sheet_setbuf(sht_back, buf_back, binfo->scrnx, binfo->scrny, -1);   // 背景图层没有透明色
    sheet_setbuf(sht_mouse, buf_mouse, MOUSE_XSIZE, MOUSE_YSIZE, 99);   // 鼠标图层透明色号99
    sheet_setbuf(sht_win, buf_win, 160, 84, -1);                        // 窗口图层没有透明色
    init_screen8(buf_back, binfo->scrnx, binfo->scrny);
    init_mouse_cursor8(buf_mouse, 99);  // 初始化鼠标，设定透明色号99
    make_window8(buf_win, 160, 84, "window");   // 在窗口图层缓冲区中打印窗口
    putstring8_ascii(buf_win, 160, 24, 28, COL8_000000, "Welcome to");
    putstring8_ascii(buf_win, 160, 24, 44, COL8_000000, "  Haribote-OS!");
    sheet_slide(sht_back, 0, 0);// 移动背景至(0, 0)
    // 显示鼠标
    int mx, my;
    mx = (binfo->scrnx - 10) / 2;
    my = (binfo->scrny - 28 - 18) / 2;
    sheet_slide(sht_mouse, mx, my); // 移动鼠标至屏幕中央
    sheet_slide(sht_win, 80, 72);
    sheet_updown(sht_back,  0); // 显示背景
    sheet_updown(sht_win, 1);   // 显示窗口
    sheet_updown(sht_mouse, 2); // 显示鼠标
    // 显示鼠标坐标（左上角）
    sprintf(s, "(%d, %d)", mx, my);
    putstring8_ascii(buf_back, binfo->scrnx, 0, 0, COL8_FFFFFF, s);
    // 显示可用内存
    sprintf(s, "memory %d MB, free: %d KB", 
        memtotal / (1024 * 1024), memman_total(memman) / 1024);
    putstring8_ascii(buf_back, binfo->scrnx, 0, 32, COL8_FFFFFF, s);
    sheet_refresh(sht_back, 0, 0, binfo->scrnx, 48);  // 局部刷新back图层中(0, 0) ~ (320, 48)，即3行字的区域
    
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

    // 操作系统的主循环
    while (1) {
        // 打印出counter中的值
        counter++;
        sprintf(s, "%010d", counter);
        boxfill8(buf_win, 160, COL8_C6C6C6, 40, 60, 119, 75);
        putstring8_ascii(buf_win, 160, 40, 60, COL8_000000, s);
        sheet_refresh(sht_win, 40, 60, 120, 76);    // 刷新计数器区域

        // 先关中断，再作处理，不然就乱了
        io_cli();
        
        if (fifo8_status(&keyfifo) + fifo8_status(&mousefifo) == 0) {
            io_sti();
        } else {
            if (fifo8_status(&keyfifo) != 0) {
                // 键盘缓冲区有数据
                i = fifo8_get(&keyfifo); 
                io_sti();
                sprintf(s, "%02x", i);
                boxfill8(buf_back, binfo->scrnx, COL8_008484, 0, 16, 15, 31);
                putstring8_ascii(buf_back, binfo->scrnx, 0, 16, COL8_FFFFFF, s);
                sheet_refresh(sht_back, 0, 16, 16, 32);             // 局部刷新键盘按键信息
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
                    sheet_refresh(sht_back, 32, 16, 32 + 8 * 13, 32);   // 局部刷新鼠标信息

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
                    if (mx > binfo->scrnx - 1) {
                        mx = binfo->scrnx - 1;
                    }
                    if (my > binfo->scrny - 1) {
                        my = binfo->scrny - 1;
                    }
                    // 显示鼠标当前坐标
                    sprintf(s, "(%3d, %3d)", mx, my);
                    boxfill8(buf_back, binfo->scrnx, COL8_008484, 0, 0, 79, 15);             // 隐藏之前的鼠标坐标
                    putstring8_ascii(buf_back, binfo->scrnx, 0, 0, COL8_FFFFFF, s);          // 显示新的坐标
                    sheet_refresh(sht_back, 0, 0, 80, 16);  // 局部刷新鼠标坐标
                    // 通过移动图层来移动鼠标到新的(mx, my)
                    sheet_slide(sht_mouse, mx, my);
                }
            }
        }
    }
}

// 创建窗口的函数
/*
    buf: 窗口图层的缓冲区
    *title: 窗口名称的字符串，指向第一个字符的指针
*/
void make_window8(unsigned char *buf, int xsize, int ysize, char *title) {
    // 关闭按钮
    static char closebtn[14][16] = {
        "OOOOOOOOOOOOOOO@", 
        "OQQQQQQQQQQQQQ$@", 
        "OQQQQQQQQQQQQQ$@", 
        "OQQQ@@QQQQ@@QQ$@", 
        "OQQQQ@@QQ@@QQQ$@", 
        "OQQQQQ@@@@QQQQ$@", 
        "OQQQQQQ@@QQQQQ$@", 
        "OQQQQQ@@@@QQQQ$@",
        "OQQQQ@@QQ@@QQQ$@", 
        "OQQQ@@QQQQ@@QQ$@",
        "OQQQQQQQQQQQQQ$@",
        "OQQQQQQQQQQQQQ$@",
        "O$$$$$$$$$$$$$$@", 
        "@@@@@@@@@@@@@@@@"
    };

    int x, y;
    char c; 
    // 绘制窗口
    boxfill8(buf, xsize, COL8_C6C6C6, 0         , 0         , xsize - 1 , 0         );
    boxfill8(buf, xsize, COL8_FFFFFF, 1         , 1         , xsize - 2 , 1         );
    boxfill8(buf, xsize, COL8_C6C6C6, 0         , 0         , 0         , ysize - 1 );
    boxfill8(buf, xsize, COL8_FFFFFF, 1         , 1         , 1         , ysize - 2 );
    boxfill8(buf, xsize, COL8_848484, xsize - 2 , 1         , xsize - 2 , ysize - 2 );
    boxfill8(buf, xsize, COL8_000000, xsize - 1 , 0         , xsize - 1 , ysize - 1 ); 
    boxfill8(buf, xsize, COL8_C6C6C6, 2         , 2         , xsize - 3 , ysize - 3 ); 
    boxfill8(buf, xsize, COL8_000084, 3         , 3         , xsize - 4 , 20        );  // 标题栏
    boxfill8(buf, xsize, COL8_848484, 1         , ysize - 2 , xsize - 2 , ysize - 2 );
    boxfill8(buf, xsize, COL8_000000, 0         , ysize - 1 , xsize - 1 , ysize - 1 );
    // 打印窗口标题，打印在窗口的中间吧
    putstring8_ascii(buf, xsize, (xsize - strlen(title) * 8) / 2, 4, COL8_FFFFFF, title);   // 使用string.h中strlen得到title长度
    // 在窗口左上角打印关闭按钮
    for (y = 0; y < 14; y++) {
        for (x = 0; x < 16;x++) {
            c = closebtn[y][x]; 
            if (c == '@') {
                c = COL8_000000;
            } else if (c == '$') {
                c = COL8_848484; 
            } else if (c == 'Q') {
                c = COL8_C6C6C6;
            } else {
                c = COL8_FFFFFF;
            }
            // 左上角坐标大概是(5, 5)，当前像素的坐标为(5 + x, 5 + y)
            buf[5 + x + (5 + y) * xsize] = c;
            // buf[(5 + y) * xsize + (xsize - 21) + x] = c;
        }
    }
    return;
}


