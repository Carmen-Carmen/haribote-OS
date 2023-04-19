// console.c
#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include "bootpack.h"

struct CONSOLE cons;

// 命令行窗口任务的入口函数
void console_task(struct SHEET *sheet, unsigned int memtotal, int size_x, int size_y) {
    struct TIMER *timer;
    struct TASK *task = task_now();
    int i, fifobuf[128], 
        cmdindex = 0;   // 命令字符串的索引
    char s[30];
    char cmdline[2048];
    struct MEMMAN *memman = (struct MEMMAN *) MEMMAN_ADDR;

    int *fat = (int *) memman_alloc_4k(memman, 4 * 2880);

    cons.sht = sheet;
    cons.cur_x = 8;
    cons.cur_y = 28;
    cons.cur_c = -1;
    cons.size_x = size_x;
    cons.size_y = size_y;

    *((int *) 0x0fec) = (int) &cons;    // 姑且先把cons结构体的地址放在这边，位于BOOTINFO之前

    fifo32_init(&task->fifo, 128, fifobuf, task); // 有数据写入时，唤醒task_cons
    timer = timer_alloc();
    timer_init(timer, &task->fifo, 1);
    timer_settime(timer, 50);

    file_decodefat(fat, (unsigned char *) (ADR_DISKIMG + 0x000200));  // 将磁盘映像中的fat内容解压，并载入fat指针

    // sprintf(s, "size_x: %d, size_y: %d.", size_x, size_y);
    // cons_putstring(&cons, s, COL8_FFFFFF, COL8_000000, 0, strlen(s));
    // cons_newline(&cons);
    sprintf(s, "power-on time: %s.", get_ontime());
    cons_putstring(&cons, s, -1);
    cons_newline(&cons);
    // 显示提示符 $ 
    cons_putstring(&cons, "$ ", COL8_00FFFF, -1);

    cons.multi_line = 0; // 记录当前一行是否为多行的flag

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
                    if (cons.cur_c >= 0) {
                        cons.cur_c = COL8_FFFFFF;
                    }
                } else {
                    timer_init(timer, &task->fifo, 1);
                    if (cons.cur_c >= 0) {
                        cons.cur_c = COL8_000000;
                    }
                }
                timer_settime(timer, 50);
            } else if (i == 2) {
                // 开始光标闪烁
                cons.cur_c = COL8_000000;
            } else if (i == 3) {
                // 停止光标闪烁
                cons.cur_c = -1;
                boxfill8(sheet->buf, sheet->bxsize, COL8_000000, cons.cur_x, cons.cur_y, cons.cur_x + 7, cons.cur_y + 15);   // 用黑色将光标隐去
            } else if (256 <= i && i <= 511) {
                // 证明是task_a传来的键盘数据
                i -= 256;

                if (i == 8) {           
                // 退格键
                    cons.cur_c = COL8_FFFFFF;
                    if (cmdindex > 0) {
                        cmdline[cmdindex--] = 0;
                    }

                    cons_backspace(&cons, cmdline, cmdindex);
                } else if (i == 10) {   
                // 回车键
                    cons.multi_line = 0;
                    // 删除当前位置的光标
                    putstring8_ascii_sht(sheet, cons.cur_x, cons.cur_y, COL8_FFFFFF, COL8_000000, " ");
                    cmdline[cmdindex] = 0;
                    cons_newline(&cons);
                    cons_runcmd(cmdline, &cons, fat, memtotal); // 执行命令
                    cons_putstring(&cons, "$ ", COL8_00FFFF, -1);
                    cmdindex = 0;
                } else {                
                // 一般字符
                    s[0] = i;
                    s[1] = 0;

                    if (cmdindex > 2048) continue;
                    cmdline[cmdindex++] = i;  

                    putstring8_ascii_sht(cons.sht, cons.cur_x, cons.cur_y, COL8_FFFFFF, COL8_000000, s);
                    cons.cur_x += 8;
                    if (cons.cur_x == 8 + size_x) {
                        cons.multi_line++;
                        cons_newline(&cons);
                    }
                }
            }
            // 重新显示光标
            if (cons.cur_c >= 0) {
                boxfill8(sheet->buf, sheet->bxsize, cons.cur_c, cons.cur_x, cons.cur_y, cons.cur_x + 7, cons.cur_y + 15);
            }
            sheet_refresh(sheet, cons.cur_x, cons.cur_y, cons.cur_x + 8, cons.cur_y + 16);
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
    return;
}

/*
    命令行换行
*/
void cons_newline(struct CONSOLE *cons) {
    if (cons->cur_y < (28 + cons->size_y - 16)) {
        // 将光标坐标切换到下一行
        cons->cur_y += 16; 
    } else {
        scroll_down(cons->sht, 8, 28, cons->size_x, cons->size_y);
    }
        cons->cur_x = 8;
    return;
}

