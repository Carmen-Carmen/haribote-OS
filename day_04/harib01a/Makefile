TOOLPATH = ../z_tools/
INCPATH  = ../z_tools/haribote/

MAKE     = $(TOOLPATH)make.exe -r
NASK     = $(TOOLPATH)nask.exe
CC1      = $(TOOLPATH)cc1.exe -I$(INCPATH) -Os -Wall -quiet
GAS2NASK = $(TOOLPATH)gas2nask.exe -a
OBJ2BIM  = $(TOOLPATH)obj2bim.exe
BIM2HRB  = $(TOOLPATH)bim2hrb.exe
RULEFILE = $(TOOLPATH)haribote/haribote.rul
EDIMG    = $(TOOLPATH)edimg.exe
IMGTOL   = $(TOOLPATH)imgtol.com
COPY     = copy
DEL      = del

# 默认执行

default :
	$(MAKE) img

# 文件生成规定

ipl10.bin : ipl10.asm Makefile
	$(NASK) ipl10.asm ipl10.bin ipl10.lst

asmhead.bin : asmhead.asm Makefile
	$(NASK) asmhead.asm asmhead.bin asmhead.lst

# C compiler1将bootpack.c编译成bootpack.gas gas是g的as汇编
bootpack.gas : bootpack.c Makefile
	$(CC1) -o bootpack.gas bootpack.c

# GAS2NASK工具，产生真正的汇编文件
bootpack.asm : bootpack.gas Makefile
	$(GAS2NASK) bootpack.gas bootpack.asm

# 使用NASK制作目标文件（obj），即src是C语言，目标是机器语言
bootpack.obj : bootpack.asm Makefile
	$(NASK) bootpack.asm bootpack.obj bootpack.lst

naskfunc.obj : naskfunc.asm Makefile
	$(NASK) naskfunc.asm naskfunc.obj naskfunc.lst

# 然而目标文件只是一种特殊的机器语言文件，必须和其他文件链接（link）后才可以变成可以执行的机器语言
# 使用OBJ2BIM工具将目标文件转换成二进制映像文件（binary image, bim）
bootpack.bim : bootpack.obj naskfunc.obj Makefile
	$(OBJ2BIM) @$(RULEFILE) out:bootpack.bim stack:3136k map:bootpack.map \
		bootpack.obj naskfunc.obj
# 3MB+64KB=3136KB

# BIM2HRB工具对生成的二进制映像文件进行一定加工
bootpack.hrb : bootpack.bim Makefile
	$(BIM2HRB) bootpack.bim bootpack.hrb 0

# 合成一整个haribote.sys文件
haribote.sys : asmhead.bin bootpack.hrb Makefile
	copy /B asmhead.bin+bootpack.hrb haribote.sys

# 把haribote.sys和启动程序装载器一起放到haribote.img映像文件中
haribote.img : ipl10.bin haribote.sys Makefile
	$(EDIMG)   imgin:../z_tools/fdimg0at.tek \
		wbinimg src:ipl10.bin len:512 from:0 to:0 \
		copy from:haribote.sys to:@: \
		imgout:haribote.img

# 命令参数

img :
	$(MAKE) haribote.img

run :
	$(MAKE) img
	$(COPY) haribote.img ..\z_tools\qemu\fdimage0.bin
	$(MAKE) -C ../z_tools/qemu

install :
	$(MAKE) img
	$(IMGTOL) w a: haribote.img

clean :
	-$(DEL) *.bin
	-$(DEL) *.lst
	-$(DEL) *.gas
	-$(DEL) *.obj
	-$(DEL) bootpack.asm
	-$(DEL) bootpack.map
	-$(DEL) bootpack.bim
	-$(DEL) bootpack.hrb
	-$(DEL) haribote.sys

src_only :
	$(MAKE) clean
	-$(DEL) haribote.img

