// bootpack的主文件
#include <stdio.h>
#include <string.h>
#include "bootpack.h"   // 引入同一个文件夹内的头文件

void make_window8(unsigned char *buf, int xsize, int ysize, char *title);

void set490(struct FIFO32 *fifo, int mode); 

void HariMain(void) {
    init_gdtidt();  // 初始化GDT和IDT，一定要在PIC之前初始化
    init_pic();     // 初始化PIC
    io_sti();       // IDT和PIC的初始化已经完成了，此时可以解除CPU对于中断的禁止（STI指令）
    init_pit();

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

    // 开启图层管理，之后所有与vram的操作都变成了写入所属图层的缓冲区，而不是直接写入binfo->vram
    shtctl = shtctl_init(memman, binfo->vram, binfo->scrnx, binfo->scrny);  // 初始化图层管理器
    sht_back = sheet_alloc(shtctl);
    sht_mouse = sheet_alloc(shtctl);
    sht_win = sheet_alloc(shtctl);  // 为窗口分配图层
    buf_back = (unsigned char *) memman_alloc_4k(memman, binfo->scrnx * binfo->scrny);  // 给背景图层分配x * y字节大小的内存空间
    buf_win = (unsigned char *) memman_alloc_4k(memman, 160 * 52);
    sheet_setbuf(sht_back, buf_back, binfo->scrnx, binfo->scrny, -1);   // 背景图层没有透明色
    sheet_setbuf(sht_mouse, buf_mouse, MOUSE_XSIZE, MOUSE_YSIZE, 99);   // 鼠标图层透明色号99
    sheet_setbuf(sht_win, buf_win, 160, 52, -1);                        // 窗口图层没有透明色
    init_screen8(buf_back, binfo->scrnx, binfo->scrny);
    init_mouse_cursor8(buf_mouse, 99);  // 初始化鼠标，设定透明色号99
    make_window8(buf_win, 160, 52, "counter"); 
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
    sprintf(s, "(%3d, %3d)", mx, my);
    putstring8_ascii_sht(sht_back, 0, 0, COL8_FFFFFF, COL8_008484, s);
    // 显示可用内存
    sprintf(s, "memory %d MB, free: %d KB", 
        memtotal / (1024 * 1024), memman_total(memman) / 1024);
    putstring8_ascii_sht(sht_back, 0, 32, COL8_FFFFFF, COL8_008484, s);
    
    int i;                  // 存放从缓冲区中取出数据的变量

    struct FIFO32 fifo;     // 32位的fifo缓冲区，同时用作鼠标和键盘中断的数据缓冲
    int fifobuf[128];       
    fifo32_init(&fifo, 128, fifobuf);
    struct MOUSE_DEC mdec;

    // 初始化键盘，激活鼠标
    init_keyboard(&fifo, 256);          // keyboard: 256 ~ 511
    enable_mouse(&fifo, 512, &mdec);    // mouse: 512 ~ 767
    
    io_out8(PIC0_IMR, 0xf8);    // 允许PIC0、PIC1和键盘的中断（11111000）
    io_out8(PIC1_IMR, 0xef);    // 允许鼠标的中断（11101111）

    // 超时处理
    // 共用1个FIFO缓冲区
    struct TIMER *timer1, *timer2, *timer3;

    set490(&fifo, 1);                       // 设置490个50天后超时的timer
    
    timer1 = timer_alloc();                 // 分配timer
    timer_init(timer1, &fifo, 10);          // 初始化timer
    timer_settime(timer1, 1000);            // 设定超时时间
    timer2 = timer_alloc();
    timer_init(timer2, &fifo, 3);
    timer_settime(timer2, 300);
    timer3 = timer_alloc();
    timer_init(timer3, &fifo, 1);
    timer_settime(timer3, 50);

    int count = 0;
    // 操作系统的主循环
    while (1) { // 有人喜欢写for(;;)，说是因为编译出来的运行快；但实际上编译器会把while (1)编译成一样快的语句来执行
        sprintf(s, "%010d", timerctl.count);
        putstring8_ascii_sht(sht_win, 40, 28, COL8_000000, COL8_C6C6C6, s);
        // count++;

        // 先关中断，再作处理，不然就乱了
        io_cli();
        
        if (fifo32_status(&fifo) == 0) {
            io_sti();
        } else {
            i = fifo32_get(&fifo);
            io_sti();
            if (i >= 256 && i < 512) {
                // 256 <= i < 512: 键盘中断
                i -= 256;
                sprintf(s, "%02x", i);
                putstring8_ascii_sht(sht_back, 0, 16, COL8_FFFFFF, COL8_008484, s);
            } else if (i >= 512 && i < 768) {
                // 512 <= i < 768: 鼠标中断
                i -= 512;
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
                    
                    putstring8_ascii_sht(sht_back, 32, 16, COL8_FFFFFF, COL8_008484, s);

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
                    putstring8_ascii_sht(sht_back, 0, 0, COL8_FFFFFF, COL8_008484, s);
                    // 通过移动图层来移动鼠标到新的(mx, my)
                    sheet_slide(sht_mouse, mx, my);
                }
            } else if (i == 10){
                // 10秒定时器中断
                putstring8_ascii_sht(sht_back, 0, 64, COL8_FFFFFF, COL8_008484, "10[sec]");
                // 10秒之后，显示全速运行后的count的值
                // sprintf(s, "%010d", count);
                // putstring8_ascii_sht(sht_win, 40, 28, COL8_000000, COL8_C6C6C6, s);
            } else if (i == 3) {
                // 3秒定时器中断
                putstring8_ascii_sht(sht_back, 0, 80, COL8_FFFFFF, COL8_008484, "3[sec]");
                // count = 0;  // 开始测定！
            } else if (i == 1) {
                // 光标定时器中断
                timer_init(timer3, &fifo, 0);
                boxfill8(buf_back, binfo->scrnx, COL8_FFFFFF, 8, 96, 15, 111);
                timer_settime(timer3, 50);  // 重新给timer3开始计时
                sheet_refresh(sht_back, 8, 96, 16, 112);
            } else if (i == 0) {
                // 光标定时器中断
                timer_init(timer3, &fifo, 1);
                boxfill8(buf_back, binfo->scrnx, COL8_008484, 8, 96, 15, 111);
                timer_settime(timer3, 50);  // 重新给timer3开始计时
                sheet_refresh(sht_back, 8, 96, 16, 112);
            }
        }

        // 每隔一年(timerctl.count = 365 * 24 * 3600 * 100)，调整一次时间
        if (timerctl.count == 0xbbf81e00) {
            reset_timerctl_count();
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

    // 获取title长度
    int title_len = 0;
    char *title_copy = title;
    while (*title_copy != 0x00) {
        title_len++;
        title_copy++;
    }

    // 打印窗口标题，打印在窗口的中间吧
    putstring8_ascii(buf, xsize, (xsize - title_len * 8) / 2, 4, COL8_FFFFFF, title); 
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

void set490(struct FIFO32 *fifo, int mode) {
    int i;
    struct TIMER *timer;
    if (mode != 0) {
        for (i = 0; i < 490; i++) {
            timer = timer_alloc();
            timer_init(timer, fifo, 1024 + i);
            timer_settime(timer, 50 * 24 * 60 * 60 * 100);  // 设定50天后超时
        }
    }

    return;
}
