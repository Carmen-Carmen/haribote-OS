[INSTRSET "i486p"]
[BITS 32]
    CLI
fin: 
    HLT
    JMP     fin

    MOV     edx, 4
    INT     0x40
