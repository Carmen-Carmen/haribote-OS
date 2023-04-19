// console.c
#include <stdio.h>
#include <string.h>
#include "bootpack.h"

// 命令行窗口任务的入口函数
void console_task(struct SHEET *sheet, unsigned int memtotal) {
    struct TIMER *timer;
    struct TASK *task = task_now();
    int i, x, y, fifobuf[128], 
        cmdindex = 0,   // 命令字符串的索引
        cursor_x = 24, 
        cursor_y = 28,  // 光标y坐标
        cursor_c = -1;  // 将cursor_c初始化成-1，由后面主循环来判断是否要开始光标闪烁
    char s[30];
    char cmdline[2048];
    struct MEMMAN *memman = (struct MEMMAN *) MEMMAN_ADDR;
    struct FILEINFO *finfo = (struct FILEINFO *) (ADR_DISKIMG + 0x002600);
    char * p_disk;
    int *fat = (int *) memman_alloc_4k(memman, 4 * 2880);
    struct SEGMENT_DESCRIPTOR *gdt = (struct SEGMENT_DESCRIPTOR *) ADR_GDT;

    fifo32_init(&task->fifo, 128, fifobuf, task); // 有数据写入时，唤醒task_cons
    timer = timer_alloc();
    timer_init(timer, &task->fifo, 1);
    timer_settime(timer, 50);

    file_decodefat(fat, (unsigned char *) (ADR_DISKIMG + 0x000200));  // 将磁盘映像中的fat内容解压，并载入fat指针

    // 显示提示符 $ 
    putstring8_ascii_sht(sheet, 8, cursor_y, COL8_00FFFF, COL8_000000, "$ ");
    int multi_line = 0; // 记录当前一行是否为多行的flag

    for (;;) {
        io_cli();
        if (fifo32_status(&task->fifo) == 0) {
            task_sleep(task);
            io_sti();
        } else {
            i = fifo32_get(&task->fifo);
            io_sti();

            if (i <= 1) {
                // data = 1，说明是光标定时器
                if (i != 0) {
                    timer_init(timer, &task->fifo, 0);
                    if (cursor_c >= 0) {
                        cursor_c = COL8_FFFFFF;
                    }
                } else {
                    timer_init(timer, &task->fifo, 1);
                    if (cursor_c >= 0) {
                        cursor_c = COL8_000000;
                    }
                }
                timer_settime(timer, 50);

            } else if (256 <= i && i <= 511) {
                // 证明是task_a传来的键盘数据
                i -= 256;

                if (i == 8) {           
                // 退格键
                    cursor_c = COL8_FFFFFF;
                    if (cmdindex > 0) {
                        cmdline[cmdindex--] = 0;
                    }

                    if (multi_line == 0) {      
                        // 只有一行
                        if (cursor_x > 24) {    // 大于24是为了不要把命令行提示符也给删掉
                            putstring8_ascii_sht(sheet, cursor_x, cursor_y, COL8_FFFFFF, COL8_000000, " ");
                            cursor_x -= 8;
                        }
                    } else if (cursor_y <= 28) {
                        // cursor_y == 28 && multi_line != 0; 这种情况是输入多行内容后滚动导致的
                        if (cursor_x > 8) {     // 在当前行内退格
                            putstring8_ascii_sht(sheet, cursor_x, cursor_y, COL8_FFFFFF, COL8_000000, " ");
                            cursor_x -= 8;
                        } else {                // 退格至前一行
                            /*
                                从文字缓冲区中取出被滚动走的之前的属于这一行的内容，并且显示出来
                                通过multi_line的行数来计算出应该显示哪些，并显示在第一行
                            */
                            multi_line--;
                            int s_index;
                            if (multi_line != 0) {
                                for (s_index = (400 - 16) / 8 + (multi_line - 1) * 400 / 8; s_index <= cmdindex; s_index++) {
                                    s[s_index - ((400 - 16) / 8 + (multi_line - 1) * 400 / 8)] = cmdline[s_index];
                                }
                                s[400 / 8] = 0;
                                putstring8_ascii_sht(sheet, 8, 28, COL8_FFFFFF, COL8_000000, s);
                            } else {
                                for (s_index = 0; s_index <= cmdindex; s_index++) {
                                    s[s_index] = cmdline[s_index];
                                }
                                s[(400 - 16) / 8] = 0;
                                putstring8_ascii_sht(sheet, 8, 28, COL8_00FFFF, COL8_000000, "$ ");
                                putstring8_ascii_sht(sheet, 24, 28, COL8_FFFFFF, COL8_000000, s);
                            }
                            cursor_x = 400;
                            cursor_c = -1;
                        }
                    } else {                    
                        // 存在多行
                        if (cursor_x > 8) {     // 在当前行内退格
                            putstring8_ascii_sht(sheet, cursor_x, cursor_y, COL8_FFFFFF, COL8_000000, " ");
                            cursor_x -= 8;
                        } else {                // 退格至前一行
                            multi_line--;
                            // 删除当前位置的光标
                            putstring8_ascii_sht(sheet, cursor_x, cursor_y, COL8_FFFFFF, COL8_000000, " ");
                            cursor_x = 400;
                            cursor_y -= 16;
                        }
                    }
                } else if (i == 10) {   
                // 回车键
                    multi_line = 0;
                    // 删除当前位置的光标
                    putstring8_ascii_sht(sheet, cursor_x, cursor_y, COL8_FFFFFF, COL8_000000, " ");
                    
                    cmdline[cmdindex] = 0;
                    cursor_y = cons_newline(cursor_y, sheet);

                    // 执行命令
                    if (strcmp(cmdline, "mem") == 0) {
                        // "mem" command
                        sprintf(s, "total   %dMB", memtotal / (1024 * 1024));
                        putstring8_ascii_sht(sheet, 8, cursor_y, COL8_FFFFFF, COL8_000000, s);
                        cursor_y = cons_newline(cursor_y, sheet);
                        sprintf(s, "free %dKB", memman_total(memman) / 1024);
                        putstring8_ascii_sht(sheet, 8, cursor_y, COL8_FFFFFF, COL8_000000, s);
                        cursor_y = cons_newline(cursor_y, sheet);
                        cursor_y = cons_newline(cursor_y, sheet);
                    } else if (strcmp(cmdline, "clear") == 0) {
                        // "clear" command
                        cons_clear(sheet, 8, 28, 400, 320);
                        cursor_y = 28; 
                    } else if (strcmp(cmdline, "ls") == 0) {
                        // "ls" command
                        for (x = 0; x < 224; x++) { // 最多有224个文件
                            // 利用FINFO类型指针，从内存0x00102600开始按32个Byte读取
                            if (finfo[x].name[0] == 0x00) {
                                // 直到，读到空文件，退出循环
                                break;
                            }
                            if (finfo[x].name[0] != 0xe5) {
                                // 文件名开头不是0xe5，是正常的文件
                                if ((finfo[x].type & 0x18) == 0) {
                                    // 既不是非文件（0x08）也不是目录（0x10），就要读出来
                                    // 暂时只显示文件名为8个字符以内的
                                    sprintf(s, "filename.ext    %7d", finfo[x].size);
                                    for (y = 0; y < 8; y++) {
                                        s[y] = finfo[x].name[y];
                                    }
                                    // 拼接扩展名
                                    s[ 9] = finfo[x].ext[0];
                                    s[10] = finfo[x].ext[1];
                                    s[11] = finfo[x].ext[2];
                                    putstring8_ascii_sht(sheet, 8, cursor_y, COL8_FFFFFF, COL8_000000, s);
                                    cursor_y = cons_newline(cursor_y, sheet);
                                }
                            }
                        }
                        cursor_y = cons_newline(cursor_y, sheet);
                    } else if (strncmp(cmdline, "cat ", 4) == 0 || strcmp(cmdline, "cat") == 0) { 
                        // strncmp的第3个参数是从头开始比较的字符个数
                        // "cat" command

                        if (strcmp(cmdline, "cat") == 0) {
                            cursor_x = 8;
                            cursor_y = cons_putstring(sheet, cursor_x, cursor_y, COL8_FFFFFF, COL8_000000, "usage: cat filename.ext", 0, strlen("usage: cat filename.ext"));
                            cursor_y = cons_newline(cursor_y, sheet);
                            cmdindex = 0;
                            // 显示命令行提示符，并将cursor的x坐标移到最左边
                            putstring8_ascii_sht(sheet, 8, cursor_y, COL8_00FFFF, COL8_000000, "$ ");
                            cursor_x = 24;
                            continue;
                        }

                        // 将文件名读入s指针，少于8个字符的文件名用空格填充
                        for (y = 0; y < 11; y++) {
                            s[y] = ' ';
                        }
                        y = 0;
                        for (x = 4; y < 11 && cmdline[x] != 0; x++) {    // 从 "cat "开始
                            if (cmdline[x] == '.' && y <= 8) {
                                // 读到扩展名前了
                                y = 8;
                            } else {
                                s[y] = cmdline[x];
                                if ('a' <= s[y] && s[y] <= 'z') {
                                    // 将命令输入的小写字母转换成大写
                                    s[y] -= 0x20;
                                }
                                y++;
                            }
                        }
                        // 走到这里s指针指向的是 "xXX     ext" 格式的字符串

                        // 寻找文件
                        for (x = 0; x < 224; ) {
                            if (finfo[x].name[0] == 0x00) {
                                break;
                            }
                            if ((finfo[x].type & 0x18) == 0) {
                                for (y = 0; y < 8; y++) {
                                    if (finfo[x].name[y] != s[y]) {
                                        // 逐个字符比较 文件名
                                        goto next_file;
                                    }
                                }
                                for (y = 8; y < 11; y++) {
                                    if (finfo[x].ext[y - 8] != s[y]) {
                                        // 逐个字符比较 扩展名
                                        goto next_file;
                                    }
                                }
                                break;  // 走完了上面那块for循环，说明文件名内
                            }
next_file: 
                            x++;
                        }

                        if (x < 244 && finfo[x].name[0] != 0x00) {
                            // // 找到这个文件了
                            cursor_x = 8;
                            p_disk = (char *) memman_alloc_4k(memman, finfo[x].size);
                            file_loadfile(finfo[x].clustno, finfo[x].size, p_disk, fat, (char *) (ADR_DISKIMG) + 0x003e00);
                            cursor_y = cons_putstring(sheet, cursor_x, cursor_y, COL8_FFFFFF, COL8_000000, p_disk, 0, finfo[x].size);   // 将文件内容按照fat，load进定长度的p_disk指针中
                            memman_free_4k(memman, (int) p_disk, finfo[x].size);    // 相当于File.close()，不然会一直占用内存
                        } else {
                            // 没有找到文件
                            putstring8_ascii_sht(sheet, 8, cursor_y, COL8_FFFFFF, COL8_840000, "[file not found]: ");
                            cursor_x = 8 + strlen("[file not found]: ") * 8;
                            // 拼接[file not found]: 之后的内容
                            cursor_y = cons_putstring(sheet, cursor_x, cursor_y, COL8_FFFFFF, COL8_840000, cmdline, 4, cmdindex);
                        }
                    } else if (strcmp(cmdline, "fat") == 0){
                        // 想查看下解压缩后的fat长啥样
                        char * fat_content = (char *) memman_alloc_4k(memman, 2880);
                        int fat_index;
                        for (fat_index = 0; fat_index < 2880; fat_index++) {
                            if (fat[fat_index] == 0) break;
                            sprintf(s, "%03x", fat[fat_index]);
                            strcat(fat_content, s);
                            strcat(fat_content, " ");
                            if ((fat_index + 1) % 8 == 0) {
                                strcat(fat_content, "\n");
                            }
                        }
                        cursor_x = 8;
                        cursor_y = cons_putstring(sheet, cursor_x, cursor_y, COL8_FFFFFF, COL8_000000, fat_content, 0, strlen(fat_content));
                        memman_free_4k(memman, (int) fat_content, 2880);
                    } else if (strcmp(cmdline, "hlt") == 0) {
                        // 启动应用程序hlt.hrb
                        for (y = 0; y < 11; y++) {
                            s[y] = ' ';
                        }
                        sprintf(s, "HLT     HRB");

                        for (x = 0; x < 224;) {
                            if (finfo[x].name[0] == 0x00) {
                                break;
                            }
                            if ((finfo[x].type & 0x18) == 0) {
                                for (y = 0; y < 11; y++) {
                                    if (finfo[x].name[y] != s[y]) {
                                        goto hlt_next_file;
                                    }
                                }
                                for (y = 8; y < 11; y++) {
                                    if (finfo[x].ext[y - 8] != s[y]) {
                                        // 逐个字符比较 扩展名
                                        goto hlt_next_file;
                                    }
                                }
                                break;  // 找到hlt.hrb了
                            }
hlt_next_file: 
                            x++;
                        }
                        if (x < 224 && finfo[x].name[0] != 0x00) {
                            // 找到文件
                            p_disk = (char *) memman_alloc_4k(memman, finfo[x].size);
                            file_loadfile(finfo[x].clustno, finfo[x].size, p_disk, fat, (char *) (ADR_DISKIMG + 0x003e00));
                            // 为hlt.hrb设置段描述
                            set_segmdesc(gdt + 1003, finfo[x].size - 1, (int) p_disk, AR_CODE32_ER);
                            farjmp(0, 1003 * 8);    // 跳转执行
                            memman_free_4k(memman, (int) p_disk, finfo[x].size);
                        } else {
                            // 没找到文件
                            putstring8_ascii_sht(sheet, 8, cursor_y, COL8_FFFFFF, COL8_840000, "[file not found]: ");
                            cursor_x = 8 + strlen("[file not found]: ") * 8;
                            cursor_y = cons_putstring(sheet, cursor_x, cursor_y, COL8_FFFFFF, COL8_840000, cmdline, 4, cmdindex);
                        }

                    } else if (cmdline[0] != 0) {
                        // 既不是命令，也不是空行
                        putstring8_ascii_sht(sheet, 8, cursor_y, COL8_FFFFFF, COL8_840000, "[command not found]: ");
                        cursor_x = 8 + strlen("[command not found]: ") * 8;
                        // 拼接[command not found]: 后面的内容
                        cursor_y = cons_putstring(sheet, cursor_x, cursor_y, COL8_FFFFFF, COL8_840000, cmdline, 0, cmdindex);
                    }
                    
                    cmdindex = 0;
                    // 显示命令行提示符，并将cursor的x坐标移到最左边
                    putstring8_ascii_sht(sheet, 8, cursor_y, COL8_00FFFF, COL8_000000, "$ ");
                    cursor_x = 24;
                } else {                
                // 一般字符
                    s[0] = i;
                    s[1] = 0;

                    if (cmdindex > 2048) continue;
                    cmdline[cmdindex++] = i;  
                                        
                    if (cursor_x < 400) {
                        putstring8_ascii_sht(sheet, cursor_x, cursor_y, COL8_FFFFFF, COL8_000000, s);
                        cursor_x += 8;
                    } else {    // 需要切换到下一行去显示了
                        // 先把这个位置要显示的字符打出来
                        putstring8_ascii_sht(sheet, cursor_x, cursor_y, COL8_FFFFFF, COL8_000000, s);
                        multi_line++;
                        if (cursor_y < 28 + 304) {
                            cursor_y += 16;
                        } else {
                            scroll_down(sheet, 8, 28, 400, 320);
                        }
                        cursor_x = 8;
                    }
                }
            } else if (i == 2) {
                // 开始光标闪烁
                cursor_c = COL8_000000;
            } else if (i == 3) {
                // 停止光标闪烁
                cursor_c = -1;
                boxfill8(sheet->buf, sheet->bxsize, COL8_000000, cursor_x, cursor_y, cursor_x + 7, cursor_y + 15);   // 用黑色将光标隐去
            }
            // 重新显示光标
            if (cursor_c >= 0) {
                boxfill8(sheet->buf, sheet->bxsize, cursor_c, cursor_x, cursor_y, cursor_x + 7, cursor_y + 15);
            }
            sheet_refresh(sheet, cursor_x, cursor_y, cursor_x + 8, cursor_y + 16);
        }
    }
}

