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
putloop: 
    MOV     AL, [CS:ECX]
    CMP     AL, 0
    JE      fin
    INT     0x40
    ADD     ECX, 1
    JMP     putloop
fin: 
    RETF
msg: 
    DB      "hello!", 0
