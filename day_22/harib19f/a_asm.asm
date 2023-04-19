[FORMAT "WCOFF"]    ; 生成文件的模式为"对象文件模式"，这样才能与a.c的编译结果进行连接
[INSTRSET "i486p"]   ; 使用486兼容指令集
[BITS 32]           ; 生成32位模式的机器语言
[FILE "a_asm.asm"]  ; 源文件名信息

    GLOBAL  _api_openwin, _api_putchar, _api_putstr0, _api_end

[SECTION .text]

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
