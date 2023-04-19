#include <string.h>
#include "bootpack.h"

struct FILEINFO *file_search(char *name, struct FILEINFO *finfo, int max) {
    int i, j;
    char s[12];
    for (j = 0; j < 11; j++) {
        s[j] = ' ';
    }
    j = 0;
    for (i = 0; name[i] != 0x00; i++) {
        if (j >= 11) {
            return 0;   // 文件名大于8个字符，太长了
        }
        if (name[i] == '.' && j <= 8) {
            j = 8;
        } else {
            s[j] = name[i];
            if ('a' <= s[j] && s[j] <= 'z') {
                s[j] -= 0x20;
            }
            j++;
        }
    }

    // 现在可以通过在命令行输出变量的形式进行debug啦
    // cons.cur_x = 8;
    // cons_putstring(&cons, s, COL8_FFFFFF, COL8_000000, 0, 11);
    // cons_newline(&cons);

    for (i = 0; i < max;) {
        if (finfo[i].name[0] == 0x00) {
            break;
        }

        if ((finfo[i].type & 0x18) == 0) {
            for (j = 0; j < 11; j++) {
                if (finfo[i].name[j] != s[j]) {
                    goto next;
                }
            }
            return finfo + i;
        }
next: 
        i++;
    }

    return 0;
}

/*
    解压缩磁盘映像中FAT
    解压缩算法：
        img指向磁盘映像缓冲中的压缩过的fat
        压缩数据每6个为一组，abc def，换位成dab efc
*/
void file_decodefat(int *fat, unsigned char *img) {
    int i, j = 0;
    for (i = 0; i < 2880; i += 2) {
        fat[i + 0] = (img[j + 0] | img[j + 1] << 8) & 0xfff;
        fat[i + 1] = (img[j + 1] >> 4 | img[j + 2] << 4) & 0xfff;
        j += 3;
    }
    return;
}

/*
    按照fat读取文件，读到buf指针中
    fat是经过解压后的fat
    img是指向 磁盘缓冲区 中内容的指针，通过clustno计算出偏移量来拷贝数据
*/
void file_loadfile(int clustno, int size, char *buf, int *fat, char *img) {
    int i;
    while (1) {
        if (size <= 512) {
            // 对于小于512 Byte的文件，或者读到大文件最后一个碎片，直接读入并跳出循环
            for (i = 0; i < size; i++) {
                buf[i] = img[clustno * 512 + i];
            }
            break;
        }

        // 否则，从不同簇号（扇区）中读入512 Byte
        for (i = 0; i < 512; i++) {
            buf[i] = img[clustno * 512 + i];
        }

        size -= 512;
        buf += 512;     // 将buf指针的基址+512
        clustno = fat[clustno]; // 按照fat指示找到下一个簇号，接力往后读
    }
    return;
}

