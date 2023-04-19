; naskfunc
; TAB=4

[FORMAT "WCOFF"]        ; 制作目标文件的模式
[INSTRSET "i486p"]      ; INSTRuction SET，表示这个程序是给i486机器使用的（能用32位寄存器的CPU）
[BITS 32]               ; 制作32位模式用的机械语言

; 制作目标文件的信息

[FILE "naskfunc.asm"]   ; 源文件名信息
    
    GLOBAL  _io_hlt, _write_mem8     ; 程序中包含的函数名，用GLOBAL指令声明，必须在函数名前加上 _ ，才能正确的与C语言函数链接

; 以下是实际的函数

[SECTION .text]         ; 目标文件中写了这些之后再写程序

_io_hlt:                ; 即C语言中的void io_hlt(void);
    HLT
    RET                 ; 相当于C语言中的return，表示这个函数结束了

; 向指定内存中写入数据的函数
_write_mem8:                ; void write_mem8(int addr, int data):
    MOV     ECX, [ESP + 4]  ; 将内存地址ESP + 4的存储单元中存储的是欲写入的内地址，将其读入扩展计数寄存器ECX
    MOV     AL, [ESP + 8]   ; 内存地址ESP + 4的存储单元中存储的数据，读入AL寄存器
    MOV     [ECX], AL       ; 最后把AL中存储的欲写入的数据，写入ECX中存储的欲写入内存地址的内存单元中
    RET
