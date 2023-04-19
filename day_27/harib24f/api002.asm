[FORMAT "WCOFF"]
[INSTRSET "i486p"]
[BITS 32]
[FILE "api002.asm"]

    GLOBAL  _api_putstr0

[SECTION .text]

_api_putstr0:       ; void api_putstr0(char *s);
    PUSH    EBX                 ; 用EBX存储指针位置，因此先保存原来的EBX
    MOV     EDX, 2
    MOV     EBX, [ESP + 8]      ; char *指针压栈（4 Byte），因为之前压栈了EBX，所以用ESP + 8来获取参数s
    INT     0x40
    POP     EBX                 ; 恢复EBX
    RET

