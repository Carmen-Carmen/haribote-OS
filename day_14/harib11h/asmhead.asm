; haribote-os boot asm
; TAB=4
[INSTRSET "i486p"]              ; 要使用486的指令集（必须移动到前面来，为啥？）

BOTPAK  EQU     0x00280000      ; bootpack装载的位置
DSKCAC  EQU     0x00100000      ; 磁盘缓存地址
DSKCAC0 EQU     0x00008000      ; 磁盘缓存地址（real mode）
VBEMODE EQU     0x105           ; 1024 * 768 * 8bit color
;	0x100 :  640 x  400 x 8bitカラー
;	0x101 :  640 x  480 x 8bitカラー
;	0x103 :  800 x  600 x 8bitカラー
;	0x105 : 1024 x  768 x 8bitカラー
;	0x107 : 1280 x 1024 x 8bitカラー

; 有关BOOT_INFO的信息，记录在内存中哪些地址
CYLS    EQU     0x0ff0          ; IPL中最终读到了哪个柱面
LEDS    EQU     0x0ff1          ; 
VMODE   EQU     0x0ff2          ; 关于颜色数目的信息、颜色的位数等
SCRNX   EQU     0x0ff4          ; 长分辨率
SCRNY   EQU     0x0ff6          ; 宽分辨率
VRAM    EQU     0x0ff8          ; video RAM，图像缓冲区的开始地址

        ; haribote.asm的内容会写在0x004200之后的地方
        ; 启动区装载在内存0x8000号地址
        ; 因此要执行磁盘映像文件（img）上0x004200号地址的程序，就应该被装载到内存上0x8000 + 0x4200 = 0xc200号地址上！
        ORG     0xc200              ; 装载的内存地址

; 设置图形模式
        ; 确定VBE是否存在
;         MOV     AX, 0x9000
;         MOV     ES, AX
;         MOV     DI, 0           ; ES:DI开始的512字节会写入显卡能利用的VBE信息
;         MOV     AX, 0x4f00
;         INT     0x10
;         CMP     AX, 0x004f      ; 如果有VBE存在，AX会变为0x004f
;         JNE     scrn320         ; 否则就使用320 * 200画面
;         ; 检查VBE版本，只有2.0及以上版本才能使用高分辨率
;         MOV     AX, [ES:DI+4] ; 把写在ES:DI处内存中的VBE信息取到AX
;         CMP     AX, 0x0200
;         JB      scrn320         ; 如果取得的版本 < 0x0200，则还是只能使用320 * 200画面
;         ; 取得画面模式信息
;         MOV     CX, VBEMODE     ; VBEMODE = 0x105
;         MOV     AX, 0x4f01
;         INT     0x10
;         CMP     AX, 0x004f      ; 调用0x10中断后，如果AX是0x004f以外的值，就说明所指定的画面模式不可用
;         JNE     scrn320
;         ; 画面模式的信息确认
;         CMP     BYTE [ES:DI+0x19], 8  ; 颜色数必须为8
;         JNE     scrn320
;         CMP     BYTE [ES:DI+0x1b], 4  ; 颜色指定方法必须为4
;         JNE     scrn320
;         MOV     AX, [ES:DI+0x00]      ; 取出属性模式
;         AND     AX, 0x0080              ; 属性模式的bit7必须是1（不然不能加上0x4000再进行指定
;         JZ      scrn320
; 
;         ; 画面模式的切换
;         MOV     BX, VBEMODE + 0x4000    ; 指定属性模式
;         MOV     AX, 0x4f02              ; 设置显卡模式  
;         INT     0x10                    ; CPU中断，调用显卡BIOS
;         MOV     BYTE [VMODE], 8         ; 记录图像模式
;         MOV     AX, [ES:DI+0x12]        ; 取出画面长度
;         MOV     [SCRNX], AX             ; 记录画面长度
;         MOV     AX, [ES:DI+0x14]        ; 取出画面宽度
;         MOV     [SCRNY], AX             ; 记录画面宽度
;         MOV     EAX, [ES:DI+0x28]       ; 取出VRAM开始地址
;         MOV     [VRAM], EAX             ; 图像缓冲区开始地址
;         JMP     keystatus

