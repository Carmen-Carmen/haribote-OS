[FORMAT "WCOFF"]
[INSTRSET "i486p"]
[BITS 32]
[FILE "api022.asm"]

    GLOBAL  _api_fseek

[SECTION .text]

_api_fseek:	    ; void api_fseek(int fhandle, int offset, int mode); 调整文件句柄中pos以实现定位
    PUSH    EBX
    MOV	    EDX, 23
    MOV	    EAX, [ESP + 8]	; fhandle
    MOV	    ECX, [ESP + 16]	; mode
    MOV	    EBX, [ESP + 12]	; offset
    INT	    0x40
    POP	    EBX
    RET