/*
    cc: character color
    bc: background color
*/
void cons_putchar(struct CONSOLE *cons, int chr, char move, int cc, int bc) {
    char s[2];
    s[0] = chr;
    s[1] = 0;
    
    // 处理制表符、换行回车等
        if (s[0] == 0x09) {
            // 制表符
            for (;;) {
                putstring8_ascii_sht(cons->sht, cons->cur_x, cons->cur_y, cc, bc, " ");
                cons->cur_x += 8;
                if (cons->cur_x == 8 + cons->size_x) {
                    cons_newline(cons);
                }
                if (((cons->cur_x - 8) & 0x1f) == 0) {
                    // 被32整除，即达到4个字符
                    break;
                }
            }
        } else if (s[0] == 0x0a) {
            // 换行符
            cons_newline(cons);
        } else if (s[0] == 0x0d) {
            // 暂时不做操作
        } else {
            // 一般字符
            // 逐个打印字符并换行
            putstring8_ascii_sht(cons->sht, cons->cur_x, cons->cur_y, cc, bc, s);
            if (move != 0) {
                // move为0时光标不后移
                cons->cur_x += 8;
                if (cons->cur_x == 8 + cons->size_x) {
                    cons_newline(cons);
                }
            }
        }
    return;
}

/*
    在console中打印字符串
    可变参数:
        int cc: character color
        int bc: background color
        unsignen int start
        unsigned int end
*/
void cons_putstring(struct CONSOLE *cons, char *string, ...) {
    unsigned int i;
    
    // 默认值
    int cc = COL8_FFFFFF, 
        bc = COL8_000000;
    unsigned int start = 0;
    unsigned int end = strlen(string);

    va_list p_args;
    va_start(p_args, string);    // 一共4个可变参数
    int val;
    
    // 处理可变参数这么恶心的嘛= =
    val = va_arg(p_args, int);
    if (val != -1)
        cc = val;
    else 
        goto start_print;

    val = va_arg(p_args, int);
    if (val != -1)
        bc = val;
    else 
        goto start_print;

    val = va_arg(p_args, unsigned int);
    if (val != -1)
        start = val;
    else 
        goto start_print;
        
    val = va_arg(p_args, unsigned int);
    if (val != -1)
        end = val;
    else 
        goto start_print;
    
start_print:
    for (i = start; i < end; i++) {
        cons_putchar(cons, string[i], 1, cc, bc);
    }
    return;
}

/*
    处理退格键
*/
void cons_backspace(struct CONSOLE *cons, char *cmdline, int cmdindex) {
    struct SHEET *sheet = cons->sht;
    char s[128];
    if (cons->multi_line == 0) {      
        // 只有一行
        if (cons->cur_x > 24) {    // 大于24是为了不要把命令行提示符也给删掉
            putstring8_ascii_sht(sheet, cons->cur_x, cons->cur_y, COL8_FFFFFF, COL8_000000, " ");
            cons->cur_x -= 8;
        }
    } else if (cons->cur_y <= 28) {
        // cons.cur_y == 28 && cons.multi_line != 0; 这种情况是输入多行内容后滚动导致的
        if (cons->cur_x > 8) {     // 在当前行内退格
            putstring8_ascii_sht(sheet, cons->cur_x, cons->cur_y, COL8_FFFFFF, COL8_000000, " ");
            cons->cur_x -= 8;
        } else {                // 退格至前一行
            /*
                从文字缓冲区中取出被滚动走的之前的属于这一行的内容，并且显示出来
                通过cons.multi_line的行数来计算出应该显示哪些，并显示在第一行
            */
            cons->multi_line--;
            int s_index;
            if (cons->multi_line != 0) {
                for (s_index = (cons->size_x - 16) / 8 + (cons->multi_line - 1) * cons->size_x / 8; s_index <= cmdindex; s_index++) {
                    s[s_index - ((cons->size_x - 16) / 8 + (cons->multi_line - 1) * cons->size_x / 8)] = cmdline[s_index];
                }
                s[cons->size_x / 8] = 0;
                putstring8_ascii_sht(sheet, 8, 28, COL8_FFFFFF, COL8_000000, s);
            } else {
                for (s_index = 0; s_index <= cmdindex; s_index++) {
                    s[s_index] = cmdline[s_index];
                }
                s[(cons->size_x - 16) / 8] = 0;
                putstring8_ascii_sht(sheet, 8, 28, COL8_00FFFF, COL8_000000, "$ ");
                putstring8_ascii_sht(sheet, 24, 28, COL8_FFFFFF, COL8_000000, s);
            }
            cons->cur_x = cons->size_x;
            cons->cur_c = -1;
        }
    } else {                    
        // 存在多行
        if (cons->cur_x > 8) {     // 在当前行内退格
            putstring8_ascii_sht(sheet, cons->cur_x, cons->cur_y, COL8_FFFFFF, COL8_000000, " ");
            cons->cur_x -= 8;
        } else {                // 退格至前一行
            cons->multi_line--;
            // 删除当前位置的光标
            putstring8_ascii_sht(sheet, cons->cur_x, cons->cur_y, COL8_FFFFFF, COL8_000000, " ");
            cons->cur_x = cons->size_x;
            cons->cur_y -= 16;
        }
    }

    return;
}


