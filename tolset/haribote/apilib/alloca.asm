[FORMAT "WCOFF"]
[INSTRSET "i486p"]
[BITS 32]
[FILE "alloca.asm"]

    GLOBAL  __alloca

[SECTION .text]

__alloca:
    ADD	    EAX, -4
    SUB	    ESP, EAX
    JMP	    DWORD [ESP + EAX]	; 代替RET，因为ESP + EAX的值正好是减法运算之前ESP的值

    ; __alloca会被C语言程序调用的情况
    ;	- 要执行的操作从栈中分配EAX个Byte的内存空间，即ESP -= EAX
    ;	- 要遵守的规则不能改变ECX, EDX, EBP, ESI, EDI的值，或需要使用PUSH/POP来复原
    ; 但是实际上RET命令相当于"POP EIP"，即MOV	EIP, [ESP] && ADD   ESP, 4
    ; 所以ESP + EAX相对于正确的ESP还差了4个Byte，因此在程序开头就将EAX - 4
