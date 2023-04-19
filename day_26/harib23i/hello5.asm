[FORMAT "WCOFF"]    ; 制作成obj
[INSTRSET "i486p"]
[BITS 32]
[FILE "hello5.asm"]

    GLOBAL  _HariMain

[SECTION .text]     ; code segment

_HariMain: 
        ; api_putstr0(msg)
        MOV     EDX, 2
        MOV     EBX, msg
        INT     0x40

        ; api_end_app()
        MOV     EDX, 4
        INT     0x40

[SECTION .data]     ; data segment

msg: 
        DB      "HELLO5, WORLD! FROM ASSEMBLY."
        DB      0x0a    ; 换行
        DB      0
