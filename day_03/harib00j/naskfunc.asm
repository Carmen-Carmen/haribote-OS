; naskfun
; TAB=4

[FORMAT "WCOFF"]        ; 制作目标文件的模式
[BITS 32]               ; 制作32位模式用的机械语言

; 制作目标文件的信息

[FILE "naskfunc.asm"]   ; 源文件名信息
    
    GLOBAL  _io_hlt     ; 程序中包含的函数名，用GLOBAL指令声明，必须在函数名前加上 _ ，才能正确的与C语言函数链接

; 以下是实际的函数

[SECTION .text]         ; 目标文件中写了这些之后再写程序

_io_hlt:                ; 即C语言中的void io_hlt(void);
    HLT
    RET                 ; 相当于C语言中的return，表示这个函数结束了
