# 所有obj文件都要打包进bootpack.bim
OBJS_BOOTPACK = bootpack.obj naskfunc.obj hankaku.obj graphic.obj dsctbl.obj int.obj fifo.obj keyboard.obj mouse.obj memory.obj sheet.obj timer.obj multitask.obj console.obj window.obj file.obj

TOOLPATH = ../z_tools/
INCPATH  = ../z_tools/haribote/

MAKE     = $(TOOLPATH)make.exe -r
NASK     = $(TOOLPATH)nask.exe
CC1      = $(TOOLPATH)cc1.exe -I$(INCPATH) -Os -Wall -quiet
GAS2NASK = $(TOOLPATH)gas2nask.exe -a
OBJ2BIM  = $(TOOLPATH)obj2bim.exe
MAKEFONT = $(TOOLPATH)makefont.exe
BIN2OBJ  = $(TOOLPATH)bin2obj.exe
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

naskfunc.obj : naskfunc.asm Makefile
	$(NASK) naskfunc.asm naskfunc.obj naskfunc.lst

# 编译汇编应用程序
hello.hrb : hello.asm Makefile
	$(NASK) hello.asm hello.hrb hello.lst
hello2.hrb : hello2.asm Makefile
	$(NASK) hello2.asm hello2.hrb hello2.lst
crack2.hrb : crack2.asm Makefile
	$(NASK) crack2.asm crack2.hrb crack2.lst
hello5.bim : hello5.obj Makefile
	$(OBJ2BIM) @$(RULEFILE) out:hello5.bim stack:1k map:hello5.map hello5.obj
hello5.hrb : hello5.bim Makefile
	$(BIM2HRB) hello5.bim hello5.hrb 0

# 用C编写的应用程序a.c
a.bim : a.obj a_asm.obj Makefile
	$(OBJ2BIM) @$(RULEFILE) out:a.bim map:a.map a.obj a_asm.obj

a.hrb : a.bim Makefile
	$(BIM2HRB) a.bim a.hrb 0

hello3.bim : hello3.obj a_asm.obj Makefile
	$(OBJ2BIM) @$(RULEFILE) out:hello3.bim map:hello3.map hello3.obj a_asm.obj

hello3.hrb : hello3.bim Makefile
	$(BIM2HRB) hello3.bim hello3.hrb 0

hello4.bim : hello4.obj a_asm.obj Makefile
	$(OBJ2BIM) @$(RULEFILE) out:hello4.bim stack:1k map:hello4.map hello4.obj a_asm.obj

hello4.hrb : hello4.bim Makefile
	$(BIM2HRB) hello4.bim hello4.hrb 0

bug1.bim : bug1.obj a_asm.obj Makefile
	$(OBJ2BIM) @$(RULEFILE) out:bug1.bim map:bug1.map bug1.obj a_asm.obj

bug1.hrb : bug1.bim Makefile
	$(BIM2HRB) bug1.bim bug1.hrb 0

bug2.bim : bug2.obj a_asm.obj Makefile
	$(OBJ2BIM) @$(RULEFILE) out:bug2.bim map:bug2.map bug2.obj a_asm.obj

bug2.hrb : bug2.bim Makefile
	$(BIM2HRB) bug2.bim bug2.hrb 0

bug3.bim : bug3.obj a_asm.obj Makefile
	$(OBJ2BIM) @$(RULEFILE) out:bug3.bim map:bug3.map bug3.obj a_asm.obj

bug3.hrb : bug3.bim Makefile
	$(BIM2HRB) bug3.bim bug3.hrb 0

winhelo.bim : winhelo.obj a_asm.obj Makefile
	$(OBJ2BIM) @$(RULEFILE) out:winhelo.bim map:winhelo.map winhelo.obj a_asm.obj

winhelo.hrb : winhelo.bim Makefile
	$(BIM2HRB) winhelo.bim winhelo.hrb 0

winhelo2.bim : winhelo2.obj a_asm.obj Makefile
	$(OBJ2BIM) @$(RULEFILE) out:winhelo2.bim map:winhelo2.map winhelo2.obj a_asm.obj

winhelo2.hrb : winhelo2.bim Makefile
	$(BIM2HRB) winhelo2.bim winhelo2.hrb 0

winhelo3.bim : winhelo3.obj a_asm.obj Makefile
	$(OBJ2BIM) @$(RULEFILE) out:winhelo3.bim map:winhelo3.map winhelo3.obj a_asm.obj

winhelo3.hrb : winhelo3.bim Makefile
	$(BIM2HRB) winhelo3.bim winhelo3.hrb 40k

star1.bim : star1.obj a_asm.obj Makefile
	$(OBJ2BIM) @$(RULEFILE) out:star1.bim map:star1.map star1.obj a_asm.obj

