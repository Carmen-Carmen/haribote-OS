// bootpack的主文件
#include <stdio.h>
#include "bootpack.h"   // 引入同一个文件夹内的头文件

extern struct KEYBUF keybuf;   // 键盘缓冲区，注明是外部（int.c中）声明过的结构体

void HariMain(void) {
    init_gdtidt();  // 初始化GDT和IDT，一定要在PIC之前初始化
    init_pic();     // 初始化PIC
    io_sti();       // IDT和PIC的初始化已经完成了，此时可以解除CPU对于中断的禁止（STI指令）
        
    init_palette();// 初始化调色板
    
    struct BOOTINFO *binfo; // 相当于new了一个BOOTINFO类型的变量，变量名是binfo，同时还是一个指针
    binfo = (struct BOOTINFO *) 0x0ff0; // asmhead里第一条BOOT_INFO的内存地址
    init_screen8(binfo->vram, binfo->scrnx, binfo->scrny);

    // 显示鼠标
    int mx, my;
    char *mcursor = NULL;   // 避免"使用了可能未初始化的本地指针变量"
    mx = (binfo->scrnx - 16) / 2;
    my = (binfo->scrny - 16) / 2;
    init_mouse_cursor8(mcursor, COL8_008484);
    putblock8(binfo->vram, binfo->scrnx, 16, 16, mx, my, mcursor, 16);

    // 显示键盘按键编码
    int i;                  // 存放按键编码的变量

    // 显示鼠标坐标
    char *s = NULL;         // 字符串用指针
    sprintf(s, "(%d, %d)", mx, my);
    putstring8_ascii(binfo->vram, binfo->scrnx, 0, 0, COL8_FFFFFF, s);

    // 允许来自键盘和鼠标的中断
    io_out8(PIC0_IMR, 0xf9);    // 允许PIC1和键盘的中断（11111001）
    io_out8(PIC1_IMR, 0xef);    // 允许鼠标的中断（11101111）

    // 无限循环cpu休眠
    while (1) {
        // 先关中断，再作处理，不然就乱了
        io_cli();
        
        if (keybuf.len == 0) {
        // 如果键盘缓冲区里没有数据，开中断并让cpu暂停工作
            io_stihlt();
        } else {
        // 如果键盘缓冲区里有数据，先把数据取出来然后开中断
            // 读出next_r索引处的数据
            i = keybuf.data[keybuf.next_r];
            // 将len减1，让后续数据可以存入
            keybuf.len--;
            // next_r指针加1，读下一个
            keybuf.next_r++;
            // 如果下一个数据的角标为32，则将next_r置为0
            if (keybuf.next_r == 32) {
                keybuf.next_r = 0;      // keybuf.len != 0确保了此时0角标处是有数据的
            }

            io_sti();
            sprintf(s, "%02x", i);
            boxfill8(binfo->vram, binfo->scrnx, COL8_008484, 0, 16, 15, 31);
            putstring8_ascii(binfo->vram, binfo->scrnx, 0, 16, COL8_FFFFFF, s);
        }
    }
}