scrn320: 
        MOV     AL, 0x13
        MOV     AH, 0x00
        INT     0x10
        MOV     BYTE [VMODE], 8
        MOV     WORD [SCRNX], 320
        MOV     WORD [SCRNY], 200
        MOV     DWORD [VRAM], 0x000a0000

; 将键盘LED灯的状态告知BIOS
keystatus:
        MOV     AH, 0x02
        INT     0x16                ; 16号CPU中断函数，调用键盘BIOS
        MOV     [LEDS], AL

; 以下部分的用处：调用C语言编写的程序

; PIC关闭一切中断
;   根据AT兼容机的规格，如果要初始化PIC，必须在CLI（禁止中断）之前进行，否则有时会挂起
;   随后进行PIC的初始化

        MOV     AL, 0xff
        OUT     0x21, AL            ; 把0xff传递给PIC0_IMR，即屏蔽PIC0的所有中断
        NOP                         ; 有些机种可能无法连续执行OUT指令，用No OPeration指令让CPU停止1个时钟周期
        OUT     0xa1, AL            ; 把0xff传递给PIC1_IMR

        CLI                         ; 禁止CPU级别的中断

; 为了让CPU能访问1MB以上的内存空间，需要设定A20GATE。A20GATE能解除为了兼容旧版16位操作系统默认限制只能使用1MB内存

        CALL    waitkbdout          ; 有点像C语言编写的wait_KBC_sendready()
        MOV     AL, 0xd1
        OUT     0x64, AL            ; 将0xd1传递给PORT_KEYCMD
        CALL    waitkbdout
        MOV     AL, 0xdf            
        OUT     0x60, AL            ; 将0xdf传递给PORT_KEYDAT，0xdf相当于将A20GATE信号线变成ON的状态
        CALL    waitkbdout          ; 等待A20GATE的处理切实完成

; 切换到保护模式

        LGDT    [GDTR0]             ; 设定临时的GDT（后面还要在C语言中详细设定）
        MOV     EAX, CR0            ; CR0（control register 0）是特殊的32位寄存器，将其值代入同为32位的EAX寄存器，并作以下设置
        AND     EAX, 0x7fffffff     ; 与运算，设第31位（bit31）为0（不用分页）paging enable, 0 - disable
        OR      EAX, 0x00000001     ; 或运算，设bit1为1（为了切换到保护模式）PE, protect enable
        MOV     CR0, EAX            ; 把设定好的值还给CR0，就完成了 禁止分页的保护模式 的设定
        JMP     pipelineflush       ; 变成保护模式后，机器语言的解释会发生变化，CPU为了加快指令的执行速度使用了管道pipeline机制（流水？）

pipelineflush: ; 进入流水模式，要对段寄存器的意思进行重新解释（不再是16位中乘16后再加算的意思）
        MOV     AX, 1 * 8
        ; 除了CS以外所有段寄存器的值都变成0x0008，相当于"gdt + 1"的段
        MOV     DS, AX
        MOV     ES, AX
        MOV     FS, AX
        MOV     GS, AX
        MOV     SS, AX
        
; bootpack的转送
        
        MOV     ESI, bootpack       ; 转送源，是bootpack.hrb
        MOV     EDI, BOTPAK         ; 转送目的地，是bootpack应当装载的位置0x00280000
        MOV     ECX, 512 * 1024 / 4 ; 转送数据大小，512字节比bootpack.hrb大很多，是预留的；以DW为1个单位，所以是字节数 / 4
        CALL    memcpy              ; 复制内存数据的函数

; 磁盘数据最终转送到它本来的位置
; 先从启动扇区IPL开始转送
        MOV     ESI, 0x7c00         ; 转送源，IPL是ORG 0x7c00，即从0x7c00开始
        MOV     EDI, DSKCAC         ; 转送目的地，磁盘缓存地址
        MOV     ECX, 512 / 4
        CALL    memcpy
