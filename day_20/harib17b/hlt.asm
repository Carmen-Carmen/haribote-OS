[BITS 32]
    
    MOV     AL, 'A'
    CALL    2*8:0xDB7

fin: 
    HLT
    JMP     fin
