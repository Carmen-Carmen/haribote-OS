// bootpack.h

// ====================   CONSTANTS  ====================

// asmhead.asm
#define ADR_BOOTINFO            0x00000ff0  // BOOT_INFO��0x0ff0?�n
#define ADR_DISKIMG             0x00100000  // ��??�t�n���i��?�����j

//graphic.c
#define COL8_000000             0           // �K
#define COL8_FF0000             1           // ��?
#define COL8_00FF00             2           // ����
#define COL8_FFFF00             3           // ��?
#define COL8_0000FF             4           // ��?
#define COL8_FF00FF             5           // ����
#define COL8_00FFFF             6           // ��?
#define COL8_FFFFFF             7           // ��
#define COL8_C6C6C6             8           // ���D
#define COL8_840000             9           // ��?
#define COL8_008400             10          // ��?
#define COL8_848400             11          // �É�
#define COL8_000084             12          // ��?
#define COL8_840084             13          // ����
#define COL8_008484             14          // ����?
#define COL8_848484             15          // �[�D

#define MOUSE_XSIZE             16
#define MOUSE_YSIZE             16

// dsctbl.c
#define ADR_IDT                 0x0026f800  // IDT�\�I�N�n�n��
#define LIMIT_IDT               0x000007ff
#define ADR_GDT                 0x00270000  // GDT�\�I�N�n�n��
#define LIMIT_GDT               0x0000ffff
#define ADR_BOTPAK              0x00280000  // bootpack�I�N�n�n��
#define LIMIT_BOTPAK            0x0007ffff
#define AR_DATA32_RW            0x4092      // �����i�C��?����
#define AR_CODE32_ER            0x409a      // ��?�i�C��?�s + ��?
#define AR_TSS32                0x0089      // �C?��?�i
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
#define KEYCMD_LED              0xed    // 0b11101101�C�T��??LED���I�w��?�H

// mouse.c
#define KEYCMD_SENDTO_MOUSE     0xd4
#define MOUSECMD_ENABLE         0xf4

// memory.h
#define EFLAGS_AC_BIT           0x00040000  // eflags Allow Cache bit��eflags�I��18��
#define CR0_CACHE_DISABLE       0x60000000

#define MEMMAN_FREES            4090        // ��?��32KB
#define MEMMAN_ADDR             0x003c0000

// sheet.c
#define MAX_SHEETS              256
#define SHEET_USE               1

// timer.c
#define PIT_CTRL                0x0043
#define PIT_CNT0                0x0040

#define MAX_TIMER               500
#define TIMER_FLAGS_ALLOC       1       // ��?��?���ߔz�u��?�i��?�s
#define TIMER_FLAGS_USING       2       // ��?��?�s��

// multitask.c
#define MAX_TASKS               1000    // �ő�C?����
#define TASK_GDT0               3       // ��?��GDT�I�{��?�n���z?TSS
#define MAX_TASKS_LV            100     // ?��??�I�ő�C?��
#define MAX_TASKLEVELS          10      // �ő�C???

// ====================  STRUCTURES  ====================

// asmhead.asm
struct BOOTINFO {       // 0x0ff0~0x0fff
    char cyls;          // ??��???�I?���ʒu
    char leds;          // ?????����?
    char vmode;         // ?�������C�@�������F
    char reserve;           
    short scrnx, scrny; // ������            
    char *vram;         // ?��?�t��N�n�n��                         
};