; 转送剩下的内容
        MOV     ESI, DSKCAC0 + 512  ; 转送源，real mode下写入的磁盘缓存地址，在IPL后的（+512）数据
        MOV     EDI, DSKCAC + 512   ; 转送目的地，保护模式下的磁盘缓存地址，写在IPL之后
        MOV     ECX, 0              ; 先把ECX寄存器置0
        MOV     CL, BYTE [CYLS]     ; 读取0x0ff0号内存地址中存储的柱面数，放进ECX寄存器的低4位（CL）
        IMUL    ECX, 512 * 18 * 2 / 4   ; integer multiple，对ECX中数据进行乘法运算
        ; 即要转送的数据单元数 = 柱面数 * 磁头数 * 扇区数（低4位） * 扇区字节数（512） / 4
        SUB     ECX, 512 / 4        ; 减去之前已经转送完毕的IPL（512字节）
        CALL    memcpy

; 必须由asmhead来完成的工作至此已经全部做完，后续交给C语言编写的bootpack完成
; bootpack的启动

        MOV     EBX, BOTPAK
        MOV     ECX, [EBX + 16]     ; bootpack.hrb之后的第16号地址，值为0x11a8
        ADD     ECX, 3              ; ECX += 3
        SHR     ECX, 2              ; 逻辑右移（SHift Right），相当于 ECX /= 4
        JZ      skip                ; Jump if Zero，即ECX没有内容了的时候
        MOV     ESI, [EBX + 20]     ; memcpy转送源，bootpack.hrb之后的第20号地址，值为0x10c8
        ADD     ESI, EBX
        MOV     EDI, [EBX + 12]     ; memcpy转送目的地，bootpack.hrb之后的第12号地址，值为0x00310000
        CALL    memcpy              ; 实际上是将bootpack.hrb第0x10c8字节开始的0x11a8(4520)字节复制到0x00310000号内存地址去

skip: ; 注意这个skip标签就算没有在JZ那边跳转，也会顺序执行下来
        MOV     ESP, [EBX + 12]
        JMP     DWORD 2 * 8: 0x0000001b ; 将2*8代入到CS中，同时移动到0x1b地址，即第2个段的0x1b号地址
        ; 第2个段的基址是0x00280000，所以实际上是从0x0028001b开始执行，这也是bootpack.hrb的0x1b号地址

; 之前用到的函数
waitkbdout: 
        IN      AL, 0x64            
        AND     AL, 0x02            ; 与运算，把0x64号设备(PORT_KEY_STA)传入CPU的数据与0x02(KEYSTA_SEND_NOTREADY)做比较
        IN      AL, 0x60            ; 从0x60号设备中空读一次，晴空数据接收缓冲区中的垃圾数据
        JNZ     waitkbdout          ; Jump if Not Zero，AND AL, 0x02的执行结果如果不是0，那么键盘就还没准备好；否则就return
        RET                         ; 有return，就是跳回原来执行到的位置

memcpy: ; 复制内存数据的函数，ESI是复制起始地址，EDI是目的地址，ECI是要复制数据的大小
        MOV     EAX, [ESI]          ; 把ESI代表的内存地址数据赋给EAX，省略了数据类型DWORD，实际上转移了[ESI] ~ [ESI + 3]的4Byte数据
        ADD     ESI, 4              ; 源地址ESI += 4，一次传输DWORD（4Byte）
        MOV     [EDI], EAX          ; 把取出数据写入内存地址EDI
        ADD     EDI, 4              ; 目的地址EDI += 4
        SUB     ECX, 1              ; 复制数据大小ECX--
        JNZ     memcpy              ; while(ECX != 0) 都执行memcpy
        RET

        ALIGNB  16                  ; 用于字节对其，即一只添加DB 0，直到当前地址为16的整数
GDT0: ; GDT0是一种特定的GDT，不能够定义段，是 空区域
        RESB    8                   ; NULL sector
        DW      0xffff, 0x0000, 0x9200, 0x00cf  ; 可以读写的段 32bit
        DW      0xffff, 0x0000, 0x9a28, 0x0047  ; 可以执行的段 32bit

        DW      0
GDTR0: 
        DW      8 * 3 - 1
        DD      GDT0                ; Data Dword GDT0，向内存中写入GDT0(4 Byte)

        ALIGNB  16
bootpack:
