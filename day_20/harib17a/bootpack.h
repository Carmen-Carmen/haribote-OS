// bootpack.h

// ====================   CONSTANTS  ====================

// asmhead.asm
#define ADR_BOOTINFO            0x00000ff0  // BOOT_INFO从0x0ff0开始
#define ADR_DISKIMG             0x00100000  // 磁盘缓冲地址（虚拟模式）

//graphic.c
#define COL8_000000             0           // 黑
#define COL8_FF0000             1           // 亮红
#define COL8_00FF00             2           // 亮黄
#define COL8_FFFF00             3           // 亮绿
#define COL8_0000FF             4           // 亮蓝
#define COL8_FF00FF             5           // 亮紫
#define COL8_00FFFF             6           // 浅亮蓝
#define COL8_FFFFFF             7           // 白
#define COL8_C6C6C6             8           // 亮灰
#define COL8_840000             9           // 暗红
#define COL8_008400             10          // 暗绿
#define COL8_848400             11          // 暗黄
#define COL8_000084             12          // 暗蓝
#define COL8_840084             13          // 暗紫
#define COL8_008484             14          // 浅暗蓝
#define COL8_848484             15          // 深灰

#define MOUSE_XSIZE             16
#define MOUSE_YSIZE             16

// dsctbl.c
#define ADR_IDT                 0x0026f800  // IDT表的起始地址
#define LIMIT_IDT               0x000007ff
#define ADR_GDT                 0x00270000  // GDT表的起始地址
#define LIMIT_GDT               0x0000ffff
#define ADR_BOTPAK              0x00280000  // bootpack的起始地址
#define LIMIT_BOTPAK            0x0007ffff
#define AR_DATA32_RW            0x4092      // 数据段，可读可写
#define AR_CODE32_ER            0x409a      // 代码段，可执行 + 可读
#define AR_TSS32                0x0089      // 任务状态段
#define AR_INTGATE32            0x008e

// int.c
#define PIC0_ICW1	        0x0020
#define PIC0_OCW2   	        0x0020
#define PIC0_IMR                0x0021
#define PIC0_ICW2	        0x0021
#define PIC0_ICW3	        0x0021
#define PIC0_ICW4	        0x0021
#define PIC1_ICW1	        0x00a0
#define PIC1_OCW2	        0x00a0
#define PIC1_IMR	        0x00a1
#define PIC1_ICW2	        0x00a1
#define PIC1_ICW3	        0x00a1
#define PIC1_ICW4	        0x00a1

// keyboard.c
#define PORT_KEYDAT             0x0060
#define PORT_KEYSTA             0x0064
#define PORT_KEYCMD             0x0064
#define KEYSTA_SEND_NOTREADY    0x02
#define KEYCMD_WRITE_MODE       0x60
#define KBC_MODE                0x47
#define KEYCMD_LED              0xed    // 0b11101101，控制键盘LED灯的指令码？

// mouse.c
#define KEYCMD_SENDTO_MOUSE     0xd4
#define MOUSECMD_ENABLE         0xf4

// memory.h
#define EFLAGS_AC_BIT           0x00040000  // eflags Allow Cache bit是eflags的第18位
#define CR0_CACHE_DISABLE       0x60000000

#define MEMMAN_FREES            4090        // 大约是32KB
#define MEMMAN_ADDR             0x003c0000

// sheet.c
#define MAX_SHEETS              256
#define SHEET_USE               1

// timer.c
#define PIT_CTRL                0x0043
#define PIT_CNT0                0x0040

#define MAX_TIMER               500
#define TIMER_FLAGS_ALLOC       1       // 定时器处于已配置状态（未运行
#define TIMER_FLAGS_USING       2       // 定时器运行中

// multitask.c
#define MAX_TASKS               1000    // 最大任务数量
#define TASK_GDT0               3       // 定义从GDT的几号开始分配给TSS
#define MAX_TASKS_LV            100     // 每个层级的最大任务数
#define MAX_TASKLEVELS          10      // 最大任务层级

// ====================  STRUCTURES  ====================

// asmhead.asm
struct BOOTINFO {       // 0x0ff0~0x0fff
    char cyls;          // 启动区读软盘的结束位置
    char leds;          // 启动时键盘灯状态
    char vmode;         // 图像模式，如多少位色
    char reserve;           
    short scrnx, scrny; // 分辨率            
    char *vram;         // 图像缓冲区起始地址                         
};

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

// fifo.c
struct FIFO8 {
    unsigned char *buf; // 缓冲区的实际容器：指针（就是数组）
    int p,              // 队列尾（写入）
        q,              // 队列头（读取）
        size,           // 队列大小
        free,           //缓冲区剩余位置
        flags;          // 溢出标志
};

struct FIFO32 {
    int *buf;
    int p, q, size ,free, flags;
    struct TASK *task;
};

// mouse.c
struct MOUSE_DEC {          // mouse decoded，存放解析鼠标传回信息的结构体
    unsigned char buf[3];
    unsigned char phase;
    int x, y, btn;          // 存放当前鼠标的x、y坐标以及按下按键的状态
};