/*
    将文本框内容向下滚动一行
*/
void scroll_down(struct SHEET *sheet, int x_init, int y_init, int xsize, int ysize) {
    int x, y;
    // 需要向下滚动一行，具体做法：所有像素向上移动一行，再把最下面一行涂黑
    for (y = y_init; y < y_init + ysize; y++) {
        for (x = x_init; x < x_init + xsize; x++) {
            sheet->buf[x + y * sheet->bxsize] = 
                sheet->buf[x + (y + 16) * sheet->bxsize];
        }
    }
    for (y = y_init + ysize - 16; y < y_init + ysize; y++) {
        for (x = x_init; x < x_init + xsize; x++) {
            sheet->buf[x + y * sheet->bxsize] = COL8_000000;
        }
    }
    sheet_refresh(sheet, x_init, y_init, x_init + xsize, y_init + ysize); // 刷新整片文本框
}

/*
    命令行换行，并返回换行后的cursor_y
*/
int cons_newline(int cursor_y, struct SHEET *sheet) {
    if (cursor_y < 28 + 304) {
           // 将光标坐标切换到下一行
           cursor_y += 16; 
    } else {
        scroll_down(sheet, 8, 28, 400, 320);
    }
    
    return cursor_y;
}

void cons_clear(struct SHEET *sheet, int x_init, int y_init, int xsize, int ysize) {
    int x, y;
    for (y = y_init; y < y_init + ysize; y++) {
        for (x = x_init; x < x_init + xsize; x++) {
            sheet->buf[x + y * sheet->bxsize] = COL8_000000;
        }
    }
    sheet_refresh(sheet, x_init, y_init, x_init + xsize, y_init + ysize);
    return;
}

