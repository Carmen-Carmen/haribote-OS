; haribote-os
; TAB=4
        ; haribote.asm的内容会写在0x004200之后的地方
        ; 启动区装载在内存0x8000号地址
        ; 因此要执行磁盘映像文件（img）上0x004200号地址的程序，就应该被装载到内存上0x8000 + 0x4200 = 0xc200号地址上！
        ORG     0xc200          ; 装载的内存地址

        MOV     AL, 0x13        ; VGA图形模式，是320*200*8位彩色，调色板模式
        MOV     AH, 0x00        ; 设置显卡模式
        INT     0x10            ; CPU中断，调用显卡BIOS

fin: 
        HLT
        JMP     fin