void cons_runcmd(char *cmdline, struct CONSOLE *cons, int *fat, unsigned int memtotal) {
// 执行命令
    if (strcmp(cmdline, "mem") == 0) {
        // "mem" command
        cmd_mem(cons, memtotal);
        cons_newline(cons);
    } else if (strcmp(cmdline, "clear") == 0) {
        // "clear" command
        cmd_clear(cons);
    } else if (strcmp(cmdline, "ls") == 0) {
        // "ls" command
        cmd_ls(cons);
        cons_newline(cons);
    } else if (strncmp(cmdline, "cat ", 4) == 0 || strcmp(cmdline, "cat") == 0) { 
        // "cat" command
        cmd_cat(cons, fat, cmdline);
        cons_newline(cons);
    }  else if (strcmp(cmdline, "fat") == 0){
        // "fat" command
        cmd_fat(cons, fat);
        cons_newline(cons);
    } else if (strcmp(cmdline, "time") == 0) {
        // "time" command
        cmd_time(cons);
        cons_newline(cons);
    } else if (cmdline[0] != 0) {
        if (cmd_app(cons, fat, cmdline) == 0) { // 在调用函数的同时获取返回值
            // 既不是命令，也不是应用程序，也不是空行
            cons->cur_x = 8;
            cons_putstring(cons, "[command not found]: ", COL8_FFFFFF, COL8_840000, -1);
            // 拼接[command not found]: 后面的内容
            cons_putstring(cons, cmdline, COL8_FFFFFF, COL8_840000, -1);
            cons_newline(cons);
        }
        cons_newline(cons);
    }
    return;
}

void cmd_mem(struct CONSOLE *cons, unsigned int memtotal) {
    struct MEMMAN *memman = (struct MEMMAN *) MEMMAN_ADDR;
    char s[30];
    sprintf(s, "total   %dMB", memtotal / (1024 * 1024));
    cons_putstring(cons, s, -1);
    cons_newline(cons);
    sprintf(s, "free %dKB", memman_total(memman) / 1024);
    cons_putstring(cons, s, -1);
    cons_newline(cons);
}

void cmd_clear(struct CONSOLE *cons) {
    int x, y;
    struct SHEET *sheet = cons->sht;
    for (y = 28; y < 28 + cons->size_y; y++) {
        for (x = 8; x < 8 + cons->size_x; x++) {
            sheet->buf[x + y * sheet->bxsize] = COL8_000000;
        }
    }
    sheet_refresh(sheet, 8, 28, 8 + cons->size_x, 28 + cons->size_y);
    cons->cur_y = 28;
    return;
}

void cmd_ls(struct CONSOLE *cons) {
    struct FILEINFO *finfo = (struct FILEINFO *) (ADR_DISKIMG + 0x002600);
    int index_file, index_name;
    char s[30];
    for (index_file = 0; index_file < 224; index_file++) { // 最多有224个文件
            // 利用FINFO类型指针，从内存0x00102600开始按32个Byte读取
            if (finfo[index_file].name[0] == 0x00) {
                sprintf(s, "total: %d", index_file);
                cons_putstring(cons, s, -1);
                cons_newline(cons);
                // 直到，读到空文件，退出循环
                break;
            }
            if (finfo[index_file].name[0] != 0xe5) {
                // 文件名开头不是0xe5，是正常的文件
                if ((finfo[index_file].type & 0x18) == 0) {
                    // 既不是非文件（0x08）也不是目录（0x10），就要读出来
                    // 暂时只显示文件名为8个字符以内的
                    sprintf(s, "filename.ext    %7d", finfo[index_file].size);
                    for (index_name = 0; index_name < 8; index_name++) {
                        s[index_name] = finfo[index_file].name[index_name];
                    }
                    // 拼接扩展名
                    s[ 9] = finfo[index_file].ext[0];
                    s[10] = finfo[index_file].ext[1];
                    s[11] = finfo[index_file].ext[2];
                    cons_putstring(cons, s, -1);
                    cons_newline(cons);
                }
            }
        }
        return;
}

