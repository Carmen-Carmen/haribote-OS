// graphic处理相关
#include "bootpack.h"

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

// 绘制字符的函数: 从(x, y)开始绘制，颜色为c
void putfont8(char *vram, int xsize, int x, int y, char c, char *font) {
    int i;
    char *p;    //指针
    char d;     // data
    for (i = 0; i < 16; i++) {
        p = vram + (y + i) * xsize + x;
        d = font[i];
        
        // 相当于一次处理8个像素
        if ((d & 0x80) != 0) { p[0] = c; }  // 0x10000000，比较第一位
        if ((d & 0x40) != 0) { p[1] = c; }  // 0x01000000，比较第二位
        if ((d & 0x20) != 0) { p[2] = c; }  // ……
        if ((d & 0x10) != 0) { p[3] = c; }
        if ((d & 0x08) != 0) { p[4] = c; }
        if ((d & 0x04) != 0) { p[5] = c; }
        if ((d & 0x02) != 0) { p[6] = c; }
        if ((d & 0x01) != 0) { p[7] = c; }
    }

    return;
}

// 绘制字符串: 最后一个参数是指针，相当于是从字符串第一个字符开始向后遍历
void putstring8_ascii(char *vram, int xsize, int x, int y, char c, unsigned char *s) {
    // 告诉编译器hankaku数组是外部数据
    extern char hankaku[4096];
    
    // C语言中字符串都是以0x00结尾的
    for (; *s != 0x00; s++) {
        putfont8(vram, xsize, x, y, c, hankaku + *s * 16);
        x += 8;
    }

    return;
}

void init_screen(char *vram, int x, int y) {
    boxfill8(vram, x, COL8_008484,      0,      0, x -  1, y - 29); // 蓝色背景
    boxfill8(vram, x, COL8_C6C6C6,      0, y - 28, x -  1, y - 28); // 浅灰色一条
    boxfill8(vram, x, COL8_FFFFFF,      0, y - 27, x -  1, y - 27); // 白色一条
    boxfill8(vram, x, COL8_C6C6C6,      0, y - 26, x -  1, y -  1); // 浅灰色的task bar

    // windows按钮？
    boxfill8(vram, x, COL8_FFFFFF,      3, y - 24,     59, y - 24); // 白色上边框
    boxfill8(vram, x, COL8_FFFFFF,      2, y - 24,      2, y -  4); // 白色左边框
    boxfill8(vram, x, COL8_848484,      3, y -  4,     59, y -  4); // 深灰色下边框
    boxfill8(vram, x, COL8_848484,     59, y - 23,     59, y -  5); // 深灰色右边框
    boxfill8(vram, x, COL8_000000,      2, y -  3,     59, y -  3); // 黑色下边框
    boxfill8(vram, x, COL8_000000,     60, y - 24,     60, y -  3); // 黑色右边框

    // 时间按钮
    boxfill8(vram, x, COL8_848484, x - 47, y - 24, x -  4, y - 24); // 深灰色上边框
    boxfill8(vram, x, COL8_848484, x - 47, y - 23, x - 47, y -  4); // 深灰色左边框
    boxfill8(vram, x, COL8_FFFFFF, x - 47, y -  3, x -  4, y -  3); // 白色下边框
    boxfill8(vram, x, COL8_FFFFFF, x -  3, y - 24, x -  3, y -  3); // 白色右边框

}

// 初始化16 * 16大小鼠标指针
// 只是把要显示鼠标的信息写入了mouse指针，还没有完成显示的工作
void init_mouse_cursor8(char *mouse, char bc) {
    // char *mouse: 绘制鼠标的vram地址
    // char bc: 鼠标所处位置的背景颜色

    // 用一个二维数组来存放鼠标指针图案；字符串就是char数组
    static char cursor[16][16] = {
        "**************..",
	"*OOOOOOOOOOO*...",
    	"*OOOOOOOOOO*....",
    	"*OOOOOOOOO*.....",
    	"*OOOOOOOO*......",
    	"*OOOOOOO*.......",
    	"*OOOOOOO*.......",
    	"*OOOOOOOO*......",
    	"*OOOO**OOO*.....",
    	"*OOO*..*OOO*....",
    	"*OO*....*OOO*...",
    	"*O*......*OOO*..",
    	"**........*OOO*.",
    	"*..........*OOO*",
    	"............*OO*",
    	".............***"
    };
    
    int x, y;
    // 逐行渲染鼠标
    for (y = 0; y < 16; y++) {
        for (x = 0; x < 16; x++) {
            if (cursor[x][y] == '*') { mouse[y * 16 + x] = COL8_000000; }
            if (cursor[x][y] == 'O') { mouse[y * 16 + x] = COL8_FFFFFF; }
            if (cursor[x][y] == '.') { mouse[y * 16 + x] = bc; }
        }
    }

    return;
}

// 显示一块儿区域的方法
void putblock8(
    char *vram, int vxsize, 
    int pxsize, int pysize, // pxsize. pysize: picture x size, picture y size图片的长宽
    int px0, int py0,       // px0, py0: 要显示图片的坐标，即图片左上角的坐标
    char *p_addr, int bxsize
    // *p_addr: picture address图片的内存地址
    // bxsize: block x size，一般情况下和pxsize相同，若要缩放显示就会不同，因此作为一个参数传入
) {
    int x, y;
    for (y = 0; y < pysize; y++) {
        for (x  = 0; x < pxsize; x++) {
            vram[(py0 + y) * vxsize + (px0 + x)] = p_addr[y * bxsize + x];
        }
    }
    return;
}

