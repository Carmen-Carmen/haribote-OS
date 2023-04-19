; haribote-os
; TAB=4

; 有关BOOT_INFO的信息，记录在内存中哪些地址
CYLS    EQU     0x0ff0          ; 设定启动区
LEDS    EQU     0x0ff1          ; 
VMODE   EQU     0x0ff2          ; 关于颜色数目的信息、颜色的位数等
SCRNX   EQU     0x0ff4          ; 长分辨率
SCRNY   EQU     0x0ff6          ; 宽分辨率
VRAM    EQU     0x0ff8          ; video RAM，图像缓冲区的开始地址

        ; haribote.asm的内容会写在0x004200之后的地方
        ; 启动区装载在内存0x8000号地址
        ; 因此要执行磁盘映像文件（img）上0x004200号地址的程序，就应该被装载到内存上0x8000 + 0x4200 = 0xc200号地址上！
        ORG     0xc200              ; 装载的内存地址

        MOV     AL, 0x13            ; VGA图形模式，是320*200*8位彩色，调色板模式
        MOV     AH, 0x00            ; 设置显卡模式
        INT     0x10                ; CPU中断，调用显卡BIOS
        MOV     BYTE [VMODE], 8     ; 记录图像模式
        MOV     WORD [SCRNX], 320   ; 画面长度
        MOV     WORD [SCRNY], 200   ; 画面宽度
        MOV     DWORD [VRAM], 0x000a0000    ; 图像缓冲区开始地址

; 将键盘LED灯的状态告知BIOS
        MOV     AH, 0x02
        INT     0x16                ; 16号CPU中断函数，调用键盘BIOS
        MOV     [LEDS], AL

fin: 
        HLT
        JMP     fin
