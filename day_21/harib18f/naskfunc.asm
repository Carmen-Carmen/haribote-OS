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
    GLOBAL  _asm_inthandler20, _asm_inthandler21, _asm_inthandler27, _asm_inthandler2c, _asm_inthandler0d
    GLOBAL  _memtest_sub
    GLOBAL  _load_tr, _farjmp
    GLOBAL  _farcall
    GLOBAL  _asm_cons_putchar
    GLOBAL  _asm_hrb_api
    GLOBAL  _start_app
    ; 来自外部的函数，需要被汇编调用
    EXTERN  _inthandler20, _inthandler21, _inthandler27, _inthandler2c, _inthandler0d
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
    MOV     AX, SS
    CMP     AX, 1 * 8   ; 根据此时段栈寄存器的值来判断是OS活动产生中断还是应用程序活动产生中断
    JNE     .from_app
; 操作系统活动产生中断时
    MOV     EAX, ESP
    PUSH    SS          ; 保存中断产生时的SS
    PUSH    EAX         ; 保存中断产生时的ESP
    MOV     AX, SS
    MOV     DS, AX
    MOV     ES, AX
    CALL    _inthandler21   ; 中断处理
    ADD     ESP, 8
    POPAD
    POP     DS
    POP     ES
    IRETD

.from_app:
;   当应用程序活动时发生中断
    MOV     EAX, 1 * 8
    MOV     DS, AX          ; 先仅将DS设定为OS用，因为只有OS能处理中断！
    MOV     ECX, [0xfe4]    ; 操作系统的ESP
    ADD     ECX, -8
    MOV     [ECX + 4], SS   ; 保存中断产生时的SS
    MOV     [ECX], ESP      ; 保存中断产生时的ESP
    MOV     SS, AX
    MOV     ES, AX
    MOV     ESP, ECX
    CALL    _inthandler21   ; 中断处理
    POP     ECX
    POP     EAX
    MOV     SS, AX          ; 将SS设回应用程序用
    MOV     ESP, ECX        ; 将ESP设回应用程序用
    POPAD
    POP     DS
    POP     ES
    IRETD

_asm_inthandler27: 
    PUSH    ES
    PUSH    DS
    PUSHAD
    MOV     AX, SS
    CMP     AX, 1 * 8   ; 根据此时段栈寄存器的值来判断是OS活动产生中断还是应用程序活动产生中断
    JNE     .from_app
; 操作系统活动产生中断时
    MOV     EAX, ESP
    PUSH    SS          ; 保存中断产生时的SS
    PUSH    EAX         ; 保存中断产生时的ESP
    MOV     AX, SS
    MOV     DS, AX
    MOV     ES, AX
    CALL    _inthandler27   ; 中断处理
    ADD     ESP, 8
    POPAD
    POP     DS
    POP     ES
    IRETD

.from_app:
;   当应用程序活动时发生中断
    MOV     EAX, 1 * 8
    MOV     DS, AX          ; 先仅将DS设定为OS用，因为只有OS能处理中断！
    MOV     ECX, [0xfe4]    ; 操作系统的ESP
    ADD     ECX, -8
    MOV     [ECX + 4], SS   ; 保存中断产生时的SS
    MOV     [ECX], ESP      ; 保存中断产生时的ESP
    MOV     SS, AX
    MOV     ES, AX
    MOV     ESP, ECX
    CALL    _inthandler27   ; 中断处理
    POP     ECX
    POP     EAX
    MOV     SS, AX          ; 将SS设回应用程序用
    MOV     ESP, ECX        ; 将ESP设回应用程序用
    POPAD
    POP     DS
    POP     ES
    IRETD

_asm_inthandler2c: 
    PUSH    ES
    PUSH    DS
    PUSHAD
    MOV     AX, SS
    CMP     AX, 1 * 8   ; 根据此时段栈寄存器的值来判断是OS活动产生中断还是应用程序活动产生中断
    JNE     .from_app
; 操作系统活动产生中断时
    MOV     EAX, ESP
    PUSH    SS          ; 保存中断产生时的SS
    PUSH    EAX         ; 保存中断产生时的ESP
    MOV     AX, SS
    MOV     DS, AX
    MOV     ES, AX
    CALL    _inthandler2c   ; 中断处理
    ADD     ESP, 8
    POPAD
    POP     DS
    POP     ES
    IRETD

