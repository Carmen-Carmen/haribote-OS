// bootpack的主文件
#include <stdio.h>
#include <string.h>
#include "bootpack.h"   // 引入同一个文件夹内的头文件

void make_window8(unsigned char *buf, int xsize, int ysize, char *title);
void make_textbox8(struct SHEET *sht, int x0, int y0, int sx, int sy, int c);

struct TSS32 {
    int backlink, esp0, ss0, esp1, ss1, esp2, ss2, cr3, 
        eip, eflags, eax, ecx, edx, ebx, esp, ebp, esi, edi, 
        es, cs, ss, ds, fs, gs, 
        ldtr, iomap;
};

void task_b_main(void);

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
    binfo = (struct BOOTINFO *) ADR_BOOTINFO;   // asmhead里第一条BOOT_INFO的内存地址

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
    make_window8(buf_win, 160, 52, "window"); 
    sheet_slide(sht_back, 0, 0);// 移动背景至(0, 0)
    // 在window窗口中绘制文本框
    int cursor_x, cursor_c; // 光标位置、光标颜色
    cursor_x = 8;           // 光标初始位置是8（相对于sht_win）
    cursor_c = COL8_FFFFFF;
    make_textbox8(sht_win, 8, 28, 144, 16, COL8_FFFFFF);
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
    
    timer1 = timer_alloc();                 // 分配timer
    timer_init(timer1, &fifo, 10);          // 初始化timer
    timer_settime(timer1, 1000);            // 设定超时时间
    timer2 = timer_alloc();
    timer_init(timer2, &fifo, 3);
    timer_settime(timer2, 300);
    timer3 = timer_alloc();
    timer_init(timer3, &fifo, 1);
    timer_settime(timer3, 50);
    
    // 任务切换
    struct TSS32 tss_a, tss_b;
    struct SEGMENT_DESCRIPTOR *gdt = (struct SEGMENT_DESCRIPTOR *) ADR_GDT;

    tss_a.ldtr = 0;
    tss_a.iomap = 0x40000000;
    tss_b.ldtr = 0;
    tss_b.iomap = 0x40000000;
    set_segmdesc(gdt + 3, 103, (int) &tss_a, AR_TSS32);
    set_segmdesc(gdt + 4, 103, (int) &tss_b, AR_TSS32);
    load_tr(3 * 8);                 // 设置当前运行的任务为a（GDT的3号）
    // 准备tss_b
    int task_b_esp;                 // 为任务B的栈分配64KB的内存
    task_b_esp = memman_alloc_4k(memman, 64 * 1024) + 64 * 1024;    // 栈底的内存地址（栈底是高地址；栈顶是低地址！）
    tss_b.eip = (int) &task_b_main; // 将函数task_b_main的地址赋给任务状态段b的指令指针
    tss_b.eflags = 0x00000202;      // interrupt flag (IF) = 1; 就是STI之后eflags寄存器的值
    tss_b.eax = 0;
    tss_b.ecx = 0;
    tss_b.edx = 0;
    tss_b.ebx = 0;
    tss_b.esp = task_b_esp;         // 为任务B定义的栈空间
    tss_b.ebp = 0;
    tss_b.esi = 0;
    tss_b.edi = 0;
    tss_b.es = 1 * 8;
    tss_b.cs = 2 * 8;               // 代码段寄存器（CS）置为GDT的2号；其余置为GDT的1号
    tss_b.ss = 1 * 8;
    tss_b.ds = 1 * 8;
    tss_b.fs = 1 * 8;
    tss_b.gs = 1 * 8;

    // 初始化一个0.02秒的定时器，用于0.02秒一次的任务切换
    struct TIMER *timer_ts;
    timer_ts = timer_alloc();
    timer_init(timer_ts, &fifo, 2); // timer_ts的标识符（data）设为2
    timer_settime(timer_ts, 2);     // 0.02秒后超时

    // 键盘按键对应字符
    static char keytable[0x54] = {
        0,   0,   '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '-', '=', 0,   0,
        'Q', 'W', 'E', 'R', 'T', 'Y', 'U', 'I', 'O', 'P', '[', ']', 0,   0,   'A', 'S',
        'D', 'F', 'G', 'H', 'J', 'K', 'L', ';', '\'', '`',   0,'\\', 'Z', 'X', 'C', 'V',
        'B', 'N', 'M', ',', '.', '/', 0,   '*', 0,   ' ', 0,   0,   0,   0,   0,   0,
        0,   0,   0,   0,   0,   0,   0,   '7', '8', '9', '-', '4', '5', '6', '+', '1',
        '2', '3', '0', '.'
    };

    // 将sht_back指针地址，找个内存中的地址存进去
    *((int *) 0x0fec) = (int) sht_back;

    // 操作系统的主循环
    while (1) { // 有人喜欢写for(;;)，说是因为编译出来的运行快；但实际上编译器会把while (1)编译成一样快的语句来执行
        // 先关中断，再作处理，不然就乱了
        io_cli();
        
        if (fifo32_status(&fifo) == 0) {
            io_stihlt();
        } else {
            i = fifo32_get(&fifo);
            io_sti();

            if (i == 2) {
                farjmp(0, 4 * 8);           // 跳转到任务b
                timer_settime(timer_ts, 2); // 重新设定0.02秒的定时
            } else if (i >= 256 && i < 512) {
                // 256 <= i < 512: 键盘中断
                i -= 256;
                sprintf(s, "%02x", i);
                putstring8_ascii_sht(sht_back, 0, 16, COL8_FFFFFF, COL8_008484, s);

                // 在文本框中打印出字母
                if (i < 0x54) {
                    if (keytable[i] != 0 && cursor_x < 144) {
                    // 键盘输入的是一般字符
                        s[0] = keytable[i];
                        s[1] = 0;           // 若s指针之前被使用过，s[1]可能存有不想要的数据，要改成0x00
                        putstring8_ascii_sht(sht_win, cursor_x, 28, COL8_000000, COL8_FFFFFF, s);
                        cursor_x += 8;      // 光标向右移动8个像素
                    }   // 否则就不显示
                }
                if (i == 0x0e && cursor_x > 8) {
                    // 按下退格键
                    putstring8_ascii_sht(sht_win, cursor_x, 28, COL8_000000, COL8_FFFFFF, " "); // 用空格把光标消去
                    cursor_x -= 8;
                }
                // 刷新光标
                boxfill8(sht_win->buf, sht_win->bxsize, cursor_c, cursor_x, 28, cursor_x + 7, 43);
                sheet_refresh(sht_win, cursor_x, 28, cursor_x + 8, 44);
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

                    // 移动窗口
                    if ((mdec.btn & 0x01) != 0) {
                        sheet_slide(sht_win, mx - 80, my - 8);  // 先让窗口移动到鼠标左键按下时的位置
                    }
                }
            } else if (i == 10){
                // 10秒定时器中断
                putstring8_ascii_sht(sht_back, 0, 64, COL8_FFFFFF, COL8_008484, "10[sec]");
            } else if (i == 3) {
                // 3秒定时器中断
                putstring8_ascii_sht(sht_back, 0, 80, COL8_FFFFFF, COL8_008484, "3[sec]");
            } else if (i <= 1) {
                // 光标定时器中断
                if (i != 0) {
                    timer_init(timer3, &fifo, 0);
                    cursor_c = COL8_000000;
                } else {
                    // i == 0
                    timer_init(timer3, &fifo, 1);
                    cursor_c = COL8_FFFFFF;
                }
                boxfill8(sht_win->buf, sht_win->bxsize, cursor_c, cursor_x, 28, cursor_x + 7, 43);
                timer_settime(timer3, 50);  // 重新给timer3开始计时
                sheet_refresh(sht_win, cursor_x, 28, cursor_x + 8, 44);
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

/*
    struct SHEEET *sht: 要绘制文本框的图层
    int x0, y0: 起始坐标
    int sx, sy: 长宽
    int c: 文本框颜色
*/
void make_textbox8(struct SHEET *sht, int x0, int y0, int sx, int sy, int c) {
    int x1 = x0 + sx, 
        y1 = y0 + sy;

    boxfill8(sht->buf, sht->bxsize, COL8_848484, x0 - 2, y0 - 3, x1 + 1, y0 - 3);
    boxfill8(sht->buf, sht->bxsize, COL8_848484, x0 - 3, y0 - 3, x0 - 3, y1 + 1);
    boxfill8(sht->buf, sht->bxsize, COL8_FFFFFF, x0 - 3, y1 + 2, x1 + 1, y1 + 2);
    boxfill8(sht->buf, sht->bxsize, COL8_FFFFFF, x1 + 2, y0 - 3, x1 + 2, y1 + 2);
    boxfill8(sht->buf, sht->bxsize, COL8_000000, x0 - 1, y0 - 2, x1 + 0, y0 - 2);
    boxfill8(sht->buf, sht->bxsize, COL8_000000, x0 - 2, y0 - 2, x1 + 0, y1 + 0);
    boxfill8(sht->buf, sht->bxsize, COL8_C6C6C6, x0 - 2, y1 + 1, x1 + 0, y1 + 1);
    boxfill8(sht->buf, sht->bxsize, COL8_C6C6C6, x1 + 1, y0 - 2, x1 + 1, y1 + 1);
    boxfill8(sht->buf, sht->bxsize, c,           x0 - 1, y0 - 1, x1 + 0, y1 + 0);

    return;
}

void task_b_main(void) {
    struct FIFO32 fifo;
    struct TIMER *timer_ts;
    int i, fifobuf[128], count = 0;
    char s[11];
    struct SHEET *sht_back; 

    fifo32_init(&fifo, 128, fifobuf);
    timer_ts = timer_alloc();
    timer_init(timer_ts, &fifo, 1); // 初始化一个timer，5秒后超时
    timer_settime(timer_ts, 2);
    sht_back = (struct SHEET *) *((int *) 0x0fec);  // 从ADR_BOOTINFO - 4取出sht_back的地址，强制转换为SHEET类型指针

    for (;;) {
        count++;
        sprintf(s, "%010d", count);
        putstring8_ascii_sht(sht_back, 0, 144, COL8_FFFFFF, COL8_008484, s);

        io_cli();
        if (fifo32_status(&fifo) == 0) {
            // 没有中断进来（没有超时
            io_sti();
        } else {
            i = fifo32_get(&fifo);
            io_sti();
            if (i == 1) {
               farjmp(0, 3 * 8);    // 切换到任务a
               timer_settime(timer_ts, 2); 
            }
        }
    }
}
