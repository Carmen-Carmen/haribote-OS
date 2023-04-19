; haribote-ipl
; TAB=4

        ORG     0x7c00              ; origin，指明程序的装载地址（从哪个内存单元开始），规定的是就是从0x00007c00 - 0x00007dff

; 以下的记述用于标准FAT12格式的软盘

        JMP     entry
        DB      0x90
        DB      "HARIBOTE"          ; 启动区名称
        DW      512                 ; 扇区大小
        DB      1                   ; 簇大小
        DW      1                   ; FAT起始位置
        DB      2                   ; FAT个数
        DW      224                 ; 根目录大小
        DW      2880                ; 该磁盘的大小（2880个扇区）
        DB      0xf0                ; 磁盘种类
        DW      9                   ; FAT长度
        DW      18                  ; 1个磁道的扇区数
        DW      2                   ; 磁头数
        DD      0                   ; 不使用分区，即0
        DD      2880                ; 重写一次磁盘大小
        DB      0, 0, 0x29          ; 意义不明，固定写法
        DD      0xffffffff          ; （可能是）卷标号码
        DB      "HARIBOTE-OS"       ; 11个字节的磁盘名称
        DB      "FAT12   "          ; 8个字节的磁盘格式名称
        RESB    18

; 程序主体
; 冒号表示声明标签，这些标签可以用于指定JMP指令的跳转目的地
entry: ; 程序入口
        MOV     AX, 0               ; 将寄存器初始化，MOV即赋值
        MOV     SS, AX              ; 累加寄存器AX的值赋给栈段寄存器SS
        MOV     SP, 0x7c00          ; 程序装载地址赋给栈指针寄存器SP
        MOV     DS, AX              ; AX的值赋给数据段寄存器DS


; 读磁盘
        MOV     AX, 0x0820
        MOV     ES, AX              ; 先指定扩展段寄存器ES的值为0x0820
        MOV     CH, 0               ; 设置柱面cylinder为0号柱面
        MOV     DH, 0               ; 设置磁头head为0，即软盘的正面
        MOV     CL, 2               ; 设置扇区sector为第2个扇区

        MOV     SI, 0               ; 用源变址寄存器SI记录读取失败次数，限制只能5次失败
retry:
        MOV     AH, 0x02            ; AH=0x02：读盘；0x03则是写盘
        MOV     AL, 1               ; 读1个扇区，AL存放读取扇区数（只能同时处理连续的扇区）
        MOV     BX, 0               ; 缓冲区地址 ES:BX，用ES寄存器指定一个大概的地址，再用BX指定其中一个具体地址
        MOV     DL, 0x00            ; 0号驱动器，DL存放驱动器号，这个操作系统只有1个软盘驱动器
        ; 因此缓冲区地址ES:BX为 0x0820:0x0000，软盘的一个扇区的数据将被装载到内存中0x8200~0x81ff（512字节）
        INT     0x13                ; CPU中断，调用磁盘BIOS
        ; 调用磁盘BIOS的中断函数0x13，返回值如下
        ; FLACS.CF==0: 没有错误，AH==0
        ; FLACS.CF==1: 有错误，错误号码存入AH内
        JNC     fin                 ; Jump if Not Carry，如果没有错误，就跳转到fin部分
        ; 到这边就是有错误了
        ADD     SI, 1               ; SI存放了失败次数，加1
        CMP     SI, 5               ; 只能失败5次
        JAE     error               ; Jump if Above or Equal，如果上一条CMP结果相等或者更大就跳转，即失败次数大于5次就跳转到error标签
        MOV     AH, 0x00            ; 重置AH寄存器（硬盘操作方式）
        MOV     DL, 0x00            ; 重置DL寄存器（驱动器）
        ; AH=0x00, DL=0x00时INT 0x13是“系统复位”功能，能够将软盘状态复位
        INT     0x13                ; 调用磁盘BIOS，重置驱动器
        JMP     retry               ; 跳转回retry标签开头

; 读取完成后先不做任何事情，让CPU休眠

fin:
        HLT                         ; 让CPU停止，等待指令
        JMP     fin                 ; 跳回fin标签，即无限循环CPU停止

; 读盘错误的处理
; 比如更改磁头为2就能触发错误，因为只有0号和1号磁头的在（正反面）
error: 
        MOV     SI, msg             ; 把msg的内存地址放到源变址寄存器，用于显示错误信息 

putloop: 
        MOV     AL, [SI]            ; 源变址寄存器SI中存放的内存地址，取出其对应内存单元中数据，赋给累加寄存器低位AL
        ADD     SI, 1               ; SI值+1，感觉就是把msg标签中的汇编指令一条一条向下执行，直到执行到写了“0”的内存单元
        CMP     AL, 0               ; 将AL的值与0进行比较，AL中的值是SI中数据对应内存地址中存放的数据
        JE      fin                 ; Jump if Equal，即上一条CMP的结果为真，则跳转到fin标签；否则继续执行
        ; 到这里就证明AL中的值不是0
        MOV     AH, 0x0e            ; 显示一个文字
        MOV     BX, 15              ; 指定字符颜色，BH: 0000，BL: 1111
        INT     0x10                ; CPU中断，调用显卡BIOS（basic input/output system）
        JMP     putloop             ; 跳转到循环的开头


msg: ; 信息显示
        DB      0x0a, 0x0a          ; 换行2次
        DB      "load error"     ; 利用Data Byte指令，把字符串中每一个字符对应的代码一个一个写入内存中
        DB      0x0a                ; 换行1次
        DB      0                   

        RESB    0x7dfe - $          ; 从当前字节开始，填写0x00，直到地址0x001fe

        DB      0x55, 0xaa          ; 启动区最后2个字节的内容应是0x55和0xAA

