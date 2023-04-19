// bootpack的主文件
#include <stdio.h>
#include <string.h>
#include "bootpack.h"   // 引入同一个文件夹内的头文件

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

    int i = 0,                  // 存放从缓冲区中取出数据的变量
        j,                      // 用来在鼠标左键点击时遍历图层的索引
        x, y;                   // 鼠标左键点击时计算是否落在图层范围内
    struct SHEET *sht;          // 接受遍历得到的图层的指针
    char s[128];                // 字符串用指针

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
    *((int *) 0x0fe4) = (int) shtctl;

    // sht_back
    sht_back = sheet_alloc(shtctl);
    buf_back = (unsigned char *) memman_alloc_4k(memman, binfo->scrnx * binfo->scrny);  // 给背景图层分配x * y字节大小的内存空间
    sheet_setbuf(sht_back, buf_back, binfo->scrnx, binfo->scrny, -1);   // 背景图层没有透明色
    init_screen8(buf_back, binfo->scrnx, binfo->scrny);

    // sht_cons
    struct SHEET *sht_cons;
    unsigned char *buf_cons;
    struct CONSOLE *cons;
    int cons_size_x = 80 * 8;   // 一行显示字符数 * 8
    int cons_size_y = 40 * 16;  // 行数 * 16
    sht_cons = sheet_alloc(shtctl);
    buf_cons = (unsigned char *) memman_alloc_4k(memman, (8 + cons_size_x + 8) * (28 + cons_size_y + 9));
    sheet_setbuf(sht_cons, buf_cons, 8 + cons_size_x + 8, 28 + cons_size_y + 9, -1);
    sprintf(s, "task_b%d", i);
    make_window8(buf_cons, 8 + cons_size_x + 8, 28 + cons_size_y + 9, "console", 0);
    make_textbox8(sht_cons, 8, 28, cons_size_x, cons_size_y, COL8_000000);
    task_cons = task_alloc();
    task_cons->tss.esp = memman_alloc_4k(memman, 64 * 1024) + 64 * 1024 - 20;   // 分配栈内存，压了新的参数进去记得要改栈顶啊阿啊！
    task_cons->tss.eip = (int) &console_task;   // 指令指针指向console_task
    task_cons->tss.es = 1 * 8;
    task_cons->tss.cs = 2 * 8;
    task_cons->tss.ss = 1 * 8;
    task_cons->tss.ds = 1 * 8;
    task_cons->tss.fs = 1 * 8;
    task_cons->tss.gs = 1 * 8;
    *((int *) (task_cons->tss.esp + 4)) = (int) sht_cons;  
    *((int *) (task_cons->tss.esp + 8)) = memtotal;         // 将memtotal作为第二个参数传入cons任务的入口函数
    *((int *) (task_cons->tss.esp + 12)) = cons_size_x;
    *((int *) (task_cons->tss.esp + 16)) = cons_size_y;
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
    // 右下角显示这个image来自哪个项目
    sprintf(s, "harib21a");
    putstring8_ascii_sht(   sht_back, 
                            sht_back->bxsize - 8 * 8, 
                            sht_back->bysize - 28 - 16, 
                            COL8_C6C6C6, COL8_008484, s);


    // 初始化键盘，激活鼠标
    init_keyboard(&fifo, 256);          // keyboard: 256 ~ 511
    enable_mouse(&fifo, 512, &mdec);    // mouse: 512 ~ 767
    
    io_out8(PIC0_IMR, 0xf8);            // 允许PIC0、PIC1和键盘的中断（11111000）
    io_out8(PIC1_IMR, 0xef);            // 允许鼠标的中断（11101111）

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
        key_ctrl = 0,   // 是否按下ctrl: left ctrl (1)
        key_leds = (binfo->leds >> 4) & 7,  // 第4位 ScrollLock，第5位 NumLock，第6位 CapsLock；右移4位后与0b111进行与运算，即只保留3位的这三个锁定键信息
        keycmd_wait = -1;
    fifo32_put(&keycmd, KEYCMD_LED);
    fifo32_put(&keycmd, key_leds);

    char * cur_time;
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
                    // 对方向键单独处理，发送给fifo写在后面了
                    if (i == 0x48 || i == 0x50 || i == 0x4b || i == 0x4d) {
                        s[0] = 0;
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

                if (s[0] != 0 && key_ctrl == 0) {    // 是一般字符
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
                // 对方向键单独处理
                if (i == 0x48) {
                    // up
                    if (key_to != 0) {
                        fifo32_put(&task_cons->fifo, 256 + KBD_UP);
                    }
                }
                if (i == 0x50) {
                    // down
                    if (key_to != 0) {
                        fifo32_put(&task_cons->fifo, 256 + KBD_DOWN);
                    }
                }
                if (i == 0x4b) {
                    // left
                    if (key_to != 0) {
                        fifo32_put(&task_cons->fifo, 256 + KBD_LEFT);
                    }
                }
                if (i == 0x4d) {
                    // right
                    if (key_to != 0) {
                        fifo32_put(&task_cons->fifo, 256 + KBD_RIGHT);
                    }
                }

                // 按下F10后，将最下方的图层sheets[1]（因为sheets[0]是背景图层）移动到top - 1，因为top应该是鼠标
                if (i == 0x44 && shtctl->top > 2) {
                    sheet_updown(shtctl->sheets[1], shtctl->top - 1);
                }

                if (i == 0x2a) key_shift |= 1;  // left shift on
                if (i == 0x36) key_shift |= 2;  // right shift on
                if (i == 0xaa) key_shift &= ~1; // left shift off
                if (i == 0xb6) key_shift &= ~2; // right shift off

                if (i == 0x1d) key_ctrl |= 1;   // left ctrl on
                if (i == 0x9d) key_ctrl &= ~1;  // left ctrl off

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

                // 强制停止应用程序的指令 ctrl+c
                if (i == 0x2e && key_ctrl != 0) {    
                    if (task_cons->tss.ss0 != 0) {
                    // tss.ss0 = 0时，没有应用程序在运行，此时强制结束就会出问题，需要做判断
                        cons = (struct CONSOLE *) *((int *) 0x0fec);
                        cons_putstring(cons, "\nBreak(key) :\n", COL8_FFFFFF, COL8_840000, -1);
                        io_cli();   // 要改变寄存器值了，此时不能切换到其他任务，先关中断
                        task_cons->tss.eax = (int) &(task_cons->tss.esp0);  // 设置eax为esp0的地址
                        task_cons->tss.eip = (int) asm_end_app;             // 这样asm_end_app中就能得到esp0的值
                        io_sti();
                    } else {
                    // 只是想清空命令行
                        for (i = 0; i < 256; i++) {
                            fifo32_put(&task_cons->fifo, 256 + 8);
                        }
                    }
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

                    // 按下鼠标左键
                    if ((mdec.btn & 0x01) != 0) {
                        for (j = shtctl->top - 1; j > 0; j--) {
                        // 从上往下遍历所有图层
                            sht = shtctl->sheets[j];
                            x = mx - sht->vx0;
                            y = my - sht->vy0;
                            if (0 <= x && x < sht->bxsize && 0 <= y && y < sht->bysize) {
                                if (sht->buf[y * sht->bxsize + x] != sht->col_inv) {
                                    // 如果落在某图层范围内，且该处不是图层的隐藏区域
                                    // 就把它提到最上层（鼠标下面一层）
                                    sheet_updown(sht, shtctl->top - 1);
                                    break;
                                }
                            }
                        }
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
                // 尝试做个显示时间的功能
                cur_time = get_ontime();
                putstring8_ascii_sht(sht_back, sht_back->bxsize - 4 - 8 * 8, sht_back->bysize - 20, COL8_000000, COL8_C6C6C6, cur_time);
            }
        }

        // 每隔一年(timerctl.count = 365 * 24 * 3600 * 100)，调整一次时间
        if (timerctl.count == 0xbbf81e00) {
            reset_timerctl_count();
        }
    }
}
