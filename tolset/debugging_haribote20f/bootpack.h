// bootpack.h

// ====================   CONSTANTS  ====================

// asmhead.asm
#define ADR_BOOTINFO            0x00000ff0  // BOOT_INFOÿ¸0x0ff0?ÿn
#define ADR_DISKIMG             0x00100000  // ÿ¥??™t’nÿ¬ÿi‹•?–Íÿ®ÿj

//graphic.c
#define COL8_000000             0           // üK
#define COL8_FF0000             1           // —º?
#define COL8_00FF00             2           // —º‰©
#define COL8_FFFF00             3           // —º?
#define COL8_0000FF             4           // —º?
#define COL8_FF00FF             5           // —ºÿ‡
#define COL8_00FFFF             6           // ÿó—º?
#define COL8_FFFFFF             7           // ”’
#define COL8_C6C6C6             8           // —ºÿD
#define COL8_840000             9           // ÿÃ?
#define COL8_008400             10          // ÿÃ?
#define COL8_848400             11          // ÿÃ‰©
#define COL8_000084             12          // ÿÃ?
#define COL8_840084             13          // ÿÃÿ‡
#define COL8_008484             14          // ÿóÿÃ?
#define COL8_848484             15          // ÿ[ÿD

#define MOUSE_XSIZE             16
#define MOUSE_YSIZE             16

// dsctbl.c
#define ADR_IDT                 0x0026f800  // IDT•\“I‹Nÿn’nÿ¬
#define LIMIT_IDT               0x000007ff
#define ADR_GDT                 0x00270000  // GDT•\“I‹Nÿn’nÿ¬
#define LIMIT_GDT               0x0000ffff
#define ADR_BOTPAK              0x00280000  // bootpack“I‹Nÿn’nÿ¬
#define LIMIT_BOTPAK            0x0007ffff
#define AR_DATA32_RW            0x4092      // ÿ”ÿÿ’iÿC‰Â?‰ÂÿÊ
#define AR_CODE32_ER            0x409a      // ‘ã?’iÿC‰Â?ÿs + ‰Â?
#define AR_TSS32                0x0089      // ”C?ÿó?’i
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
#define KEYCMD_LED              0xed    // 0b11101101ÿCÿTÿ§??LED“”“Iÿw—ß?ÿH

// mouse.c
#define KEYCMD_SENDTO_MOUSE     0xd4
#define MOUSECMD_ENABLE         0xf4

// memory.h
#define EFLAGS_AC_BIT           0x00040000  // eflags Allow Cache bitÿ¥eflags“I‘æ18ÿÊ
#define CR0_CACHE_DISABLE       0x60000000

#define MEMMAN_FREES            4090        // ‘å?ÿ¥32KB
#define MEMMAN_ADDR             0x003c0000

// sheet.c
#define MAX_SHEETS              256
#define SHEET_USE               1

// timer.c
#define PIT_CTRL                0x0043
#define PIT_CNT0                0x0040

#define MAX_TIMER               500
#define TIMER_FLAGS_ALLOC       1       // ’è?ÿí?ÿ°›ß”z’uÿó?ÿi–¢?ÿs
#define TIMER_FLAGS_USING       2       // ’è?ÿí?ÿs’†

// multitask.c
#define MAX_TASKS               1000    // ÿÅ‘å”C?ÿ”—Ê
#define TASK_GDT0               3       // ’è?ÿ¸GDT“I™{ÿ†?ÿn•ÿ”z?TSS
#define MAX_TASKS_LV            100     // ?ÿ¢??“IÿÅ‘å”C?ÿ”
#define MAX_TASKLEVELS          10      // ÿÅ‘å”C???

// ====================  STRUCTURES  ====================

