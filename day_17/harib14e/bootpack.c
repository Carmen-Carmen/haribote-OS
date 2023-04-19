// bootpack的主文件
#include <stdio.h>
#include <string.h>
#include "bootpack.h"   // 引入同一个文件夹内的头文件

void make_window8(unsigned char *buf, int xsize, int ysize, char *title, char act);
void make_textbox8(struct SHEET *sht, int x0, int y0, int sx, int sy, int c);
void make_wtitle8(unsigned char *buf, int xsize, char *title, char act);

void console_task(struct SHEET *sheet);

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

    // 初始化缓冲区
    struct FIFO32 fifo;     // 32位的fifo缓冲区，同时用作鼠标和键盘中断的数据缓冲
    int fifobuf[128];       
    fifo32_init(&fifo, 128, fifobuf, 0);// 此时多任务初始化还未完成，先把task属性置为0
    struct MOUSE_DEC mdec;

    int i;                  // 存放从缓冲区中取出数据的变量
    char *s = NULL;             // 字符串用指针

    // 开启任务切换
    struct TASK *task_a, *task_cons;
    task_a = task_init(memman);     // 开启多任务，且开启多任务的这个任务用task_a接收
    fifo.task = task_a;             // 将task_a存进fifo
    task_run(task_a, 1, 2);         // 设定task_a为level 1

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

    shtctl = shtctl_init(memman, binfo->vram, binfo->scrnx, binfo->scrny);  // 初始化图层管理器

    // sht_back
    sht_back = sheet_alloc(shtctl);
    buf_back = (unsigned char *) memman_alloc_4k(memman, binfo->scrnx * binfo->scrny);  // 给背景图层分配x * y字节大小的内存空间
    sheet_setbuf(sht_back, buf_back, binfo->scrnx, binfo->scrny, -1);   // 背景图层没有透明色
    init_screen8(buf_back, binfo->scrnx, binfo->scrny);

    // sht_cons
    struct SHEET *sht_cons; // 创建一个长度为3的图层数组
    unsigned char *buf_cons;
    sht_cons = sheet_alloc(shtctl);
    buf_cons = (unsigned char *) memman_alloc_4k(memman, 256 * 165);
    sheet_setbuf(sht_cons, buf_cons, 256, 165, -1);
    sprintf(s, "task_b%d", i);
    make_window8(buf_cons, 256, 165, "console", 0);
    make_textbox8(sht_cons, 8, 28, 240, 128, COL8_000000);
    task_cons = task_alloc();
    task_cons->tss.esp = memman_alloc_4k(memman, 64 * 1024) + 64 * 1024 - 8;   // 分配栈内存
    task_cons->tss.eip = (int) &console_task;   // 指令指针指向console_task
    task_cons->tss.es = 1 * 8;
    task_cons->tss.cs = 2 * 8;
    task_cons->tss.ss = 1 * 8;
    task_cons->tss.ds = 1 * 8;
    task_cons->tss.fs = 1 * 8;
    task_cons->tss.gs = 1 * 8;
    *((int *) (task_cons->tss.esp + 4)) = (int) sht_cons;  
    task_run(task_cons, 2, 2);  // level = 2, priority = 2
    
    // sht_win
    sht_win = sheet_alloc(shtctl);  // 为窗口分配图层
    buf_win = (unsigned char *) memman_alloc_4k(memman, 144 * 52);
    sheet_setbuf(sht_win, buf_win, 144, 52, -1);                        // 窗口图层没有透明色
    make_window8(buf_win, 144, 52, "window", 1); 
    // 在window窗口中绘制文本框
    make_textbox8(sht_win, 8, 28, 128, 16, COL8_FFFFFF);
    int cursor_x, cursor_c; // 光标位置、光标颜色
    cursor_x = 8;           // 光标初始位置是8（相对于sht_win）
    cursor_c = COL8_FFFFFF;
    // cursor的定时器
    struct TIMER *timer;
    timer = timer_alloc();
    timer_init(timer, &fifo, 1);
    timer_settime(timer, 50);

    //sht_mouse
    sht_mouse = sheet_alloc(shtctl);
    sheet_setbuf(sht_mouse, buf_mouse, MOUSE_XSIZE, MOUSE_YSIZE, 99);   // 鼠标图层透明色号99
    init_mouse_cursor8(buf_mouse, 99);  // 初始化鼠标，设定透明色号99
    // 显示鼠标
    int mx, my;
    mx = (binfo->scrnx - 10) / 2;
    my = (binfo->scrny - 28 - 18) / 2;

    // 显示各个窗口以及信息
    sheet_slide(sht_back, 0, 0);    // 移动背景至(0, 0)
    sheet_slide(sht_mouse, mx, my); // 移动鼠标至屏幕中央
    // sht_win, sht_win_b[0~2]分别位于左上、右上、左下、右下
    sheet_slide(sht_win,    8,  56);
    sheet_slide(sht_cons,   mx - sht_cons->bxsize / 2, 
                            my - sht_cons->bysize / 2); // console也移动到屏幕中央
    sheet_updown(sht_back,  0);  // 显示背景
    sheet_updown(sht_cons,  1);
    sheet_updown(sht_win,   2); 
    sheet_updown(sht_mouse, 3);  // 显示鼠标
    // 显示鼠标坐标（左上角）
    sprintf(s, "(%3d, %3d)", mx, my);
    putstring8_ascii_sht(sht_back, 0, 0, COL8_FFFFFF, COL8_008484, s);
    // 显示可用内存
    sprintf(s, "memory %d MB, free: %d KB", 
        memtotal / (1024 * 1024), memman_total(memman) / 1024);
    putstring8_ascii_sht(sht_back, 0, 32, COL8_FFFFFF, COL8_008484, s);
    // 显示这个image来自哪个项目
    sprintf(s, "harib14e");
    putstring8_ascii_sht(   sht_back, 
                            sht_back->bxsize - 8 * 8, 
                            sht_back->bysize - 28 - 16, 
                            COL8_C6C6C6, COL8_008484, s);


    // 初始化键盘，激活鼠标
    init_keyboard(&fifo, 256);          // keyboard: 256 ~ 511
    enable_mouse(&fifo, 512, &mdec);    // mouse: 512 ~ 767
    
    io_out8(PIC0_IMR, 0xf8);    // 允许PIC0、PIC1和键盘的中断（11111000）
    io_out8(PIC1_IMR, 0xef);    // 允许鼠标的中断（11101111）

    // 键盘按键对应字符
    static char keytable0[0x80] = {
        0,   0,   '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '-', '=', 0,   0,
        'q', 'w', 'e', 'r', 't', 'y', 'u', 'i', 'o', 'p', '[', ']', 0,   0,   'a', 's',
        'd', 'f', 'g', 'h', 'j', 'k', 'l', ';', '\'', '`',   0,'\\', 'z', 'x', 'c', 'v',
        'b', 'n', 'm', ',', '.', '/', 0,   '*', 0,   ' ', 0,   0,   0,   0,   0,   0,
        0,   0,   0,   0,   0,   0,   0,   '7', '8', '9', '-', '4', '5', '6', '+', '1',
        '2', '3', '0', '.', 0,   0,   0,    0,    0,   0,   0,   0,   0,   0,   0,   0, 
        0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
	0,   0,   0,   0x5c, 0,  0,   0,   0,   0,   0,   0,   0,   0,   0x5c, 0,  0
    };
    static char keytable1[0x80] = {
        0,   0,   '!', '@', '#', '$', '%', '^', '&', '*', '(', ')', '_', '+', 0,   0,
        'Q', 'W', 'E', 'R', 'T', 'Y', 'U', 'I', 'O', 'P', '{', '}', 0,   0,   'A', 'S',
        'D', 'F', 'G', 'H', 'J', 'K', 'L', ';', '\"','~',   0, '|', 'Z', 'X', 'C', 'V',
        'B', 'N', 'M', '<', '>', '?', 0,   '*', 0,   ' ', 0,   0,   0,   0,   0,   0,
        0,   0,   0,   0,   0,   0,   0,   '7', '8', '9', '-', '4', '5', '6', '+', '1',
        '2', '3', '0', '.', 0,   0,   0,    0,    0,   0,   0,   0,   0,   0,   0,   0, 
        0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
	0,   0,   0,   0x5c, 0,  0,   0,   0,   0,   0,   0,   0,   0,   0x5c, 0,  0
    };
    int key_to = 0,     // 向哪个窗口输入
        key_shift = 0;  // 是否按下shift: left shift (1), right shift (2), left right shift (3)

    // 操作系统的主循环
    while (1) { 
        // 先关中断，再作处理，不然就乱了
        io_cli();

        if (fifo32_status(&fifo) == 0) {
            task_sleep(task_a); // 先task_sleep，防止进行休眠处理时又发生中断请求，导致FIFO数据改变
            io_sti();
        } else {
            i = fifo32_get(&fifo);
            io_sti();

            if (i >= 256 && i < 512) {
                // 256 <= i < 512: 键盘中断
                i -= 256;
                sprintf(s, "%02x", i);
                putstring8_ascii_sht(sht_back, 0, 16, COL8_FFFFFF, COL8_008484, s);

                // 在文本框中打印出字母
                if (i < 0x80) { // i是键盘按键的编码
                    // 判断根据是否按下shift，分别从2个数组中解析i
                    if (key_shift == 0) {
                        s[0] = keytable0[i];
                    } else {
                        s[0] = keytable1[i];
                    }
                } else {        // i非键盘按键编码，将字符串指针开头置为0
                    s[0] = 0;
                }

                if (s[0] != 0) {// 是一般字符
                    if (key_to == 0) {
                        if (cursor_x < 128) {
                            s[1] = 0;
                            putstring8_ascii_sht(sht_win, cursor_x, 28, COL8_000000, COL8_FFFFFF, s);
                            cursor_x += 8;
                        }
                    } else {
                        fifo32_put(&task_cons->fifo, s[0] + 256);
                    }
                }

                if (i == 0x0e) {    // 按下退格键
                    if (key_to == 0) {
                        if (cursor_x > 8) {
                            putstring8_ascii_sht(sht_win, cursor_x, 28, COL8_000000, COL8_FFFFFF, " "); // 用空格把光标消去
                            cursor_x -= 8;
                        }
                    } else {
                        fifo32_put(&task_cons->fifo, 8 + 256);
                    }
                } 
                
                if (i == 0x0f) {    // 如果按下tab键，就将输入窗口切换到命令行窗口去
                    if (key_to == 0) {
                        key_to = 1;
                        make_wtitle8(buf_win, sht_win->bxsize, "task_a", 0);
                        make_wtitle8(buf_cons, sht_cons->bxsize, "console", 1);
                    } else {
                        key_to = 0;
                        make_wtitle8(buf_win, sht_win->bxsize, "task_a", 1);
                        make_wtitle8(buf_cons, sht_cons->bxsize, "console", 0);
                    }
                    sheet_refresh(sht_win, 0, 0, sht_win->bxsize, 21);  // 21是标题栏的高度
                    sheet_refresh(sht_cons, 0, 0, sht_cons->bxsize, 21);
                }

                if (i == 0x2a) key_shift |= 1;  // left shift on
                if (i == 0x36) key_shift |= 2;  // right shigt on
                if (i == 0xaa) key_shift &= ~1; // left shift off
                if (i == 0xb6) key_shift &= ~2; // right shift off

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
            } else if (i <= 1) {
                // 光标定时器中断
                if (i != 0) {
                    timer_init(timer, &fifo, 0);
                    cursor_c = COL8_000000;
                } else {
                    // i == 0
                    timer_init(timer, &fifo, 1);
                    cursor_c = COL8_FFFFFF;
                }
                boxfill8(sht_win->buf, sht_win->bxsize, cursor_c, cursor_x, 28, cursor_x + 7, 43);
                timer_settime(timer, 50);  // 重新给timer3开始计时
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
    act: 该窗口是否被focus
*/
void make_window8(unsigned char *buf, int xsize, int ysize, char *title, char act) {
    // 绘制窗口
    boxfill8(buf, xsize, COL8_C6C6C6, 0         , 0         , xsize - 1 , 0         );
    boxfill8(buf, xsize, COL8_FFFFFF, 1         , 1         , xsize - 2 , 1         );
    boxfill8(buf, xsize, COL8_C6C6C6, 0         , 0         , 0         , ysize - 1 );
    boxfill8(buf, xsize, COL8_FFFFFF, 1         , 1         , 1         , ysize - 2 );
    boxfill8(buf, xsize, COL8_848484, xsize - 2 , 1         , xsize - 2 , ysize - 2 );
    boxfill8(buf, xsize, COL8_000000, xsize - 1 , 0         , xsize - 1 , ysize - 1 ); 
    boxfill8(buf, xsize, COL8_C6C6C6, 2         , 2         , xsize - 3 , ysize - 3 ); 
    boxfill8(buf, xsize, COL8_848484, 1         , ysize - 2 , xsize - 2 , ysize - 2 );
    boxfill8(buf, xsize, COL8_000000, 0         , ysize - 1 , xsize - 1 , ysize - 1 );

    make_wtitle8(buf, xsize, title, act);

    return;
}

// 绘制窗口标题栏
void make_wtitle8(unsigned char *buf, int xsize, char *title, char act) {
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
    char c, 
        tc,         // title color
        tbc;        // title background color
    if (act != 0) { //这个window被focus了
        tc = COL8_FFFFFF;
        tbc = COL8_000084;
    } else {
        tc = COL8_C6C6C6;
        tbc = COL8_848484;
    }
    boxfill8(buf, xsize, tbc        , 3         , 3         , xsize - 4 , 20        );  // 标题栏，被focus时是深蓝色；否则是灰色

    // 获取title长度
    int title_len = 0;
    char *title_copy = title;
    while (*title_copy != 0x00) {
        title_len++;
        title_copy++;
    }
    // 打印窗口标题，打印在窗口的中间
    putstring8_ascii(buf, xsize, (xsize - title_len * 8) / 2, 4, tc, title);            // 标题，被focus时是黑色；否则是浅灰色

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

// 命令行窗口任务的入口函数
void console_task(struct SHEET *sheet) {
    struct TIMER *timer;
    struct TASK *task = task_now();
    int i, fifobuf[128], cursor_x = 24, cursor_c = COL8_000000;
    char *s = NULL;

    fifo32_init(&task->fifo, 128, fifobuf, task); // 有数据写入时，唤醒task_cons
    timer = timer_alloc();
    timer_init(timer, &task->fifo, 1);
    timer_settime(timer, 50);

    // 显示提示符 $ 
    putstring8_ascii_sht(sheet, 8, 28, COL8_FFFFFF, COL8_000000, "$ ");

    for (;;) {
        io_cli();
        if (fifo32_status(&task->fifo) == 0) {
            task_sleep(task);
            io_sti();
        } else {
            i = fifo32_get(&task->fifo);
            io_sti();

            if (i <= 1) {
                // data = 1，说明是光标定时器
                if (i != 0) {
                    timer_init(timer, &task->fifo, 0);
                    cursor_c = COL8_FFFFFF;
                } else {
                    timer_init(timer, &task->fifo, 1);
                    cursor_c = COL8_000000;
                }
                timer_settime(timer, 50);

            } else if (256 <= i && i <= 511) {
                // 证明是task_a传来的键盘数据
                i -= 256;

                if (i == 8) {   // 退格键
                    if (cursor_x > 24) {    // 大于16是为了不要把命令行提示符也给删掉
                        putstring8_ascii_sht(sheet, cursor_x, 28, COL8_FFFFFF, COL8_000000, " ");
                        cursor_x -= 8;
                    }
                } else {        // 一般字符
                    if (cursor_x < 240) {
                        s[0] = i;
                        s[1] = 0;
                        putstring8_ascii_sht(sheet, cursor_x, 28, COL8_FFFFFF, COL8_000000, s);
                        cursor_x += 8;
                    }
                }
            }
            // 重新显示光标
            boxfill8(sheet->buf, sheet->bxsize, cursor_c, cursor_x, 28, cursor_x + 7, 43);
            sheet_refresh(sheet, cursor_x, 28, cursor_x + 8, 44);
        }
    }
}