.from_app:
;   当应用程序活动时发生中断
    MOV     EAX, 1 * 8
    MOV     DS, AX          ; 先仅将DS设定为OS用，因为只有OS能处理中断！
    MOV     ECX, [0xfe4]    ; 操作系统的ESP
    ADD     ECX, -8
    MOV     [ECX + 4], SS   ; 保存中断产生时的SS
    MOV     [ECX], ESP      ; 保存中断产生时的ESP
    MOV     SS, AX
    MOV     ES, AX
    MOV     ESP, ECX
    CALL    _inthandler2c   ; 中断处理
    POP     ECX
    POP     EAX
    MOV     SS, AX          ; 将SS设回应用程序用
    MOV     ESP, ECX        ; 将ESP设回应用程序用
    POPAD
    POP     DS
    POP     ES
    IRETD

_asm_inthandler20: 
    PUSH    ES
    PUSH    DS
    PUSHAD
    MOV     AX, SS
    CMP     AX, 1 * 8   ; 根据此时段栈寄存器的值来判断是OS活动产生中断还是应用程序活动产生中断
    JNE     .from_app
; 操作系统活动产生中断时
    MOV     EAX, ESP
    PUSH    SS          ; 保存中断产生时的SS
    PUSH    EAX         ; 保存中断产生时的ESP
    MOV     AX, SS
    MOV     DS, AX
    MOV     ES, AX
    CALL    _inthandler20   ; 中断处理
    ADD     ESP, 8
    POPAD
    POP     DS
    POP     ES
    IRETD

.from_app:
;   当应用程序活动时发生中断
    MOV     EAX, 1 * 8
    MOV     DS, AX          ; 先仅将DS设定为OS用，因为只有OS能处理中断！
    MOV     ECX, [0xfe4]    ; 操作系统的ESP
    ADD     ECX, -8
    MOV     [ECX + 4], SS   ; 保存中断产生时的SS
    MOV     [ECX], ESP      ; 保存中断产生时的ESP
    MOV     SS, AX
    MOV     ES, AX
    MOV     ESP, ECX
    CALL    _inthandler20   ; 中断处理
    POP     ECX
    POP     EAX
    MOV     SS, AX          ; 将SS设回应用程序用
    MOV     ESP, ECX        ; 将ESP设回应用程序用
    POPAD
    POP     DS
    POP     ES
    IRETD

_asm_inthandler0d:
    STI
    PUSH    ES
    PUSH    DS
    PUSHAD
    MOV     AX, SS
    CMP     AX, 1 * 8   ; 根据此时段栈寄存器的值来判断是OS活动产生中断还是应用程序活动产生中断
    JNE     .from_app
; 操作系统活动产生中断时
    MOV     EAX, ESP
    PUSH    SS          ; 保存中断产生时的SS
    PUSH    EAX         ; 保存中断产生时的ESP
    MOV     AX, SS
    MOV     DS, AX
    MOV     ES, AX
    CALL    _inthandler0d   ; 中断处理
    ADD     ESP, 8
    POPAD
    POP     DS
    POP     ES
    ADD     ESP, 4
    IRETD

.from_app:
;   当应用程序活动时发生中断
    MOV     EAX, 1 * 8
    MOV     DS, AX          ; 先仅将DS设定为OS用，因为只有OS能处理中断！
    MOV     ECX, [0xfe4]    ; 操作系统的ESP
    ADD     ECX, -8
    MOV     [ECX + 4], SS   ; 保存中断产生时的SS
    MOV     [ECX], ESP      ; 保存中断产生时的ESP
    MOV     SS, AX
    MOV     ES, AX
    MOV     ESP, ECX
    STI
    CALL    _inthandler0d   ; 中断处理
    CLI
    CMP     EAX, 0
    JNE     .kill
    POP     ECX
    POP     EAX
    MOV     SS, AX          ; 将SS设回应用程序用
    MOV     ESP, ECX        ; 将ESP设回应用程序用
    POPAD
    POP     DS
    POP     ES
    ADD     ESP, 4
    IRETD

