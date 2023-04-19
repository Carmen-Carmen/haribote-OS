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
    GLOBAL  _load_gdtr, _load_idtr 
    GLOBAL  _load_cr0, _store_cr0
    GLOBAL  _asm_inthandler21, _asm_inthandler27, _asm_inthandler2c
    EXTERN  _inthandler21, _inthandler27, _inthandler2c             ; 来自外部的函数，需要被汇编调用

; 以下是实际的函数

[SECTION .text]         ; 目标文件中写了这些之后再写程序

_io_hlt:                ; 即C语言中的void io_hlt(void);
    HLT
    RET                 ; 相当于C语言中的return，表示这个函数结束了

_io_cli: 
    CLI                 ; CLear Interrupt
    RET

_io_sti:
    STI                 ; SeT Interrupt
    RET

_io_stihlt:
    STI
    HLT
    RET

_io_in8: 
    MOV     EDX, [ESP + 4]  ; port
    MOV     EAX, 0          ; 把EAX寄存器置为0，用于接收
    IN      AL, DX          ; EDX中数据表示的端口的io设备的数据读入EAX
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

; 将内存中数据写入GDTR寄存器
_load_gdtr:                 ; void load_gtdr(int limit, int addr)
    MOV     AX, [ESP + 4]   ; limit
    MOV     [ESP + 6], AX
    LGDT    [ESP + 6]
    RET

; 将内存中数据写入IDTR寄存器
_load_idtr:                 ; void load_idtr(int limit, int addr)
    MOV     AX, [ESP + 4]   ; limit
    MOV     [ESP + 6], AX
    LIDT    [ESP + 6]
    RET

; 读取控制寄存器0 cr0并返回
_load_cr0:                  ; int load_cr0(void)
    MOV     EAX, CR0
    RET

; 接收cr0信息并存入cr0寄存器
_store_cr0:                 ; void store_cr0(int cr0)
    MOV     EAX, [ESP + 4]
    MOV     CR0, EAX
    RET

; 用汇编编写的中断处理
; 汇编的重点在于程序执行现场的保护和恢复（通过入栈出栈实现）
_asm_inthandler21:
    PUSH    ES
    PUSH    DS
    PUSHAD
    MOV     EAX, ESP
    PUSH    EAX
    CALL    _set_segment_registers
    CALL    _inthandler21
    POP     EAX
    POPAD
    POP     DS
    POP     ES
    IRETD

_asm_inthandler27: 
    PUSH    ES
    PUSH    DS
    PUSHAD
    MOV     EAX, ESP
    PUSH    EAX
    CALL    _set_segment_registers
    CALL    _inthandler27
    POP     EAX
    POPAD
    POP     DS
    POP     ES
    IRETD

_asm_inthandler2c: 
    PUSH    ES
    PUSH    DS
    PUSHAD
    MOV     EAX, ESP
    PUSH    EAX
    CALL    _set_segment_registers
    CALL    _inthandler2c
    POP     EAX
    POPAD
    POP     DS
    POP     ES
    IRETD

; 函数式写法来写中断处理
; 好像出入栈操作不能这么写，可能是因为每个中断处理函数都应该有自己的栈
_reserve_status: 
    PUSH    ES
    PUSH    DS
    PUSHAD
    MOV     EAX, ESP
    PUSH    EAX
    RET

; 设定段寄存器值这个应该是可以单独拿出来的（确实可以，笑
_set_segment_registers: 
    MOV     AX, SS
    MOV     DS, AX
    MOV     ES, AX
    RET

_restore_status: 
    POP     EAX
    POPAD
    POP     DS
    POP     ES
    RET
