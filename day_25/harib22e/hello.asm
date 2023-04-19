[INSTRSET "i486p"]
[BITS 32]
    
    ; MOV     AL, 'h'
    ; INT     0x40
    ; MOV     AL, 'e'
    ; INT     0x40
    ; MOV     AL, 'l'
    ; INT     0x40
    ; MOV     AL, 'l'
    ; INT     0x40
    ; MOV     AL, 'o'
    ; INT     0x40
    ; MOV     AL, '!'
    ; INT     0x40
    ; RETF                ; far-RET

    
    MOV     ECX, msg
    MOV     EDX, 1      ; INT0x40时，EDX == 1用于区分是要调用cons_putchar
putloop: 
    MOV     AL, [CS:ECX]
    CMP     AL, 0
    JE      fin
    INT     0x40
    ADD     ECX, 1
    JMP     putloop
fin: 
    MOV     EDX, 4
    INT     0x40
    ; RETF              ; 不再能通过RETF返回，需要通过INT 0x40且edx = 4来结束应用程序
msg: 
    DB      "hello!", 0