void cmd_cat(struct CONSOLE *cons, int *fat, char *cmdline) {
    if (strcmp(cmdline, "cat") == 0) {
        cons->cur_x = 8;
        cons_putstring(cons, "usage: cat filename.ext", -1);
        cons_newline(cons);
        return;
    }

    struct MEMMAN *memman = (struct MEMMAN *) MEMMAN_ADDR;
    struct FILEINFO *finfo = file_search(cmdline + 4, (struct FILEINFO *) (ADR_DISKIMG + 0x002600), 224);
    char *p_disk;
    int i;

    if (finfo != 0) {
        // 找到文件了
        p_disk = (char *) memman_alloc_4k(memman, finfo->size);
        file_loadfile(finfo->clustno, finfo->size, p_disk, fat, (char *) (ADR_DISKIMG + 0x003e00));
        for (i = 0; i < finfo->size; i++) {
            cons_putchar(cons, p_disk[i], 1, COL8_FFFFFF, COL8_000000);
        }
        memman_free_4k(memman, (int) p_disk, finfo->size);
        cons_newline(cons);
    } else {
        // 没找到
        cons->cur_x = 8;
        cons_putstring(cons, "[file not found]: ", COL8_FFFFFF, COL8_840000, -1);
        // 拼接[command not found]: 后面的内容
        cons_putstring(cons, cmdline, COL8_FFFFFF, COL8_840000, 4, -1);
        cons_newline(cons);
    }
    return;
}

void cmd_fat(struct CONSOLE *cons, int *fat) {
    char s[3];
    int fat_index;

    cons_putstring(cons, "[FAT infomation]: \n", -1);
    for (fat_index = 0; fat_index < 2880; fat_index++) {
        if (fat[fat_index] == 0) {
            break;
        }

        sprintf(s, "%03x", fat[fat_index]);
        if (fat[fat_index] == 0xfff) {
            // 标记一下文件末尾
            cons_putstring(cons, s, COL8_00FF00, COL8_000000, -1);
        } else {
            cons_putstring(cons, s, COL8_FFFFFF, COL8_000000, -1);
        }
        if ((fat_index + 1) % 8 == 0) {
            cons_putstring(cons, "\n", COL8_FFFFFF, COL8_000000, -1);
        } else {
            cons_putstring(cons, " ", COL8_FFFFFF, COL8_000000, -1);
        }
    }
    cons_newline(cons);

    return;
}

void cmd_time(struct CONSOLE *cons) {
    char *s;
    s = get_ontime();
    cons_putstring(cons, "power-on time: ", -1);
    cons_putstring(cons, s, -1);
    cons_newline(cons);
    return;
}

int cmd_app(struct CONSOLE *cons, int *fat, char *cmdline) {
    struct MEMMAN *memman = (struct MEMMAN *) MEMMAN_ADDR;
    char *p_disk;
    char name[18];
    struct FILEINFO *finfo;
    struct SEGMENT_DESCRIPTOR *gdt = (struct SEGMENT_DESCRIPTOR *) ADR_GDT;
    int i;

    for (i = 0; i < 13; i++) {
        if (cmdline[i] <= ' ') {
            break;
        }
        name[i] = cmdline[i];
    }
    name[i] = 0x00;

    // 希望无论是输入"hlt"还是"hlt.hrb"都可以找到文件并执行
    finfo = file_search(name, (struct FILEINFO *) (ADR_DISKIMG + 0x002600), 224);
    if (finfo == 0 && name[i - 1] != '.') {
        // 找不到文件，在文件名后面加上".hrb"后继续寻找
        name[i    ] = '.';
        name[i + 1] = 'h';
        name[i + 2] = 'r';
        name[i + 3] = 'b';
        name[i + 4] = 0x00;
        finfo = file_search(name, (struct FILEINFO *) (ADR_DISKIMG + 0x002600), 224);
    }
    if (finfo != 0) {
        p_disk = (char *) memman_alloc_4k(memman, finfo->size);
        file_loadfile(finfo->clustno, finfo->size, p_disk, fat, (char *) (ADR_DISKIMG + 0x003e00));
        set_segmdesc(gdt + 1003, finfo->size - 1, (int) p_disk, AR_CODE32_ER);
        farcall(0, 1003 * 8);   // 改用farcall来调用hlt
        memman_free_4k(memman, (int) p_disk, finfo->size);
        cons_newline(cons);
        return 1;
    } else {
        // 没找到
        // cons->cur_x = 8;
        // cons_putstring(cons, "reason: [file not found]: ", COL8_FFFFFF, COL8_840000, 0, strlen("reason: [file not found]: "));
        // // 拼接[command not found]: 后面的内容
        // cons_putstring(cons, "hlt.hrb", COL8_FFFFFF, COL8_840000, 0, strlen(name));
        // cons_newline(cons);
        return 0;
    }
}
