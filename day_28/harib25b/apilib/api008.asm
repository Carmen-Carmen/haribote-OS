[FORMAT "WCOFF"]
[INSTRSET "i486p"]
[BITS 32]
[FILE "api008.asm"]

    GLOBAL  _api_initmalloc

[SECTION .text]

_api_initmalloc:    ; void api_initmalloc(void);
    PUSH    EBX
    MOV     EDX, 8
    MOV     EBX, [CS:0x0020]    ; memman在应用程序内存中的地址
    MOV     EAX, EBX
    ADD     EAX, 32 * 1024      ; 加上32KB，多申请32KB用于存储MEMMAN结构体
    MOV     ECX, [CS:0x0000]    ; 数据段的大小
    SUB     ECX, EAX            ; 数据段大小 - memman管理空间的起始地址
    INT     0x40
    POP     EBX
    RET
