// bootpack的主文件
#include <stdio.h>
#include "bootpack.h"   // 引入同一个文件夹内的头文件

void HariMain(void) {
    init_pic();// 初始化PIC
        
    init_palette();// 初始化调色板
    
    struct BOOTINFO *binfo; // 相当于new了一个BOOTINFO类型的变量，变量名是binfo，同时还是一个指针
    binfo = (struct BOOTINFO *) 0x0ff0; // asmhead里第一条BOOT_INFO的内存地址
    init_screen(binfo->vram, binfo->scrnx, binfo->scrny);

    // 显示鼠标
    int mx, my;
    char *mcursor = NULL;   // 避免"使用了可能未初始化的本地指针变量"
    mx = (binfo->scrnx - 16) / 2;
    my = (binfo->scrny - 16) / 2;
    init_mouse_cursor8(mcursor, COL8_008484);
    putblock8(binfo->vram, binfo->scrnx, 16, 16, mx, my, mcursor, 16);

    // 显示鼠标坐标
    char *s = NULL;
    sprintf(s, "(%d, %d)", mx, my);
    putstring8_ascii(binfo->vram, binfo->scrnx, 0, 0, COL8_FFFFFF, s);

    // 无限循环cpu休眠
    while (1) {
        io_hlt(); /*执行naskfunc.asm里的_io_hlt*/
    }
}



