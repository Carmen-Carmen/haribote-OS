[INSTRSET "i486p"]
[BITS 32]
    CALL    2 * 8:0x0cac        ; 0xcac是io_cli在bootpack.map中映射的地址
    MOV     EDX, 4
    INT     0x40    
