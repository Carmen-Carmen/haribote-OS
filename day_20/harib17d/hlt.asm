[BITS 32]
    
    MOV     AL, 'h'
    CALL    2*8:0xDDF
    MOV     AL, 'e'
    CALL    2*8:0xDDF
    MOV     AL, 'l'
    CALL    2*8:0xDDF
    MOV     AL, 'l'
    CALL    2*8:0xDDF
    MOV     AL, 'o'
    CALL    2*8:0xDDF
    MOV     AL, '!'
    CALL    2*8:0xDDF
    RETF                ; far-RET