.kill: 
; 强制将应用程序结束，其实是返回到cmd_app中
    MOV     EAX, 1 * 8      ; OS用的DS/SS
    MOV     ES, AX
    MOV     SS, AX
    MOV     DS, AX
    MOV     FS, AX
    MOV     GS, AX
    MOV     ESP, [0xfe4]    ; 强制返回到start_app时的esp
    STI
    POPAD
    RET

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
    ; 方便起见，在开头就关中断
    PUSH    DS
    PUSH    ES
    PUSHAD                      ; 这边是把寄存器值push进了应用程序的栈，使用操作系统栈的hrb_api读不到
    MOV     EAX, 1 * 8
    MOV     DS, AX              ; 先仅将DS设定为OS用
    MOV     ECX, [0xfe4]        ; 取出操作系统ESP地址，先放在ECX中
    ADD     ECX, -40
    MOV     [ECX + 32], ESP     ; 保存应用程序的ESP
    MOV     [ECX + 36], SS      ; 保存应用程序的SS

; 将PUSHAD后的值复制并压入OS栈 (其地址存在内存地址0xfe4，刚才保存到ECX中了)，供hrb_api来使用
    MOV     EDX, [ESP]
    MOV     EBX, [ESP + 4]
    MOV     [ECX], EDX          ; 复制传递给hrb_api (ECX中存放了原本存在内存地址0xfe4中的OS栈的地址)
    MOV     [ECX + 4], EBX      ; 复制传递给hrb_api 
    MOV     EDX, [ESP + 8]
    MOV     EBX, [ESP + 12]
    MOV     [ECX + 8], EDX      ; 复制传递给hrb_api
    MOV     [ECX + 12], EBX     ; 复制传递给hrb_api
    MOV     EDX, [ESP + 16]
    MOV     EBX, [ESP + 20]
    MOV     [ECX + 16], EDX     ; 复制传递给hrb_api
    MOV     [ECX + 20], EBX     ; 复制传递给hrb_api
    MOV     EDX, [ESP + 24]
    MOV     EBX, [ESP + 28]
    MOV     [ECX + 24], EDX     ; 复制传递给hrb_api
    MOV     [ECX + 28], EBX     ; 复制传递给hrb_api

    MOV     ES, AX              ; 将剩余的段寄存器也设为OS用
    MOV     SS, AX
    MOV     ESP, ECX
    STI                         ; 开中断

    CALL    _hrb_api            ; hrb_api是运行在OS栈

    MOV     ECX, [ESP + 32]     ; 取出应用程序的ESP
    MOV     EAX, [ESP + 36]     ; 取出应用程序的SS
    CLI
    MOV     SS, AX
    MOV     ESP, ECX
    POPAD
    POP     ES
    POP     DS
    IRETD                       ; IRETD指令会自动执行STI

_start_app:                     ; void start_app(int eip, int cs, int esp, int ds);
    ; pushad之前调用start_app时就已经压栈了4个参数，此时esp指向 -16
    PUSHAD
    ; 从栈中取出4个参数，pushad又压了8个，esp指向 -16 - 32，要取第一个参数eip，就要+36了
    MOV     EAX, [ESP + 36]     ; 应用程序用EIP
    MOV     ECX, [ESP + 40]     ; 应用程序用CS
    MOV     EDX, [ESP + 44]     ; 应用程序用ESP
    MOV     EBX, [ESP + 48]     ; 应用程序用DS/SS
    MOV     [0xfe4], ESP        ; OS用ESP
    CLI                         ; 切换过程中禁止中断请求
    ; 要运行C语言了，将段寄存器都解释为BX，即传入的应用程序的DS/SS
    MOV     ES, BX
    MOV     SS, BX
    MOV     DS, BX
    MOV     FS, BX
    MOV     GS, BX
    MOV     ESP, EDX            ; 将ESP设为应用程序的栈
    STI                         ; 切换完成，开中断
    PUSH    ECX                 ; 用于far-CALL的参数cs
    PUSH    EAX                 ; 用于far-CALL的参数eip
    CALL    FAR [ESP]           ; 刚刚push进去的cs和eip，从右往左压栈作为FAR的参数，从而切换至应用程序(的代码)运行，其中包含INT 0x40中断，这个中断被注册给了asm_hrb_api

; 应用程序结束(RET)后回到此处，要返回OS态执行了
    MOV     EAX, 1 * 8          ; OS用DS/SS
    CLI                         ; 再次切换，禁止中断请求
    ; 将段寄存器都解释为1 * 8，即操作系统数据段
    MOV     ES, AX
    MOV     SS, AX
    MOV     DS, AX
    MOV     FS, AX
    MOV     GS, AX
    MOV     ESP, [0xfe4]        ; OS栈保存在0xfe4，将ESP设为OS的栈
    STI                         ; 切换完成，开中断
    POPAD
    RET