// asmhead.asm
struct BOOTINFO {       // 0x0ff0~0x0fff
    char cyls;          // ??‹æ???“I?‘©ÿÊ’u
    char leds;          // ?????“”ÿó?
    char vmode;         // ?‘ÿ–Íÿ®ÿC”@‘½ÿ­ÿÊÿF
    char reserve;           
    short scrnx, scrny; // •ÿ™ÿ—¦            
    char *vram;         // ?‘ÿ?™t‹æ‹Nÿn’nÿ¬                         
};

// dsctbl.c
struct SEGMENT_DESCRIPTOR {
        /* 
        !!!!!!??‘Ì’†ÿº–¾‘®ÿ«“I?ÿÿÿê’èÿ¥ÿÅ’è“IÿCÿö??ÿZÿ÷ÿæÿ”“I?ÿó–çÿ¥ÿÅ’è?ÿÿÿæ“IÿC—ÿ—¹ÿA?—¹
        ’i“Iÿîÿ¬
        short base_low;
        char base_mid;
        char base_high;
        
        ’i“IÿãÿÀ
        short limit_low;
        char limit_high;
        
        ’i“IÿÇ—ÿ‘®ÿ«
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
    unsigned char *buf; // ?™t‹æ“I??—eÿíÿFÿw?ÿiÿAÿ¥ÿ”?ÿj
    int p,              // ?—ñ”öÿiÿÊ“üÿj
        q,              // ?—ñ?ÿi?ÿæÿj
        size,           // ?—ñ‘åÿ¬
        free,           //?™t‹æ™”—]ÿÊ’u
        flags;          // ÿìÿo?ÿu
};

struct FIFO32 {
    int *buf;
    int p, q, size ,free, flags;
    struct TASK *task;
};

// mouse.c
struct MOUSE_DEC {          // mouse decodedÿC‘¶•ú‰ðÿÍ‘l??‰ñÿM‘§“I??‘Ì
    unsigned char buf[3];
    unsigned char phase;
    int x, y, btn;          // ‘¶•ú“–‘O‘l?“IxÿAyÿ¿?ÿÈ‹yÿÂ‰ºÿÂ?“Iÿó?
};

// memory.h
struct FREEINFO {           // •`ÿq‰Â—p“à‘¶‹æÿæÿM‘§“I??‘Ì
    unsigned int addr, size;// •ïÿ‡‹Nÿn’nÿ¬ÿa‘åÿ¬
};

struct MEMMAN {     // MEMory MANager“à‘¶ÿÇ—ÿ“I??‘Ì
    int frees, maxfrees, lostsize, losts;
    struct FREEINFO free[MEMMAN_FREES];
};

// sheet.c
struct SHEET {          // •`ÿq“§–¾??“I??‘ÌÿC32Byte
    unsigned char *buf; // ????ÿãÿÿ•`‰æ“à—e“I’nÿ¬
    int bxsize,         // block x size
        bysize, 
        vx0,            // vram’†“Ix0ÿ¿?
        vy0, 
        col_inv,        // “§–¾ÿFÿFÿ† color_invisible
        height,         // ??ÿ‚“x
        flags;          // ‘¶•ú—L???“Iÿe??’èÿM‘§
    struct SHTCTL *ctl; // ÿ«??ÿTÿ§ÿíÿw?ÿì???“Iÿêÿ¢‘®ÿ«ÿC??ÿÈÿ@??“I‘€ÿìÿA•sÿù—v”cctlÿì?ÿQÿ”?“üÿiÿ—ÿÁ•s—pÿl?SHTCTL??‘Ìÿ¥ÿÝ?ÿ¢??‘Ìÿ@ÿº–¾“I??ÿC‰Â”\ÿö??“ü“I‘üÿ¥ÿw?ÿCÿÿÿÈ?“xÿê’èÿ¥4Byte
};

struct SHTCTL {         // sheet controller
    unsigned char *vram,                // vram’nÿ¬
                    *map;               // ??ÿ‚“x?ÿn“ImapÿC??‰æ–Êÿã“I“_ÿ¥?ÿ¢??“I‘ÿ‘fÿCÿüÿV’ê•”???‘üÿüÿV›€“I‘ÿ‘f“_‘¦‰Â
    int xsize, ysize, top;              // ‰æ–Ê‘åÿ¬ÿi?ÿÿÿ¸BOOTINFO’†??‘¾–ƒ?ÿjÿCtopÿ¥ÿÅÿã???“Iÿ‚“x
    struct SHEET *sheets[MAX_SHEETS];   // ‘¶???“I’nÿ¬ÿCÿÂÿ‚“xÿ¡ÿÿ”r—ñ
    struct SHEET sheets0[MAX_SHEETS];   // ‘¶•ú256ÿ¢??“IÿM‘§ÿC?ÿ±??“I?ÿÿÿ¥ÿ¬—ÿ“Iÿi???ÿg—p?ÿÿÿj
};

// timer.c
struct TIMER {                  // ’è?ÿí??‘Ì
    struct TIMER *next;         // nextÿw?ÿwÿü‰ºÿêÿ¢‘¦ÿ«’´?“Itimer
    unsigned int timeout, flags;// ’´? & ??ÿí“Iÿó?
    struct FIFO32 *fifo;        // ‘¶•ú’è?ÿíÿ”ÿÿ“I?™t‹æ
    int data;                   // ’è?ÿíÿ”ÿÿÿC—pÿ°‹æ•ÿ•s“¯’è?ÿí
};

struct TIMERCTL {
    unsigned int    count,              // “–‘O??
                    next_time;          // ‰ºÿêÿ¢’´??ÿÿ 
    struct TIMER *t0;                   // t0ÿw?ÿwÿü?•\?
    struct TIMER timers0[MAX_TIMER];    // ?ÿ—sheet“I?—ÿÿCtimers0ÿ”?—ÿÿÿ‘¶?ÿÿ—Ltimer
};

// multitask.c
struct TSS32 {  // Task Status Segment 32 Bit
    int backlink, esp0, ss0, esp1, ss1, esp2, ss2, cr3, 
        eip, eflags, eax, ecx, edx, ebx, esp, ebp, esi, edi, 
        es, cs, ss, ds, fs, gs, 
        ldtr, iomap;
};

struct TASK {
    int sel, flags;     // sel: ‘¶•úGDT?ÿ†
    int level, priority;// ”C??ÿæ?ÿC?ÿæ?‰zÿ‚ÿC?ÿÿÿØ?“ÿ?ÿ¢”C??ÿs“I??ÿA‰z‹v
    struct FIFO32 fifo;
    struct TSS32 tss;
};

struct TASKLEVEL{
    int running;
    int now;
    struct TASK *tasks[MAX_TASKS_LV];   // ÿêÿ¢??ÿÅ‘½100ÿ¢”C?
};

struct TASKCTL {
    int now_lv;             // ?ÿÝ?ÿs’†“ILEVEL
    char lv_change;         // ‰ºÿÿ”C?ÿØ??ÿ¥”Ûÿù—v‰ü?LEVEL
    struct TASKLEVEL level[MAX_TASKLEVELS]; // ÿÅ‘½—L10ÿ¢”C???ÿC??ÿÅ‘½100ÿ¢ÿCÿê‹¤1000ÿ¢
    struct TASK tasks0[MAX_TASKS];      
};

// console.c
struct CONSOLE {
    struct SHEET *sht;
    int cur_x, cur_y, cur_c;
    int multi_line;
    int size_x, size_y;
    struct TIMER *timer;
};

// file.c
struct FILEINFO {
    unsigned char name[8], ext[3], type;    // •¶ÿÿ–¼ÿA?“W–¼ÿA•¶ÿÿ?ÿ^ÿi‘ü?“™ÿj
    char reserve[10];                       // ”÷?ÿöÿi•Û—¯—¹10ÿ¢ÿÿ?
    unsigned short time, date, clustno;     // ‘¶•ú??ÿA‘¶•ú“úÿúÿaäõÿ†ÿicluster numberÿj
    unsigned int size;                      // •¶ÿÿ‘åÿ¬
};

// ====================   FUNCTIONS  ====================

// naskfunc.asm
void io_hlt(void);
void io_cli(void);  // ?’†’f close interrupt / clear interrupt flagÿC?”VÿAÿ¥”c’†’f?ÿu’u?0
void io_sti(void);  // ?’†’f SeT Interrupt
void io_stihlt(void);   // ?’†’fÿ@halt
void io_out8(int port, int data); // ÿüÿw’èio????8ÿÊÿi1 Byteÿjÿ”ÿÿ“I”ÿÿ”
int io_in8(int port);
int io_load_eflags(void);   // •Ô‰ñ“–‘OEFLAGSÿñ‘¶ÿí’†ÿ”ÿÿÿC•ïÿ‡’†’f?ÿu
void io_store_eflags(int eflags);    // ”cÿÿ—L?ÿu‘¶‰ñEFLAGSÿñ‘¶ÿí’†
void load_gdtr(int limit, int addr);
void load_idtr(int limit, int addr);
int load_cr0(void);
void store_cr0(int cr0);
void asm_inthandler21(void);
void asm_inthandler27(void);
void asm_inthandler2c(void);
void asm_inthandler20(void);
void asm_inthandler0d(void);
void asm_inthandler0c(void);
unsigned int memtest_sub(unsigned int start, unsigned int end);
void load_tr(int tr);
void farjmp(int eip, int cs);
void farcall(int eip, int cs);
// void asm_cons_putchar(struct CONSOLE *cons, int chr, char move, int cc, int bc);
void asm_hrb_api(int edi, int esi, int ebp, int esp, int ebx, int edx, int ecx, int eax);
void start_app(int eip, int cs, int esp, int ds, int *tss_esp0);
void asm_end_app(void);

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
char * get_ontime();

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
void console_task(struct SHEET *sheet, unsigned int memtotal);
void scroll_down(struct SHEET *sheet, int x_init, int y_init, int xsize, int ysize);
void cons_newline(struct CONSOLE *cons);
void cons_putchar(struct CONSOLE *cons, int chr, char move, int cc, int bc);
void cons_putstring(struct CONSOLE *cons, char *string, ...);
void cons_putstr0(struct CONSOLE *cons, char *s);
void cons_putstr1(struct CONSOLE *cons, char *s, int len);
void cons_backspace(struct CONSOLE *cons, char *cmdline, int cmdindex);
void cons_runcmd(char *cmdline, struct CONSOLE *cons, int *fat, unsigned int memtotal);
void cons_omit_firstline(struct CONSOLE *cons);

void cmd_mem(struct CONSOLE *cons, unsigned int memtotal);
void cmd_clear(struct CONSOLE *cons);
void cmd_ls(struct CONSOLE *cons);
void cmd_cat(struct CONSOLE *cons, int *fat, char *cmdline);
void cmd_fat(struct CONSOLE *cons, int *fat);
void cmd_hlt(struct CONSOLE *cons, int *fat);
void cmd_time(struct CONSOLE *cons);
int cmd_app(struct CONSOLE *cons, int *fat, char *cmdline);
void cmd_echo(struct CONSOLE *cons, char *cmdline);
int * hrb_api(int edi, int esi, int ebp, int esp, int ebx, int edx, int ecx, int eax);
void hrb_api_linewin(struct SHEET *sht, int x0, int y0, int x1, int y1, int col);
int *inthandler0d(int *esp);
int *inthandler0c(int *esp);

// window.c
void make_window8(unsigned char *buf, int xsize, int ysize, char *title, char act);
void make_textbox8(struct SHEET *sht, int x0, int y0, int sx, int sy, int c);
void make_wtitle8(unsigned char *buf, int xsize, char *title, char act);

// file.c
struct FILEINFO *file_search(char *name, struct FILEINFO *finfo, int max);
void file_decodefat(int *fat, unsigned char *img);
void file_loadfile(int clustno, int size, char *buf, int *fat, char *img);