// dsctbl.c
struct SEGMENT_DESCRIPTOR {
        /* 
        !!!!!!??�̒����������I?��������Œ�I�C��??�Z�������I?������Œ�?����I�C�����A?��
        �i�I����
        short base_low;
        char base_mid;
        char base_high;
        
        �i�I����
        short limit_low;
        char limit_high;
        
        �i�I�Ǘ�����
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
    unsigned char *buf; // ?�t��I??�e���F�w?�i�A����?�j
    int p,              // ?����i�ʓ��j
        q,              // ?��?�i?���j
        size,           // ?�����
        free,           //?�t�晔�]�ʒu
        flags;          // ���o?�u
};

struct FIFO32 {
    int *buf;
    int p, q, size ,free, flags;
    struct TASK *task;
};

// mouse.c
struct MOUSE_DEC {          // mouse decoded�C�������͑l??���M���I??��
    unsigned char buf[3];
    unsigned char phase;
    int x, y, btn;          // �������O�l?�Ix�Ay��?�ȋy����?�I��?
};

// memory.h
struct FREEINFO {           // �`�q�p���������M���I??��
    unsigned int addr, size;// �����N�n�n���a����
};

struct MEMMAN {     // MEMory MANager�����Ǘ��I??��
    int frees, maxfrees, lostsize, losts;
    struct FREEINFO free[MEMMAN_FREES];
};

// sheet.c
struct SHEET {          // �`�q����??�I??���C32Byte
    unsigned char *buf; // ????�����`����e�I�n��
    int bxsize,         // block x size
        bysize, 
        vx0,            // vram���Ix0��?
        vy0, 
        col_inv,        // �����F�F�� color_invisible
        height,         // ??���x
        flags;          // �����L???�I�e??���M��
    struct SHTCTL *ctl; // ��??�T�����w?��???�I���������C??���@??�I�����A�s���v�cctl��?�Q��?���i�����s�p�l?SHTCTL??������?��??���@�����I??�C�\��??���I�����w?�C����?�x�����4Byte
};

struct SHTCTL {         // sheet controller
    unsigned char *vram,                // vram�n��
                    *map;               // ??���x?�n�Imap�C??�����I�_��?��??�I���f�C���V�ꕔ???�����V���I���f�_����
    int xsize, ysize, top;              // ��ʑ����i?����BOOTINFO��??����?�j�Ctop������???�I���x
    struct SHEET *sheets[MAX_SHEETS];   // ��???�I�n���C�����x�����r��
    struct SHEET sheets0[MAX_SHEETS];   // ����256��??�I�M���C?��??�I?���������I�i???�g�p?���j
};

// timer.c
struct TIMER {                  // ��?��??��
    struct TIMER *next;         // next�w?�w��������������?�Itimer
    unsigned int timeout, flags;// ��? & ??��I��?
    struct FIFO32 *fifo;        // ������?�������I?�t��
    int data;                   // ��?�������C�p������s����?��
};

struct TIMERCTL {
    unsigned int    count,              // ���O??
                    next_time;          // ��������??�� 
    struct TIMER *t0;                   // t0�w?�w��?�\?
    struct TIMER timers0[MAX_TIMER];    // ?��sheet�I?���Ctimers0��?������?���Ltimer
};

// multitask.c
struct TSS32 {  // Task Status Segment 32 Bit
    int backlink, esp0, ss0, esp1, ss1, esp2, ss2, cr3, 
        eip, eflags, eax, ecx, edx, ebx, esp, ebp, esi, edi, 
        es, cs, ss, ds, fs, gs, 
        ldtr, iomap;
};

struct TASK {
    int sel, flags;     // sel: ����GDT?��
    int level, priority;// �C??��?�C?��?�z���C?����?��?���C??�s�I??�A�z�v
    struct FIFO32 fifo;
    struct TSS32 tss;
};

struct TASKLEVEL{
    int running;
    int now;
    struct TASK *tasks[MAX_TASKS_LV];   // ����??�ő�100���C?
};

struct TASKCTL {
    int now_lv;             // ?��?�s���ILEVEL
    char lv_change;         // �����C?��??�������v��?LEVEL
    struct TASKLEVEL level[MAX_TASKLEVELS]; // �ő��L10���C???�C??�ő�100���C�ꋤ1000��
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
    unsigned char name[8], ext[3], type;    // �������A?�W���A����?�^�i��?���j
    char reserve[10];                       // ��?���i�ۗ���10����?
    unsigned short time, date, clustno;     // ����??�A���������a�����icluster number�j
    unsigned int size;                      // ��������
};

// ====================   FUNCTIONS  ====================

// naskfunc.asm
void io_hlt(void);
void io_cli(void);  // ?���f close interrupt / clear interrupt flag�C?�V�A���c���f?�u�u?0
void io_sti(void);  // ?���f SeT Interrupt
void io_stihlt(void);   // ?���f�@halt
void io_out8(int port, int data); // ���w��io????8���i1 Byte�j�����I����
int io_in8(int port);
int io_load_eflags(void);   // �ԉ񓖑OEFLAGS���풆�����C�������f?�u
void io_store_eflags(int eflags);    // �c���L?�u����EFLAGS���풆
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
