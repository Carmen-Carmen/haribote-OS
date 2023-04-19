; haribote-os boot asm
; TAB=4

BOTPAK  EQU     0x00280000      ; bootpack装载的位置
DSKCAC  EQU     0x00100000      ; 磁盘缓存地址
DSKCAC0 EQU     0x00008000      ; 磁盘缓存地址（real mode）

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

; 设置图形模式
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

; 以下部分的用处：调用C语言编写的程序
; PICが一切の割り込みを受け付けないようにする
;	AT互換機の仕様では、PICの初期化をするなら、
;	こいつをCLI前にやっておかないと、たまにハングアップする
;	PICの初期化はあとでやる

		MOV		AL,0xff
		OUT		0x21,AL
		NOP						; OUT命令を連続させるとうまくいかない機種があるらしいので
		OUT		0xa1,AL

		CLI						; さらにCPUレベルでも割り込み禁止

; CPUから1MB以上のメモリにアクセスできるように、A20GATEを設定

		CALL	waitkbdout
		MOV		AL,0xd1
		OUT		0x64,AL
		CALL	waitkbdout
		MOV		AL,0xdf			; enable A20
		OUT		0x60,AL
		CALL	waitkbdout

; プロテクトモード移行

[INSTRSET "i486p"]				; 486の命令まで使いたいという記述

		LGDT	[GDTR0]			; 暫定GDTを設定
		MOV		EAX,CR0
		AND		EAX,0x7fffffff	; bit31を0にする（ページング禁止のため）
		OR		EAX,0x00000001	; bit0を1にする（プロテクトモード移行のため）
		MOV		CR0,EAX
		JMP		pipelineflush
pipelineflush:
		MOV		AX,1*8			;  読み書き可能セグメント32bit
		MOV		DS,AX
		MOV		ES,AX
		MOV		FS,AX
		MOV		GS,AX
		MOV		SS,AX

; bootpackの転送

		MOV		ESI,bootpack	; 転送元
		MOV		EDI,BOTPAK		; 転送先
		MOV		ECX,512*1024/4
		CALL	memcpy

; ついでにディスクデータも本来の位置へ転送

; まずはブートセクタから

		MOV		ESI,0x7c00		; 転送元
		MOV		EDI,DSKCAC		; 転送先
		MOV		ECX,512/4
		CALL	memcpy

; 残り全部

		MOV		ESI,DSKCAC0+512	; 転送元
		MOV		EDI,DSKCAC+512	; 転送先
		MOV		ECX,0
		MOV		CL,BYTE [CYLS]
		IMUL	ECX,512*18*2/4	; シリンダ数からバイト数/4に変換
		SUB		ECX,512/4		; IPLの分だけ差し引く
		CALL	memcpy

; asmheadでしなければいけないことは全部し終わったので、
;	あとはbootpackに任せる

; bootpackの起動

		MOV		EBX,BOTPAK
		MOV		ECX,[EBX+16]
		ADD		ECX,3			; ECX += 3;
		SHR		ECX,2			; ECX /= 4;
		JZ		skip			; 転送するべきものがない
		MOV		ESI,[EBX+20]	; 転送元
		ADD		ESI,EBX
		MOV		EDI,[EBX+12]	; 転送先
		CALL	memcpy
skip:
		MOV		ESP,[EBX+12]	; スタック初期値
		JMP		DWORD 2*8:0x0000001b

waitkbdout:
		IN		 AL,0x64
		AND		 AL,0x02
		JNZ		waitkbdout		; ANDの結果が0でなければwaitkbdoutへ
		RET

memcpy:
		MOV		EAX,[ESI]
		ADD		ESI,4
		MOV		[EDI],EAX
		ADD		EDI,4
		SUB		ECX,1
		JNZ		memcpy			; 引き算した結果が0でなければmemcpyへ
		RET
; memcpyはアドレスサイズプリフィクスを入れ忘れなければ、ストリング命令でも書ける

		ALIGNB	16
GDT0:
		RESB	8				; ヌルセレクタ
		DW		0xffff,0x0000,0x9200,0x00cf	; 読み書き可能セグメント32bit
		DW		0xffff,0x0000,0x9a28,0x0047	; 実行可能セグメント32bit（bootpack用）

		DW		0
GDTR0:
		DW		8*3-1
		DD		GDT0

		ALIGNB	16
bootpack:
