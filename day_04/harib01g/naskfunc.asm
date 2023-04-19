; naskfunc
; TAB=4

[FORMAT "WCOFF"]        ; 制作目标文件的模式
[INSTRSET "i486p"]      ; INSTRuction SET，表示这个程序是给i486机器使用的（能用32位寄存器的CPU）
[BITS 32]               ; 制作32位模式用的机械语言
[FILE "naskfunc.asm"]   ; 源文件名信息
    ; 程序中包含的函数名，用GLOBAL指令声明，必须在函数名前加上 _ ，才能正确的与C语言函数链接
    GLOBAL  _io_hlt, _io_cli, _io_sti, _io_stihlt
    GLOBAL  _io_in8, _io_in16, _io_in32
    GLOBAL  _io_out8, _io_out16, _io_out32
    GLOBAL  _io_load_eflags, _io_store_eflags
    GLOBAL  _write_mem8 

; 以下是实际的函数

[SECTION .text]         ; 目标文件中写了这些之后再写程序

_io_hlt:                ; 即C语言中的void io_hlt(void);
    HLT
    RET                 ; 相当于C语言中的return，表示这个函数结束了

_io_cli: 
    CLI
    RET

_io_sti:
    STI
    RET

_io_stihlt:
    STI
    HLT
    RET

_io_in8: 
    MOV     EDX, [ESP + 4]  ; port
    MOV     EAX, 0          ; 把EAX寄存器置为0，用于接收
    IN      AX, DX          ; EDX中数据表示的端口的io设备的数据读入EAX
    RET

_io_in16: 
    MOV     EDX, [ESP + 4]
    MOV     EAX, 0
    IN      AX, DX
    RET

_io_in32: 
    MOV     EDX, [ESP + 4]
    IN      EAX, DX
    RET

_io_out8: 
    MOV     EDX, [ESP + 4]  ; port
    MOV     AL, [ESP + 8]   ; 把内存地址ESP + 8的存储单元中存储的数据读入AL寄存器
    OUT     DX, AL          ; 把AL寄存器中的数据写出到DX中数据表示的端口的io设备中
    RET

_io_out16: 
    MOV     EDX, [ESP + 4]
    MOV     EAX, [ESP + 8]
    OUT     DX, AX
    RET

_io_out32: 
    MOV     EDX, [ESP + 4]
    MOV     EAX, [ESP + 8]
    OUT     DX, EAX
    RET

; 能用于读写EFLAGS寄存器的只有PUSHFD和POPFD指令
_io_load_eflags:            ; int io_load_eflags(void)
    ; 相当于MOV EAX, EFLAGS
    PUSHFD                  ; PUSH EFLAGS double-word，就是将EFLAGS寄存器中存储的标志压入一个栈结构
    POP     EAX             ; 将上一句栈结构中数据弹出，并带入EAX，注意EFLAGS长32位，就要用同为32位的EAX来接收
    RET                     ; 这个函数的返回值就是EAX中的数据

_io_store_eflags: 
    ; 相当于MOV EFLAGS, EAX
    MOV     EAX, [ESP + 4]
    PUSH    EAX             ; 把EAX中存储的数据（此时是从内存中取回的标志）压入一个栈结构
    POPFD                   ; POP EFLAGS double-word，把上一句的栈结构中数据弹出，并代入EFLAGS
    RET


; 向指定内存中写入数据的函数，可用C语言中的指针代替
_write_mem8:                ; void write_mem8(int addr, int data):
    MOV     ECX, [ESP + 4]  ; 将内存地址ESP + 4的存储单元中存储的是欲写入的内地址，将其读入扩展计数寄存器ECX
    MOV     AL, [ESP + 8]   ; 内存地址ESP + 4的存储单元中存储的数据，读入AL寄存器
    MOV     [ECX], AL       ; 最后把AL中存储的欲写入的数据，写入ECX中存储的欲写入内存地址的内存单元中
    RET
