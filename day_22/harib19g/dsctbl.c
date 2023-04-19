// GDT和IDT等记录表相关
#include "bootpack.h"

void init_gdtidt(void) {
	struct SEGMENT_DESCRIPTOR *gdt = (struct SEGMENT_DESCRIPTOR *) ADR_GDT;  // 将0x270000 ~ 0x27ffff的内存设定为GDT(8KB)
	struct GATE_DESCRIPTOR    *idt = (struct GATE_DESCRIPTOR    *) ADR_IDT;  // 将0x26f800 ~ 0x26ffff的内存设定为IDT(2KB)
	int i;

	// 初始化GDT
	for (i = 0; i <= LIMIT_GDT / 8; i++) {
		set_segmdesc(gdt + i, 0, 0, 0); // 将上限（limit，段字节数 - 1）、基址（base）和访问权限都设为0
	}
	set_segmdesc(gdt + 1, 0xffffffff, 0x00000000, AR_DATA32_RW);  // 将1段上限值设为0xffffffff（4GB），基址设为0，访问权限设为0x4092（可读可写）
	set_segmdesc(gdt + 2, LIMIT_BOTPAK, ADR_BOTPAK, AR_CODE32_ER);  // 将2段上限值设为0x007ffff（512KB），基址设为0x00280000，是为bootpack.hrb准备的，因为它是以ORG 0为前提翻译成的机器语言
	load_gdtr(LIMIT_GDT, ADR_GDT);  // naskfunc.asm中编写的将GDT信息写入GDTR的方法，包括了内存的开始地址（0x00270000）和个数（0xffff / 8，8192个）

	// 初始化IDT
	for (i = 0; i < LIMIT_IDT / 8; i++) {
		set_gatedesc(idt + i, 0, 0, 0); // DPL设为00，即是内核态
	}
	load_idtr(LIMIT_IDT, ADR_IDT);   // naskfunc.asm中编写的将IDT信息写入IDTR的方法，包括了内存的开始地址（0x0026f800）和个数

        // 设定外部设备的IDT
        set_gatedesc(idt + 0x0d, (int) asm_inthandler0d , 2 << 3, AR_INTGATE32); // 强制结束应用程序
        set_gatedesc(idt + 0x0c, (int) asm_inthandler0c , 2 << 3, AR_INTGATE32); // 处理栈异常
        set_gatedesc(idt + 0x20, (int) asm_inthandler20 , 2 << 3, AR_INTGATE32); // PIC0 PIT定时器
        set_gatedesc(idt + 0x21, (int) asm_inthandler21 , 2 << 3, AR_INTGATE32); // keyboard
        set_gatedesc(idt + 0x2c, (int) asm_inthandler2c , 2 << 3, AR_INTGATE32); // mouse
        set_gatedesc(idt + 0x27, (int) asm_inthandler27 , 2 << 3, AR_INTGATE32); // IRQ7
        set_gatedesc(idt + 0x40, (int) asm_hrb_api      , 2 << 3, AR_INTGATE32 + 0x60); // 将INT 0x40注册为"可供应用程序作为API来调用的"中断

	return;
}

// set segment description，设置段的描述，根据CPU的规格要求，将每个段的信息归结成8个字节写入内存
/*
    struct SEGMENT_DESCRIPTOR: 存放属性的结构体
    unsighed int limit: 段的上限（段的大小）
    int base: 段的基址（段的起始地址）
    int ar: 段的访问权限access right?（管理属性）
*/
void set_segmdesc(struct SEGMENT_DESCRIPTOR *sd, unsigned int limit, int base, int ar) {
	if (limit > 0xfffff) {
		ar |= 0x8000; /* G_bit = 1，即采取分页模式而不是分段 */
		limit /= 0x1000;
	}
	sd->limit_low    = limit & 0xffff;
	sd->base_low     = base & 0xffff;
	sd->base_mid     = (base >> 16) & 0xff;                         // >>是向右移位运算
	sd->access_right = ar & 0xff;
	sd->limit_high   = ((limit >> 16) & 0x0f) | ((ar >> 8) & 0xf0);
	sd->base_high    = (base >> 24) & 0xff;
	return;
}

// set gate description，设置中断描述
/*
    struct GATE_DESCRIPTOR: 存放属性的结构体
    unsighed int offset: 中断函数
    int selector: 中断函数属于哪个段，低3位有别的用处必须是0，所以传参时 selector << 3左移三位
    int ar: 中断的访问权限access right?
*/
void set_gatedesc(struct GATE_DESCRIPTOR *gd, int offset, int selector, int ar) {
	gd->offset_low   = offset & 0xffff;
	gd->selector     = selector;
	gd->dw_count     = (ar >> 8) & 0xff;
	gd->access_right = ar & 0xff;
	gd->offset_high  = (offset >> 16) & 0xffff;
	return;
}
