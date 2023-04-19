// 还需要把naskfunc.asm中编写的函数显式的声明出来
void io_hlt(void);
void io_cli(void);  // 关中断 close interrupt / clear interrupt flag，总之就是把中断标志置为0
void io_out8(int port, int data); // 向指定io设备传输8位（1 Byte）数据的函数
int io_load_eflags(void);   // 返回当前EFLAGS寄存器中数据，包括中断标志
int io_store_eflags(int eflags);    // 把所有标志存回EFLAGS寄存器中

// 由于不是像Java一样的OOP语言，C语言是面向过程的语言，因此函数在使用前必须显式声明，即使写在了同一个文件里
void init_palette(void);
void set_palette(int start, int end, unsigned char *rgb);
void boxfill8(unsigned char *vram, int xsize, unsigned char c, int x0, int y0, int x1, int y1);

// #define用于常数的声明
#define COL8_000000         0
#define COL8_FF0000         1
#define COL8_00FF00         2
#define COL8_FFFF00         3
#define COL8_0000FF         4
#define COL8_FF00FF         5
#define COL8_00FFFF         6
#define COL8_FFFFFF         7
#define COL8_C6C6C6         8
#define COL8_840000         9
#define COL8_008400         10
#define COL8_848400         11
#define COL8_000084         12
#define COL8_840084         13
#define COL8_008484         14
#define COL8_848484         15


void HariMain(void) {
    char *vram = (char *) 0xa0000; // 初始化指针
    
    init_palette();// 初始化调色板

    int xsize = 320;
    int ysize = 200;
    // 绘制 vram    xsize   color        x0          y0             x1          y1
    boxfill8(vram, xsize, COL8_008484,          0,          0, xsize -  1, ysize - 29); // 蓝色背景
    boxfill8(vram, xsize, COL8_C6C6C6,          0, ysize - 28, xsize -  1, ysize - 28); // 浅灰色一条
    boxfill8(vram, xsize, COL8_FFFFFF,          0, ysize - 27, xsize -  1, ysize - 27); // 白色一条
    boxfill8(vram, xsize, COL8_C6C6C6,          0, ysize - 26, xsize -  1, ysize -  1); // 浅灰色的task bar

    // windows按钮？
    boxfill8(vram, xsize, COL8_FFFFFF,          3, ysize - 24,         59, ysize - 24); // 白色上边框
    boxfill8(vram, xsize, COL8_FFFFFF,          2, ysize - 24,          2, ysize -  4); // 白色左边框
    boxfill8(vram, xsize, COL8_848484,          3, ysize -  4,         59, ysize -  4); // 深灰色下边框
    boxfill8(vram, xsize, COL8_848484,         59, ysize - 23,         59, ysize -  5); // 深灰色右边框
    boxfill8(vram, xsize, COL8_000000,          2, ysize -  3,         59, ysize -  3); // 黑色下边框
    boxfill8(vram, xsize, COL8_000000,         60, ysize - 24,         60, ysize -  3); // 黑色右边框

    // 时间按钮
    boxfill8(vram, xsize, COL8_848484, xsize - 47, ysize - 24, xsize -  4, ysize - 24); // 深灰色上边框
    boxfill8(vram, xsize, COL8_848484, xsize - 47, ysize - 23, xsize - 47, ysize -  4); // 深灰色左边框
    boxfill8(vram, xsize, COL8_FFFFFF, xsize - 47, ysize -  3, xsize -  4, ysize -  3); // 白色下边框
    boxfill8(vram, xsize, COL8_FFFFFF, xsize -  3, ysize - 24, xsize -  3, ysize -  3); // 白色右边框


    // 无限循环cpu休眠
    while (1) {
        io_hlt(); /*执行naskfunc.asm里的_io_hlt*/
    }
}

void init_palette(void) {
    // static的char型数组，从索引0开始每3个元素代表一种颜色
    static unsigned char table_rgb[16 * 3] = {
        0x00, 0x00, 0x00,   // black
        0xff, 0x00, 0x00,   // bright red
        0x00, 0xff, 0x00,   // bright yellow
        0xff, 0xff, 0x00,   // bright green
        0x00, 0x00, 0xff,   // bright blue
        0xff, 0x00, 0xff,   // bright purple
        0x00, 0xff, 0xff,   // 浅亮蓝
        0xff, 0xff, 0xff,   // white
        0xc6, 0xc6, 0xc6,   // bright gray
        0x84, 0x00, 0x00,   // dark red
        0x00, 0x84, 0x00,   // dark green
        0x84, 0x84, 0x00,   // dark yellow
        0x00, 0x00, 0x84,   // dark blue
        0x84, 0x00, 0x84,   // dark purple
        0x00, 0x84, 0x84,   // 浅暗蓝
        0x84, 0x84, 0x84    // dark gray
    };

    set_palette(0, 15, table_rgb);
    return;
}

// 把所有颜色写入内存中
void set_palette(int start, int end, unsigned char *rgb) {
    int i, eflags;
    eflags = io_load_eflags();  // 记录中断许可标志的值
    io_cli();                   // 将中断许可标志置0，禁止其他中断请求
    
    // 向io设备写数据
    io_out8(0x03c8, start);
    for (i = start; i <= end; i++) { // 注意这个作者写循环喜欢走到end角标的！
        io_out8(0x03c9, rgb[0] / 4);
        io_out8(0x03c9, rgb[1] / 4);
        io_out8(0x03c9, rgb[2] / 4);
        rgb += 3;
    }

    io_store_eflags(eflags);    // 复原之前记录的中断许可标志
    return;
}

// 绘制(x0, y0)到(x1, y1)的矩形
void boxfill8(unsigned char *vram, int xsize, unsigned char c, int x0, int y0, int x1, int y1) {
    int x, y;
    // 先定位行（y），从y0行开始到y1行结束
    for (y = y0; y <= y1; y++) {
    // 再定位列（x），从x0列开始到x1列结束
        for (x = x0; x <= x1; x++) {
            // 向*vram指针指向的内存地址中写入c（color）的值
            // 单个像素(x, y)对应的vram地址就是0xa0000（起始地址）+ x + y * xsize
            vram[y * xsize + x] = c;
        }
    }

    return;
}