// memory.h
struct FREEINFO {           // 描述可用内存区域信息的结构体
    unsigned int addr, size;// 包括起始地址和大小
};

struct MEMMAN {     // MEMory MANager内存管理的结构体
    int frees, maxfrees, lostsize, losts;
    struct FREEINFO free[MEMMAN_FREES];
};

// sheet.c
struct SHEET {          // 描述透明图层的结构体，32Byte
    unsigned char *buf; // 记录图层上所描画内容的地址
    int bxsize,         // block x size
        bysize, 
        vx0,            // vram中的x0坐标
        vy0, 
        col_inv,        // 透明色色号 color_invisible
        height,         // 图层高度
        flags;          // 存放有关图层的各种设定信息
    struct SHTCTL *ctl; // 将图层控制器指针作为图层的一个属性，这样以后图层的操作就不需要把ctl作为参数传入（似乎不用考虑SHTCTL结构体是在这个结构体后声明的问题，可能因为传入的只是指针，所以长度一定是4Byte
};

struct SHTCTL {         // sheet controller
    unsigned char *vram,                // vram地址
                    *map;               // 图层高度关系的map，记录画面上的点是哪个图层的像素，刷新底部图层时只刷新它的像素点即可
    int xsize, ysize, top;              // 画面大小（每次从BOOTINFO中查询太麻烦），top是最上层图层的高度
    struct SHEET *sheets[MAX_SHEETS];   // 存储图层的地址，按高度升序排列
    struct SHEET sheets0[MAX_SHEETS];   // 存放256个图层的信息，这些图层的顺序是混乱的（实际为使用顺序）
};

// timer.c
struct TIMER {                  // 定时器结构体
    struct TIMER *next;         // next指针指向下一个即将超时的timer
    unsigned int timeout, flags;// 超时 & 计时器的状态
    struct FIFO32 *fifo;        // 存放定时器数据的缓冲区
    int data;                   // 定时器数据，用于区分不同定时器
};

struct TIMERCTL {
    unsigned int    count,              // 当前计时
                    next_time;          // 下一个超时时刻 
    struct TIMER *t0;                   // t0指针指向链表头
    struct TIMER timers0[MAX_TIMER];    // 类似sheet的处理，timers0数组乱序存储所有timer
};

// multitask.c
struct TSS32 {  // Task Status Segment 32 Bit
    int backlink, esp0, ss0, esp1, ss1, esp2, ss2, cr3, 
        eip, eflags, eax, ecx, edx, ebx, esp, ebp, esi, edi, 
        es, cs, ss, ds, fs, gs, 
        ldtr, iomap;
};

struct TASK {
    int sel, flags;     // sel: 存放GDT编号
    int level, priority;// 任务优先级，优先级越高，每次切换到这个任务执行的时间就越久
    struct FIFO32 fifo;
    struct TSS32 tss;
};

struct TASKLEVEL{
    int running;
    int now;
    struct TASK *tasks[MAX_TASKS_LV];   // 一个层级最多100个任务
};

struct TASKCTL {
    int now_lv;             // 现在运行中的LEVEL
    char lv_change;         // 下次任务切换时是否需要改变LEVEL
    struct TASKLEVEL level[MAX_TASKLEVELS]; // 最多有10个任务层级，每层最多100个，一共1000个
    struct TASK tasks0[MAX_TASKS];      
};

// console.c
struct CONSOLE {
    struct SHEET *sht;
    int cur_x, cur_y, cur_c;
    int multi_line;
    int size_x, size_y;
};

// file.c
struct FILEINFO {
    unsigned char name[8], ext[3], type;    // 文件名、扩展名、文件类型（只读等）
    char reserve[10];                       // 微软公司保留了10个字节
    unsigned short time, date, clustno;     // 存放时间、存放日期和蔟号（cluster number）
    unsigned int size;                      // 文件大小
};

// ====================   FUNCTIONS  ====================

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
int load_cr0(void);
void store_cr0(int cr0);
void asm_inthandler21(void);
void asm_inthandler27(void);
void asm_inthandler2c(void);
void asm_inthandler20(void);
unsigned int memtest_sub(unsigned int start, unsigned int end);
void load_tr(int tr);
void farjmp(int eip, int cs);

// graphic.c
void init_palette(void);
void set_palette(int start, int end, unsigned char *rgb);
void boxfill8(unsigned char *vram, int xsize, unsigned char c, int x0, int y0, int x1, int y1);
void putfont8(char *vram, int xsize, int x, int y, char c, char *font);
void putstring8_ascii(char *vram, int xsize, int x, int y, char c, unsigned char *s);
void init_screen8(char *vram, int x, int y);
void init_mouse_cursor8(char *mouse, char bc);
void putblock8(char *vram, int vxsize, int pxsize, int pysize, int px0, int py0, char *p_addr, int bxsize); 
void putstring8_ascii_sht(struct SHEET *sht, int x, int y, int c, int b, char *s);

