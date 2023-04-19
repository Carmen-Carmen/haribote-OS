; hello-os
; TAB=4

        ORG     0x7c00              ; origin，指明程序的装载地址（从哪个内存单元开始），规定的是就是从0x00007c00 - 0x00007dff

; 以下的记述用于标准FAT12格式的软盘

        JMP     entry
        DB      0x90
        DB      "HELLOIPL"          ; 启动区名称
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
        DB      "HELLO-OS   "       ; 11个字节的磁盘名称
        DB      "FAT12   "          ; 8个字节的磁盘格式名称
        RESB    18

; 程序主体
; 冒号表示声明标签，这些标签可以用于指定JMP指令的跳转目的地
entry: ; 程序入口
        MOV     AX, 0               ; 将寄存器初始化，MOV即赋值
        MOV     SS, AX              ; 累加寄存器AX的值赋给栈段寄存器SS
        MOV     SP, 0x7c00          ; 程序装载地址赋给栈指针寄存器SP
        MOV     DS, AX              ; AX的值赋给数据段寄存器DS
        MOV     ES, AX              ; AX的值赋给附加段寄存器ES

        MOV     SI, msg             ; 信息显示标签赋给源变址寄存器SI

putloop: 
        MOV     AL, [SI]            ; 源变址寄存器SI中存放的内存地址，取出其对应内存单元中数据，赋给累加寄存器低位AL
        ADD     SI, 1               ; SI值+1，感觉就是把msg标签中的汇编指令一条一条向下执行，直到执行到写了“0”的内存单元
        CMP     AL, 0               ; 将AL的值与0进行比较
        JE      fin                 ; Jump if Equal，即上一条CMP的结果为真，则跳转到fin标签；否则继续执行
        ; 到这里就证明AL中的值不是0
        MOV     AH, 0x0e            ; 显示一个文字
        MOV     BX, 15              ; 指定字符颜色，BH: 0000，BL: 1111
        INT     0x10                ; 调用显卡BIOS（basic input/output system）
        JMP     putloop             ; 跳转到循环的开头

fin:
        HLT                         ; 让CPU停止，等待指令
        JMP     fin                 ; 跳回fin标签，即无限循环CPU停止


msg: ; 信息显示
        DB      0x0a, 0x0a          ; 换行2次
        DB      "Hello, world!"     ; 利用Data Byte指令，把字符串中每一个字符对应的代码一个一个写入内存中
                ; DB "H"
                ; DB "e"
                ; DB "l"
                ; ...
                ; 写入的每一个字符，都会通过putloop里的MOV AH, 0x0e中被显示出来
        DB      0x0a                ; 换行1次
        DB      0                   

        RESB    0x7dfe - $          ; 从当前字节开始，填写0x00，直到地址0x001fe

        DB      0x55, 0xaa          ; 启动区最后2个字节的内容应是0x55和0xAA

; 删掉了启动区以外部分的输出
