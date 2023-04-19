// bootpack的主文件
#include <stdio.h>
#include <string.h>
#include "bootpack.h"   // 引入同一个文件夹内的头文件

#define KEYCMD_LED      0xed    // 0b11101101，控制键盘LED灯的指令码？

void make_window8(unsigned char *buf, int xsize, int ysize, char *title, char act);
void make_textbox8(struct SHEET *sht, int x0, int y0, int sx, int sy, int c);
void make_wtitle8(unsigned char *buf, int xsize, char *title, char act);

void console_task(struct SHEET *sheet);
void scroll_down(struct SHEET *sheet, int x_init, int y_init, int xsize, int ysize);

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
    struct FIFO32 fifo, keycmd;
    int fifobuf[128], keycmd_buf[32];       
    fifo32_init(&fifo, 128, fifobuf, 0);// 此时多任务初始化还未完成，先把task属性置为0
    fifo32_init(&keycmd, 32, keycmd_buf, 0);
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
    sprintf(s, "harib15d");
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
        'Q', 'W', 'E', 'R', 'T', 'Y', 'U', 'I', 'O', 'P', '[', ']', 0,   0,   'A', 'S',
        'D', 'F', 'G', 'H', 'J', 'K', 'L', ';', '\'', '`',   0,'\\', 'Z', 'X', 'C', 'V',
        'B', 'N', 'M', ',', '.', '/', 0,   '*', 0,   ' ', 0,   0,   0,   0,   0,   0,
        0,   0,   0,   0,   0,   0,   0,   '7', '8', '9', '-', '4', '5', '6', '+', '1',
        '2', '3', '0', '.', 0,   0,   0,    0,    0,   0,   0,   0,   0,   0,   0,   0, 
        0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
	0,   0,   0,   0x5c, 0,  0,   0,   0,   0,   0,   0,   0,   0,   0x5c, 0,  0
    };
    static char keytable1[0x80] = {
        0,   0,   '!', '@', '#', '$', '%', '^', '&', '*', '(', ')', '_', '+', 0,   0,
        'Q', 'W', 'E', 'R', 'T', 'Y', 'U', 'I', 'O', 'P', '{', '}', 0,   0,   'A', 'S',
        'D', 'F', 'G', 'H', 'J', 'K', 'L', ':', '\"','~',   0, '|', 'Z', 'X', 'C', 'V',
        'B', 'N', 'M', '<', '>', '?', 0,   '*', 0,   ' ', 0,   0,   0,   0,   0,   0,
        0,   0,   0,   0,   0,   0,   0,   '7', '8', '9', '-', '4', '5', '6', '+', '1',
        '2', '3', '0', '.', 0,   0,   0,    0,    0,   0,   0,   0,   0,   0,   0,   0, 
        0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
	0,   0,   0,   0x5c, 0,  0,   0,   0,   0,   0,   0,   0,   0,   0x5c, 0,  0
    };
    int key_to = 0,     // 向哪个窗口输入
        key_shift = 0,  // 是否按下shift: left shift (1), right shift (2), left right shift (3)
        key_leds = (binfo->leds >> 4) & 7,  // 第4位 ScrollLock，第5位 NumLock，第6位 CapsLock；右移4位后与0b111进行与运算，即只保留3位的这三个锁定键信息
        keycmd_wait = -1;
    fifo32_put(&keycmd, KEYCMD_LED);
    fifo32_put(&keycmd, key_leds);

    // 操作系统的主循环
    while (1) { 
        // 如果存在向键盘控制器发送的数据，就发送
        if (fifo32_status(&keycmd) > 0 && keycmd_wait < 0) {
            keycmd_wait = fifo32_get(&keycmd);
            wait_KBC_sendready();
            io_out8(PORT_KEYDAT, keycmd_wait);
        }

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

                // 在文本框中打印出字符
                /*
                    整体逻辑：
                    - 按下shift时，字母大写，其他符号也应是上面那个
                    - CapsLock锁定时
                        - 没按下shift，字母大写，其他符号是下面那个
                        - 按下shift，字母小写，其他符号是上面那个（这个好像是windows的逻辑，mac并不是这样...
                */
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

                if ('A' <= s[0] && s[0] <= 'Z') {   // 输入为字母时，根据CapsLock状态以及shift状态，将s[0]变为小写字母
                    if (((key_leds & 4) == 0 && key_shift == 0) || 
                        ((key_leds & 4) != 0 && key_shift != 0)) {
                        s[0] += 0x20; // 小写字母ascii码比对应大写字母大0x20 
                    }

                }

                if (s[0] != 0) {    // 是一般字符
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
                        fifo32_put(&task_cons->fifo, 8 + 256);  // "8"是ASCII码中的退格
                    }
                } 
                
                if (i == 0x0f) {    // 如果按下tab键，就将输入窗口切换到命令行窗口去
                    if (key_to == 0) {
                        key_to = 1;
                        make_wtitle8(buf_win, sht_win->bxsize, "task_a", 0);
                        make_wtitle8(buf_cons, sht_cons->bxsize, "console", 1);
                        cursor_c = -1;  // 不显示光标（改成负值）
                        boxfill8(sht_win->buf, sht_win->bxsize, COL8_FFFFFF, cursor_x, 28, cursor_x + 7, 43);   // 把光标改成白色隐去光标
                        
                        fifo32_put(&task_cons->fifo, 2);    // 向console任务缓冲区中存入"2"，表示开始闪烁光标
                    } else {
                        key_to = 0;
                        make_wtitle8(buf_win, sht_win->bxsize, "task_a", 1);
                        make_wtitle8(buf_cons, sht_cons->bxsize, "console", 0);
                        cursor_c = COL8_000000; // 显示光标

                        fifo32_put(&task_cons->fifo, 3);    // 向console任务缓冲区中存入"3"，表示停止闪烁光标
                    }
                    sheet_refresh(sht_win, 0, 0, sht_win->bxsize, 21);  // 21是标题栏的高度
                    sheet_refresh(sht_cons, 0, 0, sht_cons->bxsize, 21);
                }

                if (i == 0x1c) {    // 按下回车键
                    if (key_to != 0) {  // 确定当前focus是否在命令行窗口
                        fifo32_put(&task_cons->fifo, 10 + 256); // "10"是ASCII码中的换行
                    }
                }

                if (i == 0x2a) key_shift |= 1;  // left shift on
                if (i == 0x36) key_shift |= 2;  // right shigt on
                if (i == 0xaa) key_shift &= ~1; // left shift off
                if (i == 0xb6) key_shift &= ~2; // right shift off

                // 各种锁定键
                if (i == 0x3a) {    // CapsLock
                    key_leds ^= 4;  // ^= 0b100
                    fifo32_put(&keycmd, KEYCMD_LED);
                    fifo32_put(&keycmd, key_leds);
                }
                if (i == 0x45) {    // NumLock
                    key_leds ^= 2;  // ^= 0b010
                    fifo32_put(&keycmd, KEYCMD_LED);
                    fifo32_put(&keycmd, key_leds);
                }
                if (i == 0x46) {    // ScrollLock
                    key_leds ^= 1;  // ^= 0b001
                    fifo32_put(&keycmd, KEYCMD_LED);
                    fifo32_put(&keycmd, key_leds);
                }
                
                if (i == 0xfa) {    // 键盘成功接收到数据
                    keycmd_wait = -1;
                }
                if (i == 0xfe) {    // 键盘没有成功接收到数据
                    wait_KBC_sendready();
                    io_out8(PORT_KEYDAT, keycmd_wait);
                }


                // 刷新光标
                if (cursor_c >= 0) {
                    boxfill8(sht_win->buf, sht_win->bxsize, cursor_c, cursor_x, 28, cursor_x + 7, 43);
                }
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
                    if (cursor_c >= 0) {
                        cursor_c = COL8_000000;
                    }
                } else {
                    // i == 0
                    timer_init(timer, &fifo, 1);
                    if (cursor_c >= 0) {
                        cursor_c = COL8_FFFFFF;
                    }
                }
                timer_settime(timer, 50);  // 重新给timer3开始计时
                if (cursor_c >= 0) {
                    boxfill8(sht_win->buf, sht_win->bxsize, cursor_c, cursor_x, 28, cursor_x + 7, 43);
                    sheet_refresh(sht_win, cursor_x, 28, cursor_x + 8, 44);
                }
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
    int i, fifobuf[128], 
        cursor_x = 24, 
        cursor_y = 28,  // 光标y坐标
        cursor_c = -1;  // 将cursor_c初始化成-1，由后面主循环来判断是否要开始光标闪烁
    char *s = NULL;

    fifo32_init(&task->fifo, 128, fifobuf, task); // 有数据写入时，唤醒task_cons
    timer = timer_alloc();
    timer_init(timer, &task->fifo, 1);
    timer_settime(timer, 50);

    // 显示提示符 $ 
    putstring8_ascii_sht(sheet, 8, cursor_y, COL8_FFFFFF, COL8_000000, "$ ");
    int multi_line = 0; // 记录当前一行是否为多行的flag

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
                    if (cursor_c >= 0) {
                        cursor_c = COL8_FFFFFF;
                    }
                } else {
                    timer_init(timer, &task->fifo, 1);
                    if (cursor_c >= 0) {
                        cursor_c = COL8_000000;
                    }
                }
                timer_settime(timer, 50);

            } else if (256 <= i && i <= 511) {
                // 证明是task_a传来的键盘数据
                i -= 256;

                if (i == 8) {           
                    // 退格键
                    if (multi_line == 0) {      
                        // 只有一行
                        if (cursor_x > 24) {    // 大于16是为了不要把命令行提示符也给删掉
                            putstring8_ascii_sht(sheet, cursor_x, cursor_y, COL8_FFFFFF, COL8_000000, " ");
                            cursor_x -= 8;
                        }
                    } else if (cursor_y <= 28) {
                        // cursor_y == 28 && multi_line != 0; 这种情况是输入多行内容后滚动导致的
                        if (cursor_x > 8) {     // 在当前行内退格
                            putstring8_ascii_sht(sheet, cursor_x, cursor_y, COL8_FFFFFF, COL8_000000, " ");
                            cursor_x -= 8;
                        } else {                // 退格至前一行
                            /*
                                姑且先只把multi_line置为0了
                                理论上应该从文字缓冲区中取出被滚动走的之前的属于这一行的内容，并且显示出来
                                但是还没想好该咋写 :(
                            */
                            multi_line = 0;
                        }
                    } else {                    
                        // 存在多行
                        if (cursor_x > 8) {     // 在当前行内退格
                            putstring8_ascii_sht(sheet, cursor_x, cursor_y, COL8_FFFFFF, COL8_000000, " ");
                            cursor_x -= 8;
                        } else {                // 退格至前一行
                            multi_line--;
                            // 删除当前位置的光标
                            putstring8_ascii_sht(sheet, cursor_x, cursor_y, COL8_FFFFFF, COL8_000000, " ");
                            cursor_x = 240;
                            cursor_y -= 16;
                        }
                    }
                } else if (i == 10) {   
                    // 回车键
                    multi_line = 0;
                    // 删除当前位置的光标
                    putstring8_ascii_sht(sheet, cursor_x, cursor_y, COL8_FFFFFF, COL8_000000, " ");

                    if (cursor_y < 28 + 112) {
                        // 将光标坐标切换到下一行
                        cursor_y += 16; 
                    } else {
                        scroll_down(sheet, 8, 28, 240, 128);
                    }
                    cursor_x = 24;
                    // 显示提示符
                    putstring8_ascii_sht(sheet, 8, cursor_y, COL8_FFFFFF, COL8_000000, "$ ");
                } else {                
                    // 一般字符
                    s[0] = i;
                    s[1] = 0;
                    if (cursor_x < 240) {
                        putstring8_ascii_sht(sheet, cursor_x, cursor_y, COL8_FFFFFF, COL8_000000, s);
                        cursor_x += 8;
                    } else {    // 需要切换到下一行去显示了
                        // 先把这个位置要显示的字符打出来
                        putstring8_ascii_sht(sheet, cursor_x, cursor_y, COL8_FFFFFF, COL8_000000, s);
                        multi_line++;
                        if (cursor_y < 28 + 112) {
                            cursor_y += 16;
                        } else {
                            scroll_down(sheet, 8, 28, 240, 128);
                        }
                        cursor_x = 8;
                    }
                }
            } else if (i == 2) {
                // 开始光标闪烁
                cursor_c = COL8_000000;
            } else if (i == 3) {
                // 停止光标闪烁
                cursor_c = -1;
                boxfill8(sheet->buf, sheet->bxsize, COL8_000000, cursor_x, cursor_y, cursor_x + 7, cursor_y + 15);   // 用黑色将光标隐去
            }
            // 重新显示光标
            if (cursor_c >= 0) {
                boxfill8(sheet->buf, sheet->bxsize, cursor_c, cursor_x, cursor_y, cursor_x + 7, cursor_y + 15);
            }
            sheet_refresh(sheet, cursor_x, cursor_y, cursor_x + 8, cursor_y + 16);
        }
    }
}

/*
    将文本框内容向下滚动一行
*/
void scroll_down(struct SHEET *sheet, int x_init, int y_init, int xsize, int ysize) {
    int x, y;
    // 需要向下滚动一行，具体做法：所有像素向上移动一行，再把最下面一行涂黑
    for (y = y_init; y < y_init + ysize; y++) {
        for (x = x_init; x < x_init + xsize; x++) {
            sheet->buf[x + y * sheet->bxsize] = 
                sheet->buf[x + (y + 16) * sheet->bxsize];
        }
    }
    for (y = y_init + ysize - 16; y < y_init + ysize; y++) {
        for (x = x_init; x < x_init + xsize; x++) {
            sheet->buf[x + y * sheet->bxsize] = COL8_000000;
        }
    }
    sheet_refresh(sheet, x_init, y_init, x_init + xsize, y_init + ysize); // 刷新整片文本框
}
