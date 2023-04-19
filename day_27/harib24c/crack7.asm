[FORMAT "WCOFF"]
[INSTRSET "i486p"]
[BITS 32]
[FILE "crack7.asm"]

    GLOBAL  _HariMain

[SECTION .text]

_HariMain: 
    ; 读取段号1005的前4个字节，如果是"Hari"，则很大可能性在这个段中有应用程序在运行
    MOV     AX, 1005 * 8
    MOV     DS, AX
    CMP     DWORD [DS:0x0004], "Hari"   
    JNE     fin                         ; 不是应用程序，不执行任何操作

    MOV     ECX, [DS:0x0000]            ; 读取数据段大小，在hrb文件的0x0000
    MOV     AX, 2005 * 8
    MOV     DS, AX                      ; 将正在运行的应用程序的数据段存入DS

crackloop: 
    ; 向应用程序数据段中反复写入123（无实意）
    ADD     ECX, -1
    MOV     BYTE [DS:ECX], 123
    CMP     ECX, 0
    JNE     crackloop

fin:
    MOV     EDX, 4
    INT     0x40
