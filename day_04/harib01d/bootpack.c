// 还需要把naskfunc.asm中编写的函数显式的声明出来
void io_hlt(void);
void write_mem8(int addr, int data);


void HariMain(void) {
    int i;
    char *p = (char *) 0xa0000; // 给指针赋值

    // 向图像缓冲区VRAM中全部写入15（白色）
    // VRAM的开始地址是0x000a0000，图像分辨率为320*200 = 64000
    // 0xaffff - 0xa0000 = 65535
    for (i = 0; i <= 0xffff; i++) {
        *(p + i) = i & 0x0f;
    }
    

    // 无限循环cpu休眠
    while (1) {
        io_hlt(); /*执行naskfunc.asm里的_io_hlt*/
    }
 
}
