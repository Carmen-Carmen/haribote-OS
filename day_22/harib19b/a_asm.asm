[FORMAT "WCOFF"]    ; 生成文件的模式为"对象文件模式"，这样才能与a.c的编译结果进行连接
[INSTRSET "i486p"]   ; 使用486兼容指令集
[BITS 32]           ; 生成32位模式的机器语言
[FILE "a_asm.asm"]  ; 源文件名信息

    GLOBAL  _api_putchar, _api_end

[SECTION .text]

_api_putchar:       ; void api_putchar(int c);
    MOV     EDX, 1          ; 向EDX赋值，表示调用cons_putchar函数
    MOV     AL, [ESP + 4]   ; int c压栈
    INT     0x40            ; 以中断的形式调用
    RET

_api_end:           ; void api_end(void);
    MOV     EDX, 4
    INT     0x40
