# 所有obj文件都要打包进bootpack.bim
OBJS_BOOTPACK = bootpack.obj asmfunc.obj hankaku.obj graphic.obj dsctbl.obj int.obj fifo.obj keyboard.obj mouse.obj memory.obj sheet.obj timer.obj multitask.obj console.obj window.obj file.obj

TOOLPATH = ../../z_tools/
INCPATH  = ../../z_tools/haribote/

MAKE     = $(TOOLPATH)make.exe -r
NASK     = $(TOOLPATH)nask.exe
CC1      = $(TOOLPATH)cc1.exe -I$(INCPATH) -Os -Wall -quiet
GAS2NASK = $(TOOLPATH)gas2nask.exe -a
OBJ2BIM  = $(TOOLPATH)obj2bim.exe
MAKEFONT = $(TOOLPATH)makefont.exe
BIN2OBJ  = $(TOOLPATH)bin2obj.exe
BIM2HRB  = $(TOOLPATH)bim2hrb.exe
RULEFILE = ../haribote.rul
EDIMG    = $(TOOLPATH)edimg.exe
IMGTOL   = $(TOOLPATH)imgtol.com
GOLIB    = $(TOOLPATH)golib00.exe
COPY     = copy
DEL      = del

# 默认执行
default :
	$(MAKE) ipl10.bin
	$(MAKE)	haribote.sys

# 文件生成规定
ipl10.bin : ipl10.asm Makefile
	$(NASK) ipl10.asm ipl10.bin ipl10.lst

asmhead.bin : asmhead.asm Makefile
	$(NASK) asmhead.asm asmhead.bin asmhead.lst

asmfunc.obj : asmfunc.asm Makefile
	$(NASK) asmfunc.asm asmfunc.obj asmfunc.lst

# 把hanhaku.txt编译成bin文件
hankaku.bin : hankaku.txt Makefile
	$(MAKEFONT) hankaku.txt hankaku.bin

# 把bin文件转化成obj文件，为了可以与bootpack.obj连接
hankaku.obj : hankaku.bin Makefile
	$(BIN2OBJ) hankaku.bin hankaku.obj _hankaku

# 然而目标文件只是一种特殊的机器语言文件，必须和其他文件链接（link）后才可以变成可以执行的机器语言
# 使用OBJ2BIM工具将目标文件转换成二进制映像文件（binary image, bim）
bootpack.bim : $(OBJS_BOOTPACK) Makefile
	$(OBJ2BIM) @$(RULEFILE) out:bootpack.bim stack:3136k map:bootpack.map \
		$(OBJS_BOOTPACK)
# 3MB+64KB=3136KB

# BIM2HRB工具对生成的二进制映像文件进行一定加工
bootpack.hrb : bootpack.bim Makefile
	$(BIM2HRB) bootpack.bim bootpack.hrb 0

# 合成一整个haribote.sys文件
haribote.sys : asmhead.bin bootpack.hrb Makefile
	copy /B asmhead.bin+bootpack.hrb haribote.sys


# 对于C语言文件的普遍处理方式

# C compiler1将*.c编译成*.gas gas是g的as汇编
# 注意通配符是"%"，而路径中的通配符又是"$*"
%.gas : %.c Makefile
	$(CC1) -o $*.gas $*.c

# GAS2NASK工具，产生真正的汇编文件
%.asm : %.gas Makefile
	$(GAS2NASK) $*.gas $*.asm

# 使用NASK制作目标文件（obj），即src是C语言，目标是机器语言
%.obj : %.asm Makefile
	$(NASK) $*.asm $*.obj $*.lst

# 命令参数
clean :
	-$(DEL) asmhead.bin
	-$(DEL)	hankaku.bin
	-$(DEL) *.lst
	-$(DEL) *.obj
	-$(DEL) *.map
	-$(DEL) *.bim
	-$(DEL) *.hrb

src_only :
	$(MAKE) clean
	-$(DEL)	ipl10.bin
	-$(DEL) haribote.sys