star1.hrb : star1.bim Makefile
	$(BIM2HRB) star1.bim star1.hrb 47k

stars.bim : stars.obj a_asm.obj Makefile
	$(OBJ2BIM) @$(RULEFILE) out:stars.bim map:stars.map stars.obj a_asm.obj

stars.hrb : stars.bim Makefile
	$(BIM2HRB) stars.bim stars.hrb 47k

stars2.bim : stars2.obj a_asm.obj Makefile
	$(OBJ2BIM) @$(RULEFILE) out:stars2.bim map:stars2.map stars2.obj a_asm.obj

stars2.hrb : stars2.bim Makefile
	$(BIM2HRB) stars2.bim stars2.hrb 0

lines.bim : lines.obj a_asm.obj Makefile
	$(OBJ2BIM) @$(RULEFILE) out:lines.bim map:lines.map lines.obj a_asm.obj

lines.hrb : lines.bim Makefile
	$(BIM2HRB) lines.bim lines.hrb 48k

walk.bim : walk.obj a_asm.obj Makefile
	$(OBJ2BIM) @$(RULEFILE) out:walk.bim map:walk.map walk.obj a_asm.obj

walk.hrb : walk.bim Makefile
	$(BIM2HRB) walk.bim walk.hrb 48k

noodle.bim : noodle.obj a_asm.obj Makefile
	$(OBJ2BIM) @$(RULEFILE) out:noodle.bim map:noodle.map noodle.obj a_asm.obj

noodle.hrb : noodle.bim Makefile
	$(BIM2HRB) noodle.bim noodle.hrb 40k

beepdown.hrb : beepdown.bim Makefile
	$(BIM2HRB) beepdown.bim beepdown.hrb 40k

beepdown.bim : beepdown.obj a_asm.obj Makefile
	$(OBJ2BIM) @$(RULEFILE) out:beepdown.bim map:beepdown.map beepdown.obj a_asm.obj

color.hrb : color.bim Makefile
	$(BIM2HRB) color.bim color.hrb 40k

color.bim : color.obj a_asm.obj Makefile
	$(OBJ2BIM) @$(RULEFILE) out:color.bim map:color.map color.obj a_asm.obj

color2.hrb : color2.bim Makefile
	$(BIM2HRB) color2.bim color2.hrb 40k

color2.bim : color2.obj a_asm.obj Makefile
	$(OBJ2BIM) @$(RULEFILE) out:color2.bim map:color2.map color2.obj a_asm.obj

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


                # hello.hrb hello2.hrb a.hrb hello3.hrb hello4.hrb hello5.hrb \
                # crack2.hrb bug1.hrb bug2.hrb bug3.hrb \

                # copy from:hello.hrb to:@: \
                # copy from:hello2.hrb to:@: \
                # copy from:a.hrb to:@: \
                # copy from:hello3.hrb to:@: \
                # copy from:hello4.hrb to:@: \
                # copy from:hello5.hrb to:@: \
                # copy from:crack2.hrb to:@: \
                # copy from:bug1.hrb to:@: \
                # copy from:bug2.hrb to:@: \
                # copy from:bug3.hrb to:@: \
# 把haribote.sys和启动程序装载器一起放到haribote.img映像文件中
haribote.img : ipl10.bin haribote.sys \
                winhelo.hrb winhelo2.hrb winhelo3.hrb \
                star1.hrb stars.hrb stars2.hrb lines.hrb walk.hrb \
                noodle.hrb \
                beepdown.hrb color.hrb color2.hrb \
                Makefile
	$(EDIMG)   imgin:../z_tools/fdimg0at.tek \
		wbinimg src:ipl10.bin len:512 from:0 to:0 \
		copy from:haribote.sys to:@: \
                copy from:ipl10.asm to:@: \
		copy from:example.txt to:@: \
                copy from:mouse.py to:@: \
                copy from:winhelo.hrb to:@: \
                copy from:winhelo2.hrb to:@: \
                copy from:winhelo3.hrb to:@: \
                copy from:star1.hrb to:@: \
                copy from:stars.hrb to:@: \
                copy from:stars2.hrb to:@: \
                copy from:lines.hrb to:@: \
                copy from:walk.hrb to:@: \
                copy from:noodle.hrb to:@: \
                copy from:beepdown.hrb to:@: \
                copy from:color.hrb to:@: \
                copy from:color2.hrb to:@: \
		imgout:haribote.img

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
	-$(DEL) *.map
	-$(DEL) *.bim
	-$(DEL) *.hrb
	-$(DEL) haribote.sys

src_only :
	$(MAKE) clean
	-$(DEL) haribote.img
	-$(DEL) *.hrb

