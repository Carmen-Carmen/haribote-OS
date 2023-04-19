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
    GLOBAL  _asm_inthandler20, _asm_inthandler21, _asm_inthandler27, _asm_inthandler2c
    GLOBAL  _asm_inthandler0d, _asm_inthandler0c
    GLOBAL  _memtest_sub
    GLOBAL  _load_tr, _farjmp
    GLOBAL  _farcall
    GLOBAL  _asm_cons_putchar
    GLOBAL  _asm_hrb_api
    GLOBAL  _start_app
    GLOBAL  _asm_end_app
    ; 来自外部的函数，需要被汇编调用
    EXTERN  _inthandler20, _inthandler21, _inthandler27, _inthandler2c
    EXTERN  _inthandler0d, _inthandler0c
    EXTERN  _cons_putchar 
    EXTERN  _hrb_api    

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
    MOV     AX, SS
    MOV     DS, AX
    MOV     ES, AX
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
    MOV     AX, SS
    MOV     DS, AX
    MOV     ES, AX
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
    MOV     AX, SS
    MOV     DS, AX
    MOV     ES, AX
    CALL    _inthandler2c
    POP     EAX
    POPAD
    POP     DS
    POP     ES
    IRETD

_asm_inthandler20: 
    PUSH    ES
    PUSH    DS
    PUSHAD
    MOV     EAX, ESP
    PUSH    EAX
    MOV     AX, SS
    MOV     DS, AX
    MOV     ES, AX
    CALL    _inthandler20
    POP     EAX
    POPAD
    POP     DS
    POP     ES
    IRETD

_asm_inthandler0d:              ; 强制结束应用程序的中断
    STI
    PUSH    ES
    PUSH    DS
    PUSHAD
    MOV     EAX, ESP
    PUSH    EAX
    MOV     AX, SS
    MOV     DS, AX
    MOV     ES, AX
    CALL    _inthandler0d
    CMP     EAX, 0
    JNE     _asm_end_app             ; 如果inthandler0d的返回值不是0而是OS栈地址esp0，应用程序结束
    POP     EAX
    POPAD
    POP     DS
    POP     ES
    ADD     ESP, 4
    IRETD

_asm_inthandler0c:              ; 处理栈异常的中断
    STI
    PUSH    ES
    PUSH    DS
    PUSHAD
    MOV     EAX, ESP
    PUSH    EAX
    MOV     AX, SS
    MOV     DS, AX
    MOV     ES, AX
    CALL    _inthandler0c
    CMP     EAX, 0
    JNE     _asm_end_app             ; 如果inthandler0d的返回值不是0而是OS栈地址esp0，应用程序结束
    POP     EAX
    POPAD
    POP     DS
    POP     ES
    ADD     ESP, 4
    IRETD

_memtest_sub:       ; unsigned int memtest_sub(unsigned int start, unsigned int end)
    PUSH    EDI                 ; 由于后面会用到EDI（目的变址）、ESI（源变址）和EBX（基址）
    PUSH    ESI
    PUSH    EBX
    MOV     ESI, 0xaa55aa55     ; pat0 = 0xaa55aa55;
    MOV     EDI, 0x55aa55aa     ; pat1 = 0x55aa55aa;
    MOV     EAX, [ESP + 12 + 4] ; i = start;
mts_loop:           ; memory test loop
    MOV     EBX, EAX
    ADD     EBX, 0x0ffc         ; p = i + 0x0ffc;
    MOV     EDX, [EBX]          ; old = *p;
    ; 向*p地址写入pat0，反转，检查反转结果
    MOV     [EBX], ESI          ; *p = pat0;
    XOR     DWORD [EBX], 0xffffffff ; *p ^= 0xffffffff;
    CMP     EDI, [EBX]          ; if (*p != pat0) goto mts_fin;
    JNE     mts_fin             ; Jump if Not Equal
    ; 再反转，检查反转结果
    XOR     DWORD [EBX], 0xffffffff ; *p ^= 0xffffffff;
    CMP     ESI, [EBX]          ; if (*p != pat0) goto mts_fin;
    JNE     mts_fin
    MOV     [EBX], EDX          ; *p = old;
    ADD     EAX, 0x1000         ; i += 0x1000;
    CMP     EAX, [ESP + 12 + 8] ; if (i <= end) goto mts_loop;
    JBE     mts_loop            
    
    POP     EBX
    POP     ESI
    POP     EDI
    RET                         ; return i; 汇编的返回值是EAX寄存器的值
mts_fin: 
    MOV     [EBX], EDX          ; *p = old;
    POP     EBX
    POP     ESI
    POP     EDI
    RET                         ; return i;
    
    
_load_tr:                       ; load task register, void load_tr(int tr)
    LTR     [ESP + 4]           ; 将[ESP + 4]内存地址的数据写入任务寄存器
    RET

; _taskswitch4:                   ; void taskswitch4()
;     JMP     4 * 8:0             ; 执行far模式JMP指令
;     RET
; 
; _taskswitch3: 
;     JMP     3 * 8:0
;     RET

_farjmp:                        ; void farjmp(int eip, int cs)
    JMP     FAR [ESP + 4]       ; 从右往左压栈, [ESP + 4]是eip, [ESP + 8]是cs
    RET

_farcall:                       ; void farcall(int eip, int cs)
    call    FAR [ESP + 4]       
    RET
    

; void cons_putchar(struct CONSOLE *cons, int chr, char move, int cc, int bc);
; _asm_cons_putchar: 
;     ; 在INT调用时CPU会自动执行CLI指令来禁止中断；但是这里只是代替call来使用，并不希望处理API时不能有其他中断，因此要在进入的时候开中断
;     STI                         
;     PUSHAD                      ; 之后是C函数调用，因此不能保证ECX寄存器的值不改变
; 
;     ; 从右往左pushcons_putchar的4个参数
;     PUSH    9                   ; bc: 暗红
;     PUSH    7                   ; cc: 白
;     PUSH    1                   ; move: 1
;     AND     EAX, 0xff           ; AL中存放了要打印的字符A，取出低2位
;     PUSH    EAX                 ; chr: 'A'
;     PUSH    DWORD [0x0fec]      ; 读取内存地址0x0fec的数据，这里存放了cons结构体的地址; cons: &cons
;     CALL    _cons_putchar       ; 调用C函数cons_putchar
;     ADD     ESP, 20             ; 栈顶下移5个，将之前PUSH进去的数据丢弃
; ;     RETF                        ; 由于使用far-CALL调用C函数，所以也要用far-RET
; 
;     POPAD
;     IRETD                       ; 由于改用中断的方式调用此API，就要用IRETD来返回

_asm_hrb_api: 
    STI
    PUSH    DS
    PUSH    ES
    PUSHAD                      ; 用于保存寄存器值
    PUSHAD                      ; 用于向hrb_api传参
    MOV     AX, SS
    MOV     DS, AX              ; 将OS用SS存入DS
    MOV     ES, AX              ; 将OS用SS存入ES
    CALL    _hrb_api            ; 调用hrb_api
    CMP     EAX, 0              ; 当EAX不为0时，程序结束
    JNE     _asm_end_app
    ADD     ESP, 32             ; 抵消向hrb_api传参时push了8个寄存器导致esp往下指32个地址
    POPAD                       ; 这时POP出来的才是最初PUSHAD保存的寄存器值
    POP     ES
    POP     DS
    IRETD
_asm_end_app: 
; eax为tss.esp0的地址
    MOV     ESP, [EAX]          ; 将esp0从内存中取出，赋给ESP寄存器
    MOV     DWORD [EAX + 4], 0  ; [ESP + 4]是ss寄存器，强制结束应用程序后tss.ss0应恢复成0
    POPAD
    RET                         ; 返回cmd_app

_start_app:                     ; void start_app(int eip, int cs, int esp, int ds, int *tss_esp0);
    ; pushad之前调用start_app时就已经压栈了4个参数，此时esp指向 -16
    PUSHAD
    ; 从栈中取出4个参数，pushad又压了8个，esp指向 -16 - 32，要取第一个参数eip，就要+36了
    MOV     EAX, [ESP + 36]     ; 应用程序用EIP
    MOV     ECX, [ESP + 40]     ; 应用程序用CS
    MOV     EDX, [ESP + 44]     ; 应用程序用ESP
    MOV     EBX, [ESP + 48]     ; 应用程序用DS/SS
    MOV     EBP, [ESP + 52]     ; tss.esp0的地址
    MOV     [EBP], ESP          ; 保存OS用ESP
    MOV     [EBP + 4], SS       ; 保存OS用SS，因为在TSS32结构体中ss0就是esp0后面的一个int
    ; 要运行C语言了，将段寄存器都解释为BX，即传入的应用程序的DS/SS
    MOV     ES, BX
    MOV     DS, BX
    MOV     FS, BX
    MOV     GS, BX
    ; 下面调整栈，从而能够使用RETF跳转到应用程序（x86规定不能通过far-CALL或far-JMP跳转到应用程序）
    OR      ECX, 3              ; 将应用程序用CS与3进行或运算
    OR      EBX, 3              ; 将应用程序用DS/SS与3进行或运算
    PUSH    EBX                 ; 压栈应用程序用DS/SS
    PUSH    EDX                 ; 压栈应用程序用ESP
    PUSH    ECX                 ; 压栈应用程序用CS
    PUSH    EAX                 ; 压栈应用程序用EIP
    RETF                        ; RETF本质是从栈中将地址POP出来，然后JMP到该地址进行执行，这样的话正是跳到应用程序的PC处开始执行
    ; 应用程序运行结束后也不会回到start_app中
