// bootpack的主文件
#include <stdio.h>
#include "bootpack.h"   // 引入同一个文件夹内的头文件

unsigned int memtest(unsigned int start, unsigned int end);

#define MEMMAN_FREES        4090    // 大约是32KB
#define MEMMAN_ADDR         0x003c0000

struct FREEINFO {   // 描述可用内存区域信息的结构体
    unsigned int addr, size;    // 包括起始地址和大小
};

struct MEMMAN {     // MEMory MANager内存管理的结构体
    int frees, maxfrees, lostsize, losts;
    struct FREEINFO free[MEMMAN_FREES];
};

void memman_init(struct MEMMAN *man) {
    man->frees = 0;     // 可用信息数目
    man->maxfrees = 0;  // 用于观察可用情况：frees的最大值
    man->lostsize = 0;  // 释放失败的内存的大小总和
    man->losts = 0;     // 释放失败次数
    return;
}

// 报告剩余内存大小的合计
unsigned int memman_total(struct MEMMAN *man) {
    unsigned int i, t = 0;
    for (i = 0; i < man->frees; i++) {
        t += man->free[i].size;
    }

    return t;
}

// 分配内存
unsigned int memman_alloc(struct MEMMAN *man, unsigned int size) {
    unsigned int i;
    unsigned int a = 0;  // a: address
    for (i = 0; i < man->frees; i++) {
        if (man->free[i].size >= size) {    // 如果找到了足够大的内存
            a = man->free[i].addr;
            man->free[i].addr += size;
            man->free[i].size -= size;
            if (man->free[i].size == 0) {   // 如果free[i]的大小变成0，就去掉这条信息
                man->frees--;
                // 把free数组中free[i]后所有元素向前移动1位
                for (; i < man->frees; i++) {
                    // 结构体数组也可以用指针直接
                    man->free[i] = man->free[i + 1];
                }
            }
            break;
        }
    }
    return a;   // 0
}

// 释放内存
int memman_free(struct MEMMAN *man, unsigned int addr, unsigned int size) {
    int i, j;
    // 为了便于归纳内存，将定位到free[]数组中，传入addr参数的前一个地址
    for (i = 0; i < man->frees; i++) {
        if (man->free[i].addr > addr) {
            break;
        }
    }
    // 此时 free[i - 1].addr < addr < free[i].addr
    
    // 1. 既可以与前一块内存合并，也可以与后一块内存合并
    if (i > 0) {
        // 前面有可用内存
        if (man->free[i - 1].addr + man->free[i - 1].size == addr) {
            // 前一个可用内存的地址 + 其大小 == 归还内存的起始地址
            // 可以与前面的可用内存归纳到一起
            man->free[i - 1].size += size;
            if (i < man->frees) {
                // 后面还有可用内存
                if (addr + size == man->free[i].addr) {
                    // 可以与后一个可用内存合并
                    man->free[i - 1].size += man->free[i].size;
                    // 删除man->free[i]
                    man->frees--;
                    for (; i < man->frees; i++) {
                        man->free[i] = man->free[i + 1];
                    }
                }
            }
            return 0;   // 完成
        }
    }
    
    // 2. 只能与后一块内存合并
    if (i < man->frees) {
        if (addr + size == man->free[i].addr) {
            man->free[i].addr = addr;
            man->free[i].size += size;
            return 0;
        }
    }

    // 3. 既不能与前一块合并，也不能与后一块合并
    if (man->frees < MEMMAN_FREES) {
        // 就要把free[i]及其之后的都向后移1位
        for (j = man->frees; j > i; j--) {
            man->free[j] = man->free[j - 1];
        }
        man->frees++;
        if (man->maxfrees < man->frees) {
            man->maxfrees = man->frees; // 更新maxfrees的值
        }
        // 空出来的free[i]放入当前归还内存的信息
        man->free[i].addr = addr;
        man->free[i].size = size;
        return 0;
    }

    // 4. 如果当前man->frees的值比定义的MEMMAN_FREES还大
    // 即已经生成太多内存片段，无法进一步分割了
    // 虽然内存已经释放，但是由于无法被MEMMAN管理到，因此视作损失的内存
    man->losts++;
    man->lostsize += size;
    return -1;  // 失败
}

void HariMain(void) {
    init_gdtidt();  // 初始化GDT和IDT，一定要在PIC之前初始化
    init_pic();     // 初始化PIC
    io_sti();       // IDT和PIC的初始化已经完成了，此时可以解除CPU对于中断的禁止（STI指令）
    // 允许来自键盘和鼠标的中断
    io_out8(PIC0_IMR, 0xf9);    // 允许PIC1和键盘的中断（11111001）
    io_out8(PIC1_IMR, 0xef);    // 允许鼠标的中断（11101111）

    // 设置内存管理
    unsigned int memtotal;
    struct MEMMAN *memman = (struct MEMMAN *) MEMMAN_ADDR;

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
    char keybuf[32];        // 在初始化缓冲区时真正赋予缓冲区内存空间
    fifo8_init(&keyfifo, 32, keybuf);   // "&"是取地址运算符，把结构体的内存地址作为参数传入

    // 用于显示鼠标信息
    char mousebuf[256];     // 应当用定长空数组的形式初始化缓冲空间，否则加载很慢
    fifo8_init(&mousefifo, 256, mousebuf);
    struct MOUSE_DEC mdec;

    // 初始化键盘，激活鼠标
    init_keyboard();
    enable_mouse(&mdec);

    // 开始内存管理
    
    // 虽然本程序能识别到0xbfffffff（3GB）内存
    // 但根据模拟器的设定应该只有32MB，因此在超过32MB后memtest_sub()中检查就会出错，从而让i停在32MB，据此实现检测内存大小
    memtotal = memtest(0x00400000, 0xbfffffff);     
    memman_init(memman);
    memman_free(memman, 0x00001000, 0x0009e000);     // 释放0x00001000 ~ 0x0009efff的内存区域
    memman_free(memman, 0x00400000, memtotal - 0x00400000);     // 释放0x00400000开始直到memtotal的内存

    // 显示可用内存
    sprintf(s, "memory %d MB, free: %d KB", 
        memtotal / (1024 * 1024), memman_total(memman) / 1024);
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
    i = memtest_sub(start, end);        // 进行实际上的内存测试，返回值为出错的内存单元地址（汇编编写）

    // 3. 允许缓存
    if (flg486 != 0) {                  // 再把允许缓存打开
        cr0 = load_cr0();
        cr0 &= ~CR0_CACHE_DISABLE;
        store_cr0(cr0);
    }

    return i;
}

