[FORMAT "WCOFF"]
[INSTRSET "i486p"]
[BITS 32]
[FILE "api001.asm"]

    GLOBAL  _api_putchar

[SECTION .text]

_api_putchar:       ; void api_putchar(int c);
    MOV     EDX, 1              ; 向EDX赋值，表示调用cons_putchar函数
    MOV     AL, [ESP + 4]       ; int c压栈（4 Byte）
    INT     0x40                ; 以中断的形式调用
    RET
