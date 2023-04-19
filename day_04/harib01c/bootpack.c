// 还需要把naskfunc.asm中编写的函数显式的声明出来
void io_hlt(void);
void write_mem8(int addr, int data);


void HariMain(void) {
    int i;
    char *p; // 变量p是用于内存地址的专用变量，char型是8位即1个Byte，就能让编译器知道这个语句的实际含义是MOV BYTE [i], i & 0x0f

    // 向图像缓冲区VRAM中全部写入15（白色）
    // VRAM的开始地址是0x000a0000，图像分辨率为320*200 = 64000
    // 0xaffff - 0xa0000 = 65535
    for (i = 0xa0000; i <= 0xaffff; i++) {
        // 编译错误: invalid type argument of `unary *'
        // 相当于是汇编MOV指令没有规定数据类型是BYTE, WORD还是DWORD
        // *i = i & 0x0f;
        
        // 代替write_mem8(i, i & 0x0f)
        // p = i; // 把当前地址值赋给p
        // *p = i & 0x0f; // 在p指向的内存地址中写入值
        // 编译警告: assignment makes pointer from integer without a cast
        
        // 写成如下形式，可以避免将int型的变量i赋给char型的变量p时报警告
        p = (char *) i; // MOV ECX, i，给ECX寄存器赋值
        *p = i & 0x0f;  // MOV BYTE [ECX], (i & 0x0f)，向ECX寄存器中存放的内存地址写数据

        // 再进一步，甚至也可以不依赖变量p完成向内存的写入
        // MOV BYTE [i], i & 0x0f
        // *((char *) i) = i & 0x0f;

        
    }
    

    // 无限循环cpu休眠
    while (1) {
        io_hlt(); /*执行naskfunc.asm里的_io_hlt*/
    }
 
}
