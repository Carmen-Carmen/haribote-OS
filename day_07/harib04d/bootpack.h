// asmhead.asm
struct BOOTINFO {       // 0x0ff0~0x0fff
    char cyls;          // 启动区读软盘的结束位置
    char leds;          // 启动时键盘灯状态
    char vmode;         // 图像模式，如多少位色
    char reserve;           
    short scrnx, scrny; // 分辨率            
    char *vram;         // 图像缓冲区起始地址                         
};
#define ADR_BOOTINFO        0x00000ff0  // BOOT_INFO从0x0ff0开始

// naskfunc.asm
void io_hlt(void);
void io_cli(void);  // 关中断 close interrupt / clear interrupt flag，总之就是把中断标志置为0
void io_sti(void);  // 开中断 SeT Interrupt
void io_stihlt(void);   // 开中断后halt
void io_out8(int port, int data); // 向指定io设备传输8位（1 Byte）数据的函数
int io_in8(int port);
int io_load_eflags(void);   // 返回当前EFLAGS寄存器中数据，包括中断标志
void io_store_eflags(int eflags);    // 把所有标志存回EFLAGS寄存器中
void load_gdtr(int limit, int addr);
void load_idtr(int limit, int addr);
void asm_inthandler21(void);
void asm_inthandler27(void);
void asm_inthandler2c(void);

// graphic.c
void init_palette(void);
void set_palette(int start, int end, unsigned char *rgb);
void boxfill8(unsigned char *vram, int xsize, unsigned char c, int x0, int y0, int x1, int y1);
void putfont8(char *vram, int xsize, int x, int y, char c, char *font);
void putstring8_ascii(char *vram, int xsize, int x, int y, char c, unsigned char *s);
void init_screen8(char *vram, int x, int y);
void init_mouse_cursor8(char *mouse, char bc);
void putblock8(char *vram, int vxsize, int pxsize, int pysize, int px0, int py0, char *p_addr, int bxsize); 

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

// dsctbl.c
struct SEGMENT_DESCRIPTOR {
        /* 
        !!!!!!结构体中声明属性的顺序一定是固定的，因为计算机取数的时候也是固定顺序取的，乱了就错了
        段的基址
        short base_low;
        char base_mid;
        char base_high;
        
        段的上限
        short limit_low;
        char limit_high;
        
        段的管理属性
        char access_right;
        */
        short limit_low, base_low;
	char base_mid, access_right;
	char limit_high, base_high;
};

struct GATE_DESCRIPTOR {
	short offset_low, selector;
	char dw_count, access_right;
	short offset_high;
};

void init_gdtidt(void); 
void set_segmdesc(struct SEGMENT_DESCRIPTOR *sd, unsigned int limit, int base, int ar); 
void set_gatedesc(struct GATE_DESCRIPTOR *gd, int offset, int selector, int ar); 

#define ADR_IDT             0x0026f800
#define LIMIT_IDT           0x000007ff
#define ADR_GDT             0x00270000
#define LIMIT_GDT           0x0000ffff
#define ADR_BOTPAK          0x00280000
#define LIMIT_BOTPAK        0x0007ffff
#define AR_DATA32_RW        0x4092      // 可读可写
#define AR_CODE32_ER        0x409a      // 可执行 + 可读
#define AR_INTGATE32        0x008e

// int.c
void init_pic(void);
#define PIC0_ICW1	    0x0020
#define PIC0_OCW2   	    0x0020
#define PIC0_IMR            0x0021
#define PIC0_ICW2	    0x0021
#define PIC0_ICW3	    0x0021
#define PIC0_ICW4	    0x0021
#define PIC1_ICW1	    0x00a0
#define PIC1_OCW2	    0x00a0
#define PIC1_IMR	    0x00a1
#define PIC1_ICW2	    0x00a1
#define PIC1_ICW3	    0x00a1
#define PIC1_ICW4	    0x00a1

void inthandler21(int *esp);
void inthandler2c(int *esp);
void inthandler27(int *esp);

// 用数组和指针实现一个循环的FIFO型缓冲区
struct KEYBUF {
    unsigned char data[32];
    int next_r, next_w, len;    // 分别记录取数的下一个、存数的下一个以及长度（存了多少个）
};