/*
    在console中打印字符串
    cc: character color
    bc: background color
*/
int cons_putstring(struct SHEET *sheet, int cursor_x, int cursor_y, int cc, int bc, char *string, unsigned int start, unsigned int end) {
    unsigned int i;
    char *s = NULL;
    for (i = start; i < end; i++) {
        s[0] = string[i];
        s[1] = 0;
        // 处理制表符、换行回车等
        if (s[0] == 0x09) {
            // 制表符
            for (;;) {
                putstring8_ascii_sht(sheet, cursor_x, cursor_y, cc, bc, " ");
                cursor_x += 8;
                if (cursor_x == 8 + 400) {
                    cursor_x = 8;
                    cursor_y = cons_newline(cursor_y, sheet);
                }
                if (((cursor_x - 8) & 0x1f) == 0) {
                    // 被32整除，即达到4个字符
                    break;
                }
            }
        } else if (s[0] == 0x0a) {
            // 换行符
            cursor_x = 8;
            cursor_y = cons_newline(cursor_y, sheet);
        } else if (s[0] == 0x0d) {
            // 暂时不做操作
        } else {
            // 一般字符
            // 逐个打印字符并换行
            putstring8_ascii_sht(sheet, cursor_x, cursor_y, cc, bc, s);
            cursor_x += 8;
            if (cursor_x == 8 + 400) {
                cursor_y = cons_newline(cursor_y, sheet);
                cursor_x = 8;
            }
        }

    }
    cursor_y = cons_newline(cursor_y, sheet);
    cursor_y = cons_newline(cursor_y, sheet);

    return cursor_y;
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
