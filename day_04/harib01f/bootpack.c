// 还需要把naskfunc.asm中编写的函数显式的声明出来
void io_hlt(void);
void io_cli(void);  // 关中断 close interrupt / clear interrupt flag，总之就是把中断标志置为0
void io_out8(int port, int data); // 向指定io设备传输8位（1 Byte）数据的函数
int io_load_eflags(void);
int io_store_eflags(int eflags);

// 由于不是像Java一样的OOP语言，C语言是面向过程的语言，因此函数在使用前必须显式声明，即使写在了同一个文件里
void init_palette(void);
void set_palette(int start, int end, unsigned char *rgb);


void HariMain(void) {
    int i;
    char *p; // 给指针赋值
    
    init_palette();// 初始化调色板

    p = (char *) 0xa0000; // 指定地址

    for (i = 0; i <= 0xffff; i++) {
        p[i] = i & 0x0f;
    }
    

    // 无限循环cpu休眠
    while (1) {
        io_hlt(); /*执行naskfunc.asm里的_io_hlt*/
    }
}

void init_palette(void) {
    // static的char型数组，从索引0开始每3个元素代表一种颜色
    static unsigned char table_rgb[16 * 3] = {
        0x00, 0x00, 0x00, // black
        0xff, 0x00, 0x00, // bright red
        0x00, 0xff, 0x00, // bright yellow
        0xff, 0xff, 0x00, // bright green
        0x00, 0x00, 0xff, // bright blue
        0xff, 0x00, 0xff, // bright purple
        0x00, 0xff, 0xff, // 浅亮蓝
        0xff, 0xff, 0xff, // white
        0xc6, 0xc6, 0xc6, // bright gray
        0x84, 0x00, 0x00, // dark red
        0x00, 0x84, 0x00, // dark green
        0x84, 0x84, 0x00, // dark yellow
        0x00, 0x00, 0x84, // dark blue
        0x84, 0x00, 0x84, // dark purple
        0x00, 0x84, 0x84, // 浅暗蓝
        0x84, 0x84, 0x84 // dark gray
    };

    set_palette(0, 15, table_rgb);
    return;
}

void set_palette(int start, int end, unsigned char *rgb) {
    int i, eflags;
    eflags = io_load_eflags();  // 记录中断许可标志的值
    io_cli();                   // 将中断许可标志置0，禁止其他中断请求
    
    // 向io设备写数据
    io_out8(0x03c8, start);
    for (i = start; i < end; i++) {
        io_out8(0x03c9, rgb[0] / 4);
        io_out8(0x03c9, rgb[1] / 4);
        io_out8(0x03c9, rgb[2] / 4);
        rgb += 3;
    }

    io_store_eflags(eflags);    // 复原之前记录的中断许可标志
    return;
}
