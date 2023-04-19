问题：
启动lines后，通过enter关闭后会出现general protection except
eip指向naskfunc中的jmp

研究：
发现将自己写的bootpack.c and console.c替换到原工程后，也会出现卡死
将原工程中的bootpack.c and console.c替换过来后，可以正常结束

本文件夹中内容为改写后的原工程中的bootpack.c and console.c文件
