// bootpack的主文件
#include <stdio.h>
#include "bootpack.h"   // 引入同一个文件夹内的头文件

extern struct FIFO8 keyfifo;    // 键盘缓冲区，注明是外部（int.c中）声明的结构体
extern struct FIFO8 mousefifo;  // 鼠标数据缓冲区

unsigned int memtest(unsigned int start, unsigned int end);
unsigned int memtest_sub(unsigned int start, unsigned int end);

void HariMain(void) {
    init_gdtidt();  // 初始化GDT和IDT，一定要在PIC之前初始化
    init_pic();     // 初始化PIC
    io_sti();       // IDT和PIC的初始化已经完成了，此时可以解除CPU对于中断的禁止（STI指令）
    // 允许来自键盘和鼠标的中断
    io_out8(PIC0_IMR, 0xf9);    // 允许PIC1和键盘的中断（11111001）
    io_out8(PIC1_IMR, 0xef);    // 允许鼠标的中断（11101111）

    init_palette(); // 初始化调色板 
    struct BOOTINFO *binfo;
    binfo = (struct BOOTINFO *) 0x0ff0; // asmhead里第一条BOOT_INFO的内存地址
    init_screen8(binfo->vram, binfo->scrnx, binfo->scrny);

    char *s = NULL;         // 字符串用指针

    // 显示鼠标
    int mx, my;
    char mcursor[256];   // 如果不写成固定长度的数组，鼠标上半会花掉，原因不明
    mx = (binfo->scrnx - 16) / 2;       // 计算屏幕中间坐标
    my = (binfo->scrny - 28 - 16) / 2;
    init_mouse_cursor8(mcursor, COL8_008484);
    putblock8(binfo->vram, binfo->scrnx, 16, 16, mx, my, mcursor, 16);
    // 显示鼠标坐标（左上角）
    sprintf(s, "(%d, %d)", mx, my);
    putstring8_ascii(binfo->vram, binfo->scrnx, 0, 0, COL8_FFFFFF, s);

    int i;                  // 存放从缓冲区中取出数据的变量

    // 用于显示键盘信息
    char *keybuf = NULL;    // 在初始化缓冲区时真正赋予缓冲区内存空间
    fifo8_init(&keyfifo, 32, keybuf);   // "&"是取地址运算符，把结构体的内存地址作为参数传入

    // 用于显示鼠标信息
    char *mousebuf = NULL;
    fifo8_init(&mousefifo, 256, mousebuf);  // 好像缓冲区大一点就能避免鼠标瞬移问题
    struct MOUSE_DEC mdec;

    // 初始化键盘，激活鼠标
    init_keyboard();
    enable_mouse(&mdec);

    // 内存检查
    i = memtest(0x00400000, 0xbfffffff) / (1024 * 1024);
    sprintf(s, "memory %d MB", i);
    putstring8_ascii(binfo->vram, binfo->scrnx, 0, 32, COL8_FFFFFF, s);

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
                boxfill8(binfo->vram, binfo->scrnx, COL8_008484, 0, 16, 15, 31);
                putstring8_ascii(binfo->vram, binfo->scrnx, 0, 16, COL8_FFFFFF, s);
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

                    boxfill8(binfo->vram, binfo->scrnx, COL8_008484, 32, 16, 32 + 8*13 - 1, 31);
                    putstring8_ascii(binfo->vram, binfo->scrnx, 32, 16, COL8_FFFFFF, s);

                    // 鼠标的移动
                    boxfill8(binfo->vram, binfo->scrnx, COL8_008484, mx, my, mx + 15, my + 15); // 先隐藏鼠标
                    boxfill8(binfo->vram, binfo->scrnx, COL8_008484, 0, 0, 79, 15);             // 隐藏之前的鼠标坐标
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
                    putstring8_ascii(binfo->vram, binfo->scrnx, 0, 0, COL8_FFFFFF, s);          // 显示新的坐标
                    putblock8(binfo->vram, binfo->scrnx, 16, 16, mx, my, mcursor, 16);          // 显示新位置的鼠标
                }
            }
        }
    }

}


#define EFLAGS_AC_BIT           0x00040000  // eflags Allow Cache bit是eflags的第18位
#define CR0_CACHE_DISABLE       0x60000000

// 返回值为出错的内存单元地址
unsigned int memtest(unsigned int start, unsigned int end) {
    // 为了测试内存连接是否正常，采取向内存中随便写入一个值再马上读取
    // 然而由于缓存机制的存在，再次读回的信息是从缓存读取的，一定不会有错
    // 为此，需要把CPU的缓存功能暂时关闭（486及以上CPU有缓存功能）
    
    // 1. 禁止缓存
    char flg486 = 0;
    unsigned int eflg, cr0, i;

    // 确认CPU是386还是486及以上的
    eflg = io_load_eflags();            // 读取当前eflags信息
    eflg |= EFLAGS_AC_BIT;              // 与运算，将Allow Cache位（第18位）设为1
    io_store_eflags(eflg);              // 将设好AC位的eflags存回EFLAGS寄存器
    eflg = io_load_eflags();            // 再次读取eflags信息
    // 如果是386处理器，那么之前设置AC位为1的操作是无效的，再次读取的eflags信息中AC = 0
    if ((eflg & EFLAGS_AC_BIT) != 0) {  // 所以如果AC位是1，那么当前处理器位486
        flg486 = 1;
    }
    eflg &= ~EFLAGS_AC_BIT;             // 取反操作，将eflags的AC位设为0
    io_store_eflags(eflg);              // 存回EFLAGS寄存器

    if (flg486 != 0) {                  // 如果之前判断出是486处理器
        cr0 = load_cr0();
        cr0 |= CR0_CACHE_DISABLE;       // 将控制寄存器0 cr0中允许缓存功能关闭
        store_cr0(cr0);                 // 将设好的cr0信息存回cr0寄存器
    }
    
    // 2. 执行内存测试
    i = memtest_sub(start, end);        // 进行实际上的内存测试，返回值为出错的内存单元地址

    // 3. 允许缓存
    if (flg486 != 0) {                  // 再把允许缓存打开
        cr0 = load_cr0();
        cr0 &= ~CR0_CACHE_DISABLE;
        store_cr0(cr0);
    }

    return i;
}

// 实际进行内存读写的内存测试
unsigned int memtest_sub(unsigned int start, unsigned int end) {
    unsigned int i, *p, old, pat0 = 0xaa55aa55, pat1 = 0x55aa55aa;  // pat1取反等于pat0

    for (i = start; i <= end; i += 0x1000) {
        p = (unsigned int *) (i + 0x0ffc);     // 本轮循环要操作的内存地址，是4KB（0x1000）中的最后4位

        old = *p;                   // 记住修改前的值
        *p = pat0;                  // 试写
        *p ^= 0xffffffff;           // 异或运算，将*p地址的数据（现在等于pat0）反转（应当等于pat1）

        if (*p != pat1) {           // 如果反转后数据有误
not_memory: 
            *p = old;               // 先把*p地址修改前的值恢复
            break;                  // 跳出for循环
        }
        
        *p ^= 0xffffffff;           // 再次通过XOR运算反转
        if (*p != pat0) {
            goto not_memory;
        }
        
        // 两轮反转下来都没错
        *p = old;                   // 恢复为修改前的值
    }

    return i;
}
