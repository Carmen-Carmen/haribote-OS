// bootpack的主文件
#include <stdio.h>
#include <string.h>
#include "bootpack.h"   // 引入同一个文件夹内的头文件

#define KEYCMD_LED      0xed    // 0b11101101，控制键盘LED灯的指令码？

void make_window8(unsigned char *buf, int xsize, int ysize, char *title, char act);
void make_textbox8(struct SHEET *sht, int x0, int y0, int sx, int sy, int c);
void make_wtitle8(unsigned char *buf, int xsize, char *title, char act);

void console_task(struct SHEET *sheet, unsigned int memtotal);
void scroll_down(struct SHEET *sheet, int x_init, int y_init, int xsize, int ysize);
int cons_newline(int cursor_y, struct SHEET *sheet);
void cons_clear(struct SHEET *sheet, int x_init, int y_init, int xsize, int ysize);
int cons_putstring(struct SHEET *sheet, int cursor_x, int cursor_y, int cc, int bc, char *string, unsigned int start, unsigned int end);

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

    int i = 0;                  // 存放从缓冲区中取出数据的变量
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
    buf_cons = (unsigned char *) memman_alloc_4k(memman, 416 * 357);
    sheet_setbuf(sht_cons, buf_cons, 416, 357, -1);
    sprintf(s, "task_b%d", i);
    make_window8(buf_cons, 416, 357, "console", 0);
    make_textbox8(sht_cons, 8, 28, 400, 320, COL8_000000);
    task_cons = task_alloc();
    task_cons->tss.esp = memman_alloc_4k(memman, 64 * 1024) + 64 * 1024 - 12;   // 分配栈内存
    task_cons->tss.eip = (int) &console_task;   // 指令指针指向console_task
    task_cons->tss.es = 1 * 8;
    task_cons->tss.cs = 2 * 8;
    task_cons->tss.ss = 1 * 8;
    task_cons->tss.ds = 1 * 8;
    task_cons->tss.fs = 1 * 8;
    task_cons->tss.gs = 1 * 8;
    *((int *) (task_cons->tss.esp + 4)) = (int) sht_cons;  
    *((int *) (task_cons->tss.esp + 8)) = memtotal; // 将memtotal作为第二个参数传入cons任务的入口函数
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
    // // 显示可用内存
    // sprintf(s, "memory %d MB, free: %d KB", 
    //     memtotal / (1024 * 1024), memman_total(memman) / 1024);
    // putstring8_ascii_sht(sht_back, 0, 32, COL8_FFFFFF, COL8_008484, s);
    // 显示这个image来自哪个项目
    sprintf(s, "harib16b");
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
void console_task(struct SHEET *sheet, unsigned int memtotal) {
    struct TIMER *timer;
    struct TASK *task = task_now();
    int i, x, y, fifobuf[128], 
        cmdindex = 0,   // 命令字符串的索引
        cursor_x = 24, 
        cursor_y = 28,  // 光标y坐标
        cursor_c = -1;  // 将cursor_c初始化成-1，由后面主循环来判断是否要开始光标闪烁
    char s[30];
    char cmdline[2048];
    struct MEMMAN *memman = (struct MEMMAN *) MEMMAN_ADDR;
    struct FILEINFO *finfo = (struct FILEINFO *) (ADR_DISKIMG + 0x002600);
    char * p_disk;
    int *fat = (int *) memman_alloc_4k(memman, 4 * 2880);

    fifo32_init(&task->fifo, 128, fifobuf, task); // 有数据写入时，唤醒task_cons
    timer = timer_alloc();
    timer_init(timer, &task->fifo, 1);
    timer_settime(timer, 50);

    file_decodefat(fat, (unsigned char *) (ADR_DISKIMG + 0x000200));  // 将磁盘映像中的fat内容解压，并载入fat指针

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
                    cursor_c = COL8_FFFFFF;
                    if (cmdindex > 0) {
                        cmdline[cmdindex--] = 0;
                    }

                    if (multi_line == 0) {      
                        // 只有一行
                        if (cursor_x > 24) {    // 大于24是为了不要把命令行提示符也给删掉
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
                                从文字缓冲区中取出被滚动走的之前的属于这一行的内容，并且显示出来
                                通过multi_line的行数来计算出应该显示哪些，并显示在第一行
                            */
                            multi_line--;
                            int s_index;
                            if (multi_line != 0) {
                                for (s_index = (400 - 16) / 8 + (multi_line - 1) * 400 / 8; s_index <= cmdindex; s_index++) {
                                    s[s_index - ((400 - 16) / 8 + (multi_line - 1) * 400 / 8)] = cmdline[s_index];
                                }
                                s[400 / 8] = 0;
                                putstring8_ascii_sht(sheet, 8, 28, COL8_FFFFFF, COL8_000000, s);
                            } else {
                                for (s_index = 0; s_index <= cmdindex; s_index++) {
                                    s[s_index] = cmdline[s_index];
                                }
                                s[(400 - 16) / 8] = 0;
                                putstring8_ascii_sht(sheet, 8, 28, COL8_FFFFFF, COL8_000000, "$ ");
                                putstring8_ascii_sht(sheet, 24, 28, COL8_FFFFFF, COL8_000000, s);
                            }
                            cursor_x = 400;
                            cursor_c = -1;
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
                            cursor_x = 400;
                            cursor_y -= 16;
                        }
                    }
                } else if (i == 10) {   
                // 回车键
                    multi_line = 0;
                    // 删除当前位置的光标
                    putstring8_ascii_sht(sheet, cursor_x, cursor_y, COL8_FFFFFF, COL8_000000, " ");
                    
                    cmdline[cmdindex] = 0;
                    cursor_y = cons_newline(cursor_y, sheet);

                    // 执行命令
                    if (strcmp(cmdline, "mem") == 0) {
                        // "mem" command
                        sprintf(s, "total   %dMB", memtotal / (1024 * 1024));
                        putstring8_ascii_sht(sheet, 8, cursor_y, COL8_FFFFFF, COL8_000000, s);
                        cursor_y = cons_newline(cursor_y, sheet);
                        sprintf(s, "free %dKB", memman_total(memman) / 1024);
                        putstring8_ascii_sht(sheet, 8, cursor_y, COL8_FFFFFF, COL8_000000, s);
                        cursor_y = cons_newline(cursor_y, sheet);
                        cursor_y = cons_newline(cursor_y, sheet);
                    } else if (strcmp(cmdline, "clear") == 0) {
                        // "clear" command
                        cons_clear(sheet, 8, 28, 400, 320);
                        cursor_y = 28; 
                    } else if (strcmp(cmdline, "ls") == 0) {
                        // "ls" command
                        for (x = 0; x < 224; x++) { // 最多有224个文件
                            // 利用FINFO类型指针，从内存0x00102600开始按32个Byte读取
                            if (finfo[x].name[0] == 0x00) {
                                // 直到，读到空文件，退出循环
                                break;
                            }
                            if (finfo[x].name[0] != 0xe5) {
                                // 文件名开头不是0xe5，是正常的文件
                                if ((finfo[x].type & 0x18) == 0) {
                                    // 既不是非文件（0x08）也不是目录（0x10），就要读出来
                                    // 暂时只显示文件名为8个字符以内的
                                    sprintf(s, "filename.ext    %7d", finfo[x].size);
                                    for (y = 0; y < 8; y++) {
                                        s[y] = finfo[x].name[y];
                                    }
                                    // 拼接扩展名
                                    s[ 9] = finfo[x].ext[0];
                                    s[10] = finfo[x].ext[1];
                                    s[11] = finfo[x].ext[2];
                                    putstring8_ascii_sht(sheet, 8, cursor_y, COL8_FFFFFF, COL8_000000, s);
                                    cursor_y = cons_newline(cursor_y, sheet);
                                }
                            }
                        }
                        cursor_y = cons_newline(cursor_y, sheet);
                    } else if (strncmp(cmdline, "cat ", 4) == 0 || strcmp(cmdline, "cat") == 0) { 
                        // strncmp的第3个参数是从头开始比较的字符个数
                        // "cat" command

                        if (strcmp(cmdline, "cat") == 0) {
                            cursor_x = 8;
                            cursor_y = cons_putstring(sheet, cursor_x, cursor_y, COL8_FFFFFF, COL8_000000, "usage: cat filename.ext", 0, strlen("usage: cat filename.ext"));
                            cursor_y = cons_newline(cursor_y, sheet);
                            cmdindex = 0;
                            // 显示命令行提示符，并将cursor的x坐标移到最左边
                            putstring8_ascii_sht(sheet, 8, cursor_y, COL8_FFFFFF, COL8_000000, "$ ");
                            cursor_x = 24;
                            continue;
                        }

                        // 将文件名读入s指针，少于8个字符的文件名用空格填充
                        for (y = 0; y < 11; y++) {
                            s[y] = ' ';
                        }
                        y = 0;
                        for (x = 4; y < 11 && cmdline[x] != 0; x++) {    // 从 "cat "开始
                            if (cmdline[x] == '.' && y <= 8) {
                                // 读到扩展名前了
                                y = 8;
                            } else {
                                s[y] = cmdline[x];
                                if ('a' <= s[y] && s[y] <= 'z') {
                                    // 将命令输入的小写字母转换成大写
                                    s[y] -= 0x20;
                                }
                                y++;
                            }
                        }
                        // 走到这里s指针指向的是 "xXX     ext" 格式的字符串

                        // 寻找文件
                        for (x = 0; x < 224; ) {
                            if (finfo[x].name[0] == 0x00) {
                                break;
                            }
                            if ((finfo[x].type & 0x18) == 0) {
                                for (y = 0; y < 8; y++) {
                                    if (finfo[x].name[y] != s[y]) {
                                        // 逐个字符比较 文件名
                                        goto next_file;
                                    }
                                }
                                for (y = 8; y < 11; y++) {
                                    if (finfo[x].ext[y - 8] != s[y]) {
                                        // 逐个字符比较 扩展名
                                        goto next_file;
                                    }
                                }
                                break;  // 走完了上面那块for循环，说明文件名内
                            }
next_file: 
                            x++;
                        }

                        if (x < 244 && finfo[x].name[0] != 0x00) {
                            // // 找到这个文件了
                            cursor_x = 8;
                            p_disk = (char *) memman_alloc_4k(memman, finfo[x].size);
                            file_loadfile(finfo[x].clustno, finfo[x].size, p_disk, fat, (char *) (ADR_DISKIMG) + 0x003e00);
                            cursor_y = cons_putstring(sheet, cursor_x, cursor_y, COL8_FFFFFF, COL8_000000, p_disk, 0, finfo[x].size);   // 将文件内容按照fat，load进定长度的p_disk指针中
                            memman_free_4k(memman, (int) p_disk, finfo[x].size);    // 相当于File.close()
                        } else {
                            // 没有找到文件
                            putstring8_ascii_sht(sheet, 8, cursor_y, COL8_FFFFFF, COL8_840000, "[file not found]: ");
                            cursor_x = 8 + strlen("[file not found]: ") * 8;
                            // 拼接[file not found]: 之后的内容
                            cursor_y = cons_putstring(sheet, cursor_x, cursor_y, COL8_FFFFFF, COL8_840000, cmdline, 4, cmdindex);
                        }

                    } else if (cmdline[0] != 0) {
                        // 既不是命令，也不是空行
                        putstring8_ascii_sht(sheet, 8, cursor_y, COL8_FFFFFF, COL8_840000, "[command not found]: ");
                        cursor_x = 8 + strlen("[command not found]: ") * 8;
                        // 拼接[command not found]: 后面的内容
                        cursor_y = cons_putstring(sheet, cursor_x, cursor_y, COL8_FFFFFF, COL8_840000, cmdline, 0, cmdindex);
                    }
                    
                    cmdindex = 0;
                    // 显示命令行提示符，并将cursor的x坐标移到最左边
                    putstring8_ascii_sht(sheet, 8, cursor_y, COL8_FFFFFF, COL8_000000, "$ ");
                    cursor_x = 24;
                } else {                
                // 一般字符
                    s[0] = i;
                    s[1] = 0;

                    if (cmdindex > 2048) continue;
                    cmdline[cmdindex++] = i;  
                                        
                    if (cursor_x < 400) {
                        putstring8_ascii_sht(sheet, cursor_x, cursor_y, COL8_FFFFFF, COL8_000000, s);
                        cursor_x += 8;
                    } else {    // 需要切换到下一行去显示了
                        // 先把这个位置要显示的字符打出来
                        putstring8_ascii_sht(sheet, cursor_x, cursor_y, COL8_FFFFFF, COL8_000000, s);
                        multi_line++;
                        if (cursor_y < 28 + 304) {
                            cursor_y += 16;
                        } else {
                            scroll_down(sheet, 8, 28, 400, 320);
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

/*
    命令行换行，并返回换行后的cursor_y
*/
int cons_newline(int cursor_y, struct SHEET *sheet) {
    if (cursor_y < 28 + 304) {
           // 将光标坐标切换到下一行
           cursor_y += 16; 
    } else {
        scroll_down(sheet, 8, 28, 400, 320);
    }
    
    return cursor_y;
}

void cons_clear(struct SHEET *sheet, int x_init, int y_init, int xsize, int ysize) {
    int x, y;
    for (y = y_init; y < y_init + ysize; y++) {
        for (x = x_init; x < x_init + xsize; x++) {
            sheet->buf[x + y * sheet->bxsize] = COL8_000000;
        }
    }
    sheet_refresh(sheet, x_init, y_init, x_init + xsize, y_init + ysize);
    return;
}

/*
    在console中打印字符串
    cc: character color
    bc: background color
*/
int cons_putstring(struct SHEET *sheet, int cursor_x, int cursor_y, int cc, int bc, char *string, unsigned int start, unsigned int end) {
    unsigned int i;
    char *s = NULL;
    for (i = start; i < end; i++) {
        s[0] = string[i];
        s[1] = 0;
        // 处理制表符、换行回车等
        if (s[0] == 0x09) {
            // 制表符
            for (;;) {
                putstring8_ascii_sht(sheet, cursor_x, cursor_y, cc, bc, " ");
                cursor_x += 8;
                if (cursor_x == 8 + 400) {
                    cursor_x = 8;
                    cursor_y = cons_newline(cursor_y, sheet);
                }
                if (((cursor_x - 8) & 0x1f) == 0) {
                    // 被32整除，即达到4个字符
                    break;
                }
            }
        } else if (s[0] == 0x0a) {
            // 换行符
            cursor_x = 8;
            cursor_y = cons_newline(cursor_y, sheet);
        } else if (s[0] == 0x0d) {
            // 暂时不做操作
        } else {
            // 一般字符
            // 逐个打印字符并换行
            putstring8_ascii_sht(sheet, cursor_x, cursor_y, cc, bc, s);
            cursor_x += 8;
            if (cursor_x == 8 + 400) {
                cursor_y = cons_newline(cursor_y, sheet);
                cursor_x = 8;
            }
        }

    }
    cursor_y = cons_newline(cursor_y, sheet);
    cursor_y = cons_newline(cursor_y, sheet);

    return cursor_y;
}

/*
    解压缩磁盘映像中FAT
    解压缩算法：
        img指向磁盘映像缓冲中的压缩过的fat
        压缩数据每3个为一组，abc def，换位成dab efc
*/
void file_decodefat(int *fat, unsigned char *img) {
    int i, j = 0;
    for (i = 0; i < 2880; i += 2) {
        fat[i + 0] = (img[j + 0] | img[j + 1] << 8) & 0xfff;
        fat[i + 1] = (img[j + 1] >> 4 | img[j + 2] << 4) & 0xfff;
        j += 3;
    }
    return;
}

/*
    按照fat读取文件，读到buf指针中
    fat是经过解压后的fat
    img是指向 磁盘缓冲区 中内容的指针
*/
void file_loadfile(int clustno, int size, char *buf, int *fat, char *img) {
    int i;
    while (1) {
        if (size <= 512) {
            // 对于小于512 Byte的文件，或者读到大文件最后一个碎片，直接读入并跳出循环
            for (i = 0; i < size; i++) {
                buf[i] = img[clustno * 512 + i];
            }
            break;
        }

        // 否则，从不同簇号（扇区）中读入512 Byte
        for (i = 0; i < 512; i++) {
            buf[i] = img[clustno * 512 + i];
        }

        size -= 512;
        buf += 512;
        clustno = fat[clustno]; // 按照fat指示找到下一个簇号，接力往后读
    }
    return;
}