// dsctbl.c
void init_gdtidt(void); 
void set_segmdesc(struct SEGMENT_DESCRIPTOR *sd, unsigned int limit, int base, int ar); 
void set_gatedesc(struct GATE_DESCRIPTOR *gd, int offset, int selector, int ar); 

// int.c
void init_pic(void);
void inthandler27(int *esp);

// fifo.c
void fifo8_init(struct FIFO8 *fifo, int size, unsigned char *buf);
int fifo8_put(struct FIFO8 *fifo, unsigned char data);
int fifo8_get(struct FIFO8 *fifo);
int fifo8_status(struct FIFO8 *fifo);

int fifo32_put(struct FIFO32 *fifo, int data);
int fifo32_get(struct FIFO32 *fifo);
int fifo32_status(struct FIFO32 *fifo);
void fifo32_init(struct FIFO32 *fifo, int size, int *buf, struct TASK *task);

// keyboard.c
void wait_KBC_sendready(void);
void init_keyboard(struct FIFO32 *fifo, int data0);
void inthandler21(int *esp);

// mouse.c
void enable_mouse(struct FIFO32 *fifo, int data0, struct MOUSE_DEC *mdec);
int mouse_decode(struct MOUSE_DEC *mdec, unsigned char data);
void inthandler2c(int *esp);

// memory.h
unsigned int memtest(unsigned int start, unsigned int end);

void memman_init(struct MEMMAN *man);
unsigned int memman_total(struct MEMMAN *man);
unsigned int memman_alloc(struct MEMMAN *man, unsigned int size);
int memman_free(struct MEMMAN *man, unsigned int addr, unsigned int size);
unsigned int memman_alloc_4k(struct MEMMAN *man, unsigned int size);
int memman_free_4k(struct MEMMAN *man, unsigned int addr, unsigned int size);

// sheet.c
struct SHTCTL *shtctl_init(struct MEMMAN *memman, unsigned char *vram, int xsize, int ysize);
struct SHEET *sheet_alloc(struct SHTCTL *ctl);
void sheet_setbuf(struct SHEET *sht, unsigned char *buf, int xsize, int ysize, int col_inv);
void sheet_updown(struct SHEET *sht, int height);
void sheet_slide(struct SHEET *sht, int vx0, int vy0);
void sheet_refresh(struct SHEET *sht, int bx0, int by0, int bx1, int by1);
void sheet_refreshsub(struct SHTCTL *ctl, int vx0, int vy0, int vx1, int vy1, int h0, int h1);
void sheet_refreshmap(struct SHTCTL *ctl, int vx0, int vy0, int vx1, int vy1, int h0);
void sheet_free(struct SHEET *sht);

// timer.c
extern struct TIMERCTL timerctl;

void init_pit(void);
struct TIMER *timer_alloc(void);
void timer_free(struct TIMER *timer);
void timer_init(struct TIMER *timer, struct FIFO32 *fifo, int data);
void timer_settime(struct TIMER *timer, unsigned int timeout);
void inthandler20(int *esp);
void reset_timerctl_count(void);
void set490(struct FIFO32 *fifo, int mode);

// multitask.c
extern struct TASKCTL *taskctl;
extern struct TIMER *task_timer;

struct TASK *task_init(struct MEMMAN *memman);
struct TASK *task_alloc(void); 
void task_run(struct TASK *task, int level, int priority); 
void task_switch(void); 
struct TASK *task_now(void);
void task_sleep(struct TASK *task);
void task_add(struct TASK *task);
void task_remove(struct TASK *task); 
void task_switchsub(void); 
void task_idle(void);

// console.c
void console_task(struct SHEET *sheet, unsigned int memtotal, int size_x, int size_y);
void scroll_down(struct SHEET *sheet, int x_init, int y_init, int xsize, int ysize);
void cons_newline(struct CONSOLE *cons);
void cons_putchar(struct CONSOLE *cons, int chr, char move, int cc, int bc);
void cons_putstring(struct CONSOLE *cons, char *string, int cc, int bc, unsigned int start, unsigned int end);
void cons_backspace(struct CONSOLE *cons, char *cmdline, int cmdindex);
void cons_runcmd(char *cmdline, struct CONSOLE *cons, int *fat, unsigned int memtotal);

void cmd_mem(struct CONSOLE *cons, unsigned int memtotal);
void cmd_clear(struct CONSOLE *cons);
void cmd_ls(struct CONSOLE *cons);
void cmd_cat(struct CONSOLE *cons, int *fat, char *cmdline);
void cmd_fat(struct CONSOLE *cons, int *fat);
void cmd_hlt(struct CONSOLE *cons, int *fat);
void cmd_time(struct CONSOLE *cons);

// window.c
void make_window8(unsigned char *buf, int xsize, int ysize, char *title, char act);
void make_textbox8(struct SHEET *sht, int x0, int y0, int sx, int sy, int c);
void make_wtitle8(unsigned char *buf, int xsize, char *title, char act);

// file.c
struct FILEINFO *file_search(char *name, struct FILEINFO *finfo, int max);
void file_decodefat(int *fat, unsigned char *img);
void file_loadfile(int clustno, int size, char *buf, int *fat, char *img);
