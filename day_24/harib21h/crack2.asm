[INSTRSET "i486p"]
[BITS 32]
    MOV     EAX, 1 * 8              ; OS栈的段号
    MOV     DS, AX                  ; 将其存入DS中，相当于就获得了操作系统栈的操作权限
    MOV     BYTE [0x00102600], 0
    MOV     EDX, 4
    INT     0x40
