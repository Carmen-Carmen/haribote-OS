[FORMAT "WCOFF"]    ; 生成文件的模式为"对象文件模式"，这样才能与a.c的编译结果进行连接
[INSTRSET "i486p"]   ; 使用486兼容指令集
[BITS 32]           ; 生成32位模式的机器语言
[FILE "a_asm.asm"]  ; 源文件名信息

    GLOBAL  _api_putchar, _api_putstr0
    GLOBAL  _api_openwin, _api_putstrwin, _api_boxfillwin
    GLOBAL  _api_initmalloc, _api_malloc, _api_free
    GLOBAL  _api_end

[SECTION .text]

_api_initmalloc:    ; void api_initmalloc(void);
    PUSH    EBX
    MOV     EDX, 8
    MOV     EBX, [CS:0x0020]; memman在应用程序内存中的地址
    MOV     EAX, EBX
    ADD     EAX, 32 * 1024  ; 加上32KB，多申请32KB用于存储MEMMAN结构体
    MOV     ECX, [CS:0x0000]; 数据段的大小
    SUB     ECX, EAX        ; 数据段大小 - memman管理空间的起始地址
    INT     0x40
    POP     EBX
    RET

_api_malloc:        ; char *api_malloc(int size);
    PUSH    EBX
    MOV     EDX, 9
    MOV     EBX, [CS:0x0020]
    MOV     ECX, [ESP + 8]  ; size
    INT     0x40
    POP     EBX
    RET

_api_free:          ; void api_free(char *addr, int size);
    PUSH    EBX
    MOV     EDX, 10
    MOV     EBX, [CS:0x0020]
    MOV     EAX, [ESP + 8]  ; addr
    MOV     ECX, [ESP + 12] ; size
    INT     0x40
    POP     EBX
    RET
    
_api_putstrwin:     ; void api_putstrwin(int win, int x, int y, int col, int len, char *s);
    PUSH    EDI
    PUSH    ESI
    PUSH    EBP
    PUSH    EBX
    MOV     EDX, 6
    MOV     EBX, [ESP + 20] ; win
    MOV     ESI, [ESP + 24] ; x
    MOV     EDI, [ESP + 28] ; y
    MOV     EAX, [ESP + 32] ; col
    MOV     ECX, [ESP + 36] ; len
    MOV     EBP, [ESP + 40] ; *s
    INT     0x40
    POP     EBX
    POP     EBP
    POP     ESI
    POP     EDI
    RET

_api_boxfillwin:     ; void api_boxfillwin(int win, int x0, int y0, int x1, int y1, int col);
    PUSH    EDI
    PUSH    ESI
    PUSH    EBP
    PUSH    EBX
    MOV     EDX, 7
    MOV     EBX, [ESP + 20] ; win
    MOV     EAX, [ESP + 24] ; x0
    MOV     ECX, [ESP + 28] ; 0
    MOV     ESI, [ESP + 32] ; x1
    MOV     EDI, [ESP + 36] ; y1
    MOV     EBP, [ESP + 40] ; col
    INT     0x40
    POP     EBX
    POP     EBP
    POP     ESI
    POP     EDI
    RET
    

_api_openwin:       ; int api_openwin(char *buf, int xsize, int ysize, int col_inv, char *title);
    PUSH    EDI
    PUSH    ESI
    PUSH    EBX
    MOV     EDX, 5
    MOV     EBX, [ESP + 16] ; buf
    MOV     ESI, [ESP + 20] ; xsize
    MOV     EDI, [ESP + 24] ; ysize
    MOV     EAX, [ESP + 28] ; col_inv
    MOV     ECX, [ESP + 32] ; title
    INT     0x40            ; 调用
    POP     EBX
    POP     ESI
    POP     EDI
    RET

_api_putchar:       ; void api_putchar(int c);
    MOV     EDX, 1          ; 向EDX赋值，表示调用cons_putchar函数
    MOV     AL, [ESP + 4]   ; int c压栈（4 Byte）
    INT     0x40            ; 以中断的形式调用
    RET

_api_putstr0:       ; void api_putstr(char *s);
    PUSH    EBX             ; 用EBX存储指针位置，因此先保存原来的EBX
    MOV     EDX, 2
    MOV     EBX, [ESP + 8]  ; char *指针压栈（4 Byte），因为之前压栈了EBX，所以用ESP + 8来获取参数s
    INT     0x40
    POP     EBX             ; 恢复EBX
    RET

_api_end:           ; void api_end(void);
    MOV     EDX, 4
    INT     0x40
