// console.c
#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include "bootpack.h"

// 命令行窗口任务的入口函数
void console_task(struct SHEET *sheet, unsigned int memtotal, int size_x, int size_y) {
    struct TASK *task = task_now();
    int i, 
        cmdindex = 0;   // 命令字符串的索引
    char s[30];
    char cmdline[256];
    struct MEMMAN *memman = (struct MEMMAN *) MEMMAN_ADDR;
    struct CONSOLE cons;
    struct FILEHANDLE fhandle[8];

    // 使用二维数组，分配给它256 * 2048个Byte
    char **cmdlines = 0;// 最好给个初始值0，不然之后cmd_exit的时候传入的不是0，则free的时候会错误释放内存

    // ncst启动应用程序不需要cmdlines
    if (sheet != 0) {
        cmdlines = (char **)memman_alloc(memman, sizeof(char*) * 2048);  // 之前内存都申请多了啊
        // 注意还需要对每一个cmdlines[i]分配内存，进行初始化
        for (i = 0; i < 2048; i++) {
            cmdlines[i] = (char *)memman_alloc(memman, 256);
        }
    }
    int currentcmd = 0; // 当前执行的cmd，以及cmdliens中cmd的总数
    int cmd_updown = 0; // 上下方向键定位到哪个cmd

    int *fat = (int *) memman_alloc_4k(memman, 4 * 2880);
    cons.sht = sheet;
    cons.cur_x = 8;
    cons.cur_y = 28;
    cons.cur_c = -1;
    cons.size_x = size_x;
    cons.size_y = size_y;

    task->cons = &cons;     // 命令行窗口的任务要保存自己CONSOLE结构的地址

    if (sheet != 0) {
        cons.timer = timer_alloc();
        timer_init(cons.timer, &task->fifo, 1);
        timer_settime(cons.timer, 50);
    }

    file_decodefat(fat, (unsigned char *) (ADR_DISKIMG + 0x000200));  // 将磁盘映像中的fat内容解压，并载入fat指针
    for (i = 0; i < 8; i++) {
	fhandle[i].buf = 0;
    }
    task->fhandle = fhandle;
    task->fat = fat;


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

            if (i <= 1 && cons.sht != 0) {
                // data = 1，说明是光标定时器
                if (i != 0) {
                    timer_init(cons.timer, &task->fifo, 0);
                    if (cons.cur_c >= 0) {
                        cons.cur_c = COL8_FFFFFF;
                    }
                } else {
                    timer_init(cons.timer, &task->fifo, 1);
                    if (cons.cur_c >= 0) {
                        cons.cur_c = COL8_000000;
                    }
                }
                timer_settime(cons.timer, 50);
            } else if (i == 2) {
                // 开始光标闪烁
                cons.cur_c = COL8_000000;
            } else if (i == 3) {
                // 停止光标闪烁
                cons.cur_c = -1;
                if (cons.sht != 0) {
                    boxfill8(cons.sht->buf, cons.sht->bxsize, COL8_000000, cons.cur_x, cons.cur_y, cons.cur_x + 7, cons.cur_y + 15);   // 用黑色将光标隐去
                }
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
                    putstring8_ascii_sht(cons.sht, cons.cur_x, cons.cur_y, COL8_FFFFFF, COL8_000000, " ");
                    cmdline[cmdindex] = 0;

                    if (currentcmd == 2047) continue;
                    if (cmdindex != 0 && cons.sht != 0) {
                        strcpy(cmdlines[currentcmd++], cmdline);
                    }
                    
                    cons_newline(&cons);
                    cons_runcmd(cmdline, &cons, fat, memtotal, cmdlines, currentcmd); // 执行命令
                    if (cons.sht == 0) {
                        cmd_exit(&cons, fat, cmdlines);
                    }
                    cons_putstring(&cons, "$ ", COL8_00FFFF, -1);
                    cmdindex = 0;
                    cmd_updown = 0;
                } else if (i == KBD_UP) {
                // 上方向键
                    cmd_updown--;
                    if (currentcmd + cmd_updown >= 0) {
                        // 先把当前cmdline所占的位置消除掉
                        for (i = 0; i < cmdindex; i++) {
                            cons_backspace(&cons, cmdline, cmdindex);
                        }
                        // 再从cmdlines中找出
                        strcpy(cmdline, cmdlines[currentcmd + cmd_updown]);
                        cmdindex = strlen(cmdlines[currentcmd + cmd_updown]);
                        cons.cur_x = 24;
                        cons_putstring(&cons, cmdline, COL8_FFFFFF, COL8_000000, 0, cmdindex);
                    } else {
                        cmd_updown++;
                    }
                } else if (i == KBD_DOWN) {
                // 下方向键
                    cmd_updown++;
                    if (currentcmd + cmd_updown < currentcmd) {
                        // 先把当前cmdline所占的位置消除掉
                        for (i = 0; i < cmdindex; i++) {
                            cons_backspace(&cons, cmdline, cmdindex);
                        }
                        // 再从cmdlines中找出
                        strcpy(cmdline, cmdlines[currentcmd + cmd_updown]);
                        cmdindex = strlen(cmdlines[currentcmd + cmd_updown]);
                        cons.cur_x = 24;
                        cons_putstring(&cons, cmdline, COL8_FFFFFF, COL8_000000, 0, cmdindex);
                    } else {
                        cmd_updown--;
                    }
                } else if (i == KBD_LEFT || i == KBD_RIGHT) {
                // 左右方向键，先什么也不做
                    continue;
                } else {                
                // 一般字符
                    s[0] = i;
                    s[1] = 0;

                    if (cmdindex > 256) continue;
                    cmdline[cmdindex++] = i;  
                    
                    if (cons.sht != 0) {
                        putstring8_ascii_sht(cons.sht, cons.cur_x, cons.cur_y, COL8_FFFFFF, COL8_000000, s);
                    }
                    cons.cur_x += 8;
                    if (cons.cur_x == 8 + size_x) {
                        cons.multi_line++;
                        cons_newline(&cons);
                    }
                    if (cons.multi_line > cons.size_y / 16 - 1) {
                        cons_omit_firstline(&cons);
                    }
                }
            } else if (i == 4) {
                // 接收到来自task_a的数据 4 ，就执行exit相关程序
                cmd_exit(&cons, fat, cmdlines);
            }
            // 重新显示光标
            if (cons.sht != 0) {
                if (cons.cur_c >= 0) {
                    boxfill8(cons.sht->buf, cons.sht->bxsize, cons.cur_c, cons.cur_x, cons.cur_y, cons.cur_x + 7, cons.cur_y + 15);
                }
                sheet_refresh(cons.sht, cons.cur_x, cons.cur_y, cons.cur_x + 8, cons.cur_y + 16);
            }
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

// 当输入内容超过窗口高度时，将首行省略为"$ >.... "
void cons_omit_firstline(struct CONSOLE *cons) {
    putstring8_ascii_sht(cons->sht, 8, 28, COL8_00FFFF, COL8_000000, "$ ");
    putstring8_ascii_sht(cons->sht, 24, 28, COL8_FFFFFF, COL8_000000, ">.... ");
    int space_count = cons->size_x / 8 - strlen("$ >.... ");
    int space_index;
    for (space_index = 0; space_index < space_count; space_index++) {
        putstring8_ascii_sht(cons->sht, 8 + (strlen("$ >.... ") + space_index) * 8, 28, COL8_FFFFFF, COL8_000000, " ");
    }
    return;
}

/*
    命令行换行
*/
void cons_newline(struct CONSOLE *cons) {
    if (cons->sht == 0)
        return;

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
    if (cons->sht == 0)
        return;
        
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
    以下是书上提供的2个用于打印字符串的函数
    putstr0: 打印字符串，遇到字符编码0则结束
    putstr1: 打印制定长度的字符串
*/
void cons_putstr0(struct CONSOLE *cons, char *s) {
    for (; *s != 0x00; s++) {
        cons_putchar(cons, *s, 1, COL8_FFFFFF, COL8_000000);
    }
    return;
}
void cons_putstr1(struct CONSOLE *cons, char *s, int len) {
    int i;
    for (i = 0; i < len; i++) {
        cons_putchar(cons, s[i], 1, COL8_FFFFFF, COL8_000000);
    }
    return;
}

/*
    处理退格键
*/
void cons_backspace(struct CONSOLE *cons, char *cmdline, int cmdindex) {
    if (cons->sht == 0)
        return;
        
    int count_per_line = cons->size_x / 8;
    int count_line = cons->size_y / 16;
    struct SHEET *sheet = cons->sht;
    int start_index;        // 从cmdline中，start_index开始读取

    if (cons->multi_line == 0) {      
        // 只有一行
        if (cons->cur_x > 24) {    // 大于24是为了不要把命令行提示符也给删掉
            putstring8_ascii_sht(sheet, cons->cur_x, cons->cur_y, COL8_FFFFFF, COL8_000000, " ");
            cons->cur_x -= 8;
        }
    } else {
        // 存在多行
        if (cons->multi_line > count_line) {
            // 行数大于窗口能显示的行数，第一行还是要显示"$ >.... "
            if (cons->cur_x > 8) {  // 在当前行内退格
                putstring8_ascii_sht(sheet, cons->cur_x, cons->cur_y, COL8_FFFFFF, COL8_000000, " ");
                cons->cur_x -= 8;
            } else {                // 退格至前一行
                cons_omit_firstline(cons);

                putstring8_ascii_sht(sheet, cons->cur_x, cons->cur_y, COL8_FFFFFF, COL8_000000, " ");
                // 根据窗口长宽，找到在窗口中要显示的第一个字符
                start_index =   count_per_line - 2 +  // 第一行
                                    (cons->multi_line - count_line) * count_per_line;   // 其他未显示的行
                // 从命令行第二行开头开始，打印出从cmdline中从start_index开始的所有字符
                cons->cur_x = 8;
                cons->cur_y = 28 + 16;
                cons->cur_c = -1;
                cons_putstring(cons, cmdline, COL8_FFFFFF, COL8_000000, start_index, cmdindex);
                cons->cur_x = cons->size_x;
                cons->cur_y = 28 + cons->size_y - 16;
                cons->cur_c = COL8_000000;
                cons->multi_line--;
            }
        } else {
            // 行数小于窗口能显示的行数
            if (cons->multi_line == count_line) {
                // 1. 是从超出窗口高度的地方退格回来
                if (cons->cur_x > 8) {  // 在当前行内退格
                    putstring8_ascii_sht(sheet, cons->cur_x, cons->cur_y, COL8_FFFFFF, COL8_000000, " ");
                    cons->cur_x -= 8;
                } else {                // 退格至前一行
                    // 把之前的内容从第一行开始打印
                    putstring8_ascii_sht(sheet, 8, 28, COL8_00FFFF, COL8_000000, "$ ");
                    putstring8_ascii_sht(sheet, cons->cur_x, cons->cur_y, COL8_FFFFFF, COL8_000000, " ");
                    start_index = 0;
                    cons->cur_x = 8 + 8 * 2;    // 从第一行的"$ "之后开始
                    cons->cur_y = 28;
                    cons->cur_c = -1;
                    cons_putstring(cons, cmdline, COL8_FFFFFF, COL8_000000, start_index, cmdindex);
                    cons->cur_x = cons->size_x;
                    cons->cur_y = 28 + cons->size_y - 16 * (count_line - cons->multi_line + 1);
                    cons->cur_c = COL8_000000;
                    cons->multi_line--;
                }
            } else {
                // 2. 普通的多行退格
                if (cons->cur_x > 8) {  // 在当前行内退格
                    putstring8_ascii_sht(sheet, cons->cur_x, cons->cur_y, COL8_FFFFFF, COL8_000000, " ");
                    cons->cur_x -= 8;
                } else {                // 退格至前一行
                    putstring8_ascii_sht(sheet, cons->cur_x, cons->cur_y, COL8_FFFFFF, COL8_000000, " ");
                    cons->cur_x = cons->size_x;
                    cons->cur_y -= 16;
                    cons->multi_line--;
                }
            }
        }
    }

    return;
}


void cons_runcmd(char *cmdline, struct CONSOLE *cons, int *fat, unsigned int memtotal, char **cmdlines, int currentcmd) {
// 执行命令
    cmdline = trim_cmdline_space(cmdline);
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
    } else if (strncmp(cmdline, "cat", 3) == 0) { 
        // "cat" command
        cmdline = remove_cmdline_middle_space(cmdline, "cat");
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
    } else if (strncmp(cmdline, "echo", 4) == 0) {
        // "echo" command
        cmdline = remove_cmdline_middle_space(cmdline, "echo");
        cmd_echo(cons, cmdline);
        cons_newline(cons);
    } else if (strcmp(cmdline, "top") == 0) {
        // "top" command
        cmd_top(cons);
        cons_newline(cons);
    } else if (strcmp(cmdline, "history") == 0) {
        // "history" command
        cmd_history(cons, cmdlines, currentcmd);
        cons_newline(cons);
    } else if (strcmp(cmdline, "exit") == 0) {
        // "exit" command
        cmd_exit(cons, fat, cmdlines);
    } else if (strncmp(cmdline, "start", 5) == 0) {
        // "start" command
        cmdline = remove_cmdline_middle_space(cmdline, "start");
        cmd_start(cons, cmdline, memtotal);
        cons_newline(cons);
    } else if (strncmp(cmdline, "ncst", 4) == 0) {
        // "ncst" command no console start
        cmdline = remove_cmdline_middle_space(cmdline, "ncst");
        cmd_ncst(cons, cmdline, memtotal);
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

/*
    cat的用法: cat filename.extension
*/
void cmd_cat(struct CONSOLE *cons, int *fat, char *cmdline) {
    if (strcmp(cmdline, "cat ") == 0) {
        cons->cur_x = 8;
        cons_putstring(cons, "usage: cat filename.ext", -1);
        cons_newline(cons);
        return;
    }

    struct MEMMAN *memman = (struct MEMMAN *) MEMMAN_ADDR;
    struct FILEINFO *finfo = file_search(cmdline + 4, (struct FILEINFO *) (ADR_DISKIMG + 0x002600), 224);
    char *p;

    if (finfo != 0) {
        // 找到文件了
        p = (char *) memman_alloc_4k(memman, finfo->size);
        file_loadfile(finfo->clustno, finfo->size, p, fat, (char *) (ADR_DISKIMG + 0x003e00));
        cons_putstring(cons, p, COL8_FFFFFF, COL8_000000, 0, finfo->size);
        memman_free_4k(memman, (int) p, finfo->size);
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
    char *p, *q;  // p, q是指向磁盘缓冲区的俩指针
    char name[18];
    struct FILEINFO *finfo;
    struct SEGMENT_DESCRIPTOR *gdt = (struct SEGMENT_DESCRIPTOR *) ADR_GDT;
    int i;
    struct TASK *task = task_now();
    int segsize,            // 数据段预留大小
        datsize,            // 数据大小
        esp,                // ESP初始值&数据部分传送目的地址
        dathrb;             // hrb文件内部数据部分从哪里开始
    struct SHTCTL *shtctl;
    struct SHEET *sht;

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
        p = (char *) memman_alloc_4k(memman, finfo->size);
        file_loadfile(finfo->clustno, finfo->size, p, fat, (char *) (ADR_DISKIMG + 0x003e00));
        if (finfo->size >= 36 && strncmp(p + 4, "Hari", 4) == 0 && *p == 0x00) {
            // hrb可执行文件是0x00开头的，且第0x0004开始的4个字节为"Hari"
            segsize = *((int *) (p + 0x0000));
            esp     = *((int *) (p + 0x000c));
            datsize = *((int *) (p + 0x0010));
            dathrb  = *((int *) (p + 0x0014));
            q = (char *) memman_alloc_4k(memman, segsize);   
            task->ds_base = (int) q;    // 将q指向的内存地址存放在task中的ds_base中
            // 将代码段和数据段都指定为LDT的段号
            set_segmdesc(task->ldt + 0, finfo->size - 1,   (int) p, AR_CODE32_ER + 0x60); // 将访问权限加上0x60就将该段设置为应用程序用
            set_segmdesc(task->ldt + 1, segsize - 1,       (int) q, AR_DATA32_RW + 0x60); // 数据段
            // 将数据部分传送到esp
            for (i = 0; i < datsize; i++) {
                q[esp + i] = p[dathrb + i];
            }
            start_app(  0x1b,           // eip指向0x1b，从这条指令开始执行
                        0 * 8 + 4,      // 代码段cs
                        esp,            // esp初始值
                        1 * 8 + 4,      // 数据段ds
                        &(task->tss.esp0));
            // 应用程序调用api_end()或被强制结束后返回此处
            // 在这里查询是否有启动应用程序后未关闭的窗口（如未设置closewin或强制结束的情况）
            shtctl = (struct SHTCTL *) *((int *) 0x0fe4);
            for (i = 0; i < MAX_SHEETS; i++) {
                sht = &(shtctl->sheets0[i]);
                if ((sht->flags & 0x11) == 0x11 && sht->task == task) {
                    // sht->task = cons_task即是hrb_api中打开的窗口未关闭
                    sheet_free(sht);        // 将其关闭
                }
            }
	    // 关闭未被应用程序关闭的文件
	    for (i = 0; i < 8; i++) {
		if (task->fhandle[i].buf != 0) {
		    memman_free_4k(memman, (int) task->fhandle[i].buf, task->fhandle[i].size);
		    task->fhandle[i].buf = 0;
		}
	    }
            // 查询是否有需要自动取消的timer，并将它取消
            timer_cancelall(&task->fifo);

            memman_free_4k(memman, (int) q, segsize);       // 在这里释放数据段内存
        } else {
            // 文件不是hrb可执行文件，进行提示
            cons_putstring(cons, "[not an executable file]: ", COL8_FFFFFF, COL8_840000, -1);
            cons_putstring(cons, cmdline, COL8_FFFFFF, COL8_840000, -1);
        }

        memman_free_4k(memman, (int) p, finfo->size);  // 再释放代码段内存
        cons_newline(cons);
        return 1;
    } else {
        // 没找到
        return 0;
    }
}

/*
    echo的用法: echo "内容"
*/
void cmd_echo(struct CONSOLE *cons, char *cmdline) {
    if (strcmp(cmdline, "echo ") == 0) {
        cons_putstring(cons, "usage: echo \"content\"", -1);
        cons_newline(cons);
        return;
    }

    int i, j, start = 0, end = 0, 
        len = strlen(cmdline),
        quot_found = 0;
    // 遍历整个cmdline，找到引号中内容的索引
    // 顺便处理下制表符等
    for (i = 0; i < len; i++) {
        if (cmdline[i] == '\"') {
            if (quot_found == 0) {
                start = i + 1;
                quot_found = 1;
            } else {
                end = i;
                quot_found = 2;
            }
        }
        if (cmdline[i] == '\\') {
            switch (cmdline[i + 1]) {
                case 't': 
                    cmdline[i] = 0x09;
                    // 把\之后的所有字符往前移一位
                    for (j = i + 1; j < len; j++) {
                        cmdline[j] = cmdline[j + 1];
                    }
                    len--;
                    break;
                case 'n':
                    cmdline[i] = 0x0a;
                    // 把\之后的所有字符往前移一位
                    for (j = i + 1; j < len; j++) {
                        cmdline[j] = cmdline[j + 1];
                    }
                    len--;
                    break; 
            }
        }
    }
    if (quot_found != 2) {
        cons_putstring(cons, "[syntax error]: \n", COL8_FFFFFF, COL8_840000, -1);
        cons_putstring(cons, "usage: echo \"content\"", -1);
        cons_newline(cons);
        return;
    }

    cons_putstring(cons, cmdline, COL8_FFFFFF, COL8_000000, start, end);
    cons_newline(cons);
    return;
}

void cmd_top(struct CONSOLE *cons) {
    char s[128];
    int i;
    struct TASK *task;
    // 要查看tasks0数据了，不允许被修改
    io_cli();
    for (i = 0; i < MAX_TASKS; i++) {
        task = &taskctl->tasks0[i];
        if (task->flags != 0) {
            sprintf(s, "task[%d], level[%02d], priority[%2d], sel[%d]\n", 
                        i, task->level, task->priority, task->sel);
            cons_putstring(cons, s, -1);
        }
    }
    io_sti();

    return;
}

void cmd_history(struct CONSOLE *cons, char **cmdlines, int currentcmd) {
    if (cmdlines == 0)
        return;

    int i;
    char s[512];
    for (i = 0; i < currentcmd; i++) {
        sprintf(s, "[%4d] %s", i + 1, cmdlines[i]);
        cons_putstring(cons, s, -1);
        cons_newline(cons);
    }
    return;
}

void cmd_exit(struct CONSOLE *cons, int *fat, char **cmdlines) {
    struct MEMMAN *memman = (struct MEMMAN *) MEMMAN_ADDR;
    struct TASK *task = task_now();
    struct SHTCTL *shtctl = (struct SHTCTL *) *((int *) 0x0fe4);
    struct FIFO32 *fifo = (struct FIFO32 *) *((int *) 0x0fec);
    timer_cancel(cons->timer);
    memman_free_4k(memman, (int) fat, 4 * 2880);
    int i;
    if (cmdlines != 0) {
        for (i = 0; i < 2048; i++) {
            memman_free(memman, (int) cmdlines[i], 256);
        }
        memman_free(memman, (int) cmdlines, sizeof(char *) * 2048);
    }
    
    // 不能在这里调用close_console，不然后面的程序就无法执行下去了
    // 采用在这里向task_a的fifo发送信号，在task_a中执行关闭console的操作
    io_cli();
    if (cons->sht != 0) {
        fifo32_put(fifo, cons->sht - shtctl->sheets0 + 768);    // 768 ~ 1023, 是命令行窗口的图层编号
    } else {
        fifo32_put(fifo, task - taskctl->tasks0 + 1024);        // 1024 ~ 2023, 是命令行的任务编号
    }
    io_sti();
    for (;;) {
        task_sleep(task);
    }
}

// 在另一个命令行窗口启动应用程序
void cmd_start(struct CONSOLE *cons, char *cmdline, int memtotal) {
    if (strcmp(cmdline, "start ") == 0) {
        cons->cur_x = 8;
        cons_putstring(cons, "usage: start execfile(.hrb)", -1);
        cons_newline(cons);
        return;
    }
    struct SHTCTL *shtctl = (struct SHTCTL *) *((int *) 0x0fe4);
    struct SHEET *sht = open_console(shtctl, memtotal, cons->size_x, cons->size_y);
    struct FIFO32 *fifo = &sht->task->fifo;
    int i;
    sheet_slide(sht, cons->sht->vx0 + 50, cons->sht->vy0 + 50);
    sheet_updown(sht, shtctl->top);
    // 将命令行输入的字符串逐字复制到新的命令行窗口
    for (i = 6; cmdline[i] != 0x00; i++) {
        fifo32_put(fifo, cmdline[i] + 256);
    }
    fifo32_put(fifo, 0x0a + 256);   // 在新的命令行窗口回车
    cons_newline(cons);
    return;
}

// no console start
void cmd_ncst(struct CONSOLE *cons, char *cmdline, int memtotal) {
    if (strcmp(cmdline, "ncst ") == 0) {
        cons->cur_x = 8;
        cons_putstring(cons, "usage: ncst execfile(.hrb)", -1);
        cons_newline(cons);
        return;
    }
    struct TASK *task = open_constask(0, memtotal, 0, 0);
    struct FIFO32 *fifo = &task->fifo;
    int i;
    for (i = 5; cmdline[i] != 0x00; i++) {
        fifo32_put(fifo, cmdline[i] + 256);
    }
    fifo32_put(fifo, 0x0a + 256); // 在新的隐藏的命令行窗口回车
    cons_newline(cons);
    return;
}

/*
    参数中寄存器从右往左的顺序正是PUSHAD中寄存器压栈的顺序
*/
int * hrb_api(int edi, int esi, int ebp, int esp, int ebx, int edx, int ecx, int eax) {
    struct TASK *task = task_now(); // 是cons_task
    int ds_base = task->ds_base;    // 可执行文件的数据段基址
    struct CONSOLE *cons = task->cons;      // 取出cons结构体
    struct SHTCTL *shtctl = (struct SHTCTL *) *((int *) 0x0fe4);    // 取出存在0x0fe4地址的shtctl结构体（在bootpack.c里放的）
    struct SHEET *sht, 
            *cons_sht = task->cons->sht;
    char s[128];
    /*
        reg[0]: EDI 
        reg[1]: ESI
        reg[2]: EBP
        reg[3]: ESP
        reg[4]: EBX
        reg[5]: EDX
        reg[6]: ECX
        reg[7]: EAX
    */
    int *reg = &eax + 1;    // 存放寄存器的值，用于返回给寄存器
    int i;                  // 存放从缓冲区中取出的值
    struct FIFO32 *sys_fifo = (struct FIFO32 *) *((int *) 0x0fec);
    struct FILEINFO *finfo;
    struct FILEHANDLE *fh;
    struct MEMMAN *memman = (struct MEMMAN *) MEMMAN_ADDR;

    if (edx == 1) {
        // print单个字符
        cons_putchar(cons, eax & 0xff, 1, COL8_FFFFFF, COL8_000000);
    } else if (edx == 2) {
        // print字符串，到0x00停止
        cons_putstr0(cons, (char *) ebx + ds_base);     // 使用C语言编写的应用程序转化为hrb后，字符串等变量存储在数据段中，和代码段分开了
        // sprintf(s, "EBX = %08X\n", ebx);
        // cons_putstring(cons, s, COL8_FFFFFF, COL8_840000, -1);
    } else if (edx == 3) {
        // print定长字符串
        cons_putstr1(cons, (char *) ebx + ds_base, ecx);
    } else if (edx == 4) {  
        // 应用程序结束的api，edx = 4时结束
        return &(task->tss.esp0);   // 应用程序结束，返回OS栈地址esp0
    } else if (edx == 5) {  
        // 绘制窗口
        /*
            EDX: 5
            EBX: buf
            ESI: bxsize
            EDI: bysize
            EAX: color_invisible

            return: 窗口句柄（EAX）
        */
        sht = sheet_alloc(shtctl);
        sht->task = task;
        sht->flags |= 0x10; // 因为使用中的sheet flag为1，因此这样之后flags就是0x11，在cmd_cpp中可以识别出是这个api启动的窗口
        sheet_setbuf(sht, (char *) ebx + ds_base, esi, edi, eax);
        make_window8((char *) ebx + ds_base, esi, edi, (char *) ecx + ds_base, 0);
        // 移动到命令行窗口的中央，并且要让窗口显示位置为4的倍数，即 坐标&0xfffffffc
        if (cons->sht != 0) {
            sheet_slide(sht, ((cons_sht->bxsize - esi) / 2 + cons_sht->vx0) & ~3, ((cons_sht->bysize - edi) / 2 + cons_sht->vy0) & ~3); 
        } else {
            // 不然就移动到(50, 50)
            sheet_slide(sht, 50 & ~3, 50 & ~3); 
        }
        sheet_updown(sht, shtctl->top); // 将窗口图层高度指定为最高高度，鼠标会移动到更上层去
        reg[7] = (int) sht;         // eax = (int) sht，将sht地址return给应用程序
    } else if (edx == 6) {
        // 在窗口上显示字符
        /*
            EDX: 6
            EBX: 窗口句柄
            ESI: 从x坐标开始打印
            EDI: 从y坐标开始打印
            EAX: character color
            ECX: strlen
            EBP: *s
        */
        // 以后会默认指定一个比原有地址大1的基数，使后续不刷新，那么在此处就要用与运算来算出真正的sht地址
        sht = (struct SHEET *) (ebx & 0xfffffffe);  
        putstring8_ascii(sht->buf, sht->bxsize, esi, edi, eax, (char *) ebp + ds_base);
        if ((ebx & 1) == 0) {
            // 窗口地址为偶数才刷新
            sheet_refresh(sht, esi, edi, esi + ecx * 8, edi + 16);
        }
    } else if (edx == 7) {
        // 在图层上绘制方块
        /*
            EDX: 7
            EBX: sheet句柄
            EAX: x0
            ECX: y0
            ESI: x1
            EDI: y1
            EBP: color
        */
        // 以后会默认指定一个比原有地址大1的基数，使后续不刷新，那么在此处就要用与运算来算出真正的sht地址
        sht = (struct SHEET *) (ebx & 0xfffffffe);
        boxfill8(sht->buf, sht->bxsize, ebp, eax, ecx, esi, edi);
        if ((ebx & 1) == 0) {
            // 窗口地址为偶数才刷新
            sheet_refresh(sht, eax, ecx, esi + 1, edi + 1);
        }
    } else if (edx == 8) {
        // memman初始化
        /*
            EDX: 8
            EBX: memman addr
            EAX: init addr of memory being managed
            ECS: count of bytes being managed
        */
        memman_init((struct MEMMAN *) (ebx + ds_base));
        ecx &= 0xfffffff0;  // 以16字节为单位取整
        memman_free((struct MEMMAN *) (ebx + ds_base), eax, ecx);
    } else if (edx == 9) {
        // malloc
        /*
            EDX: 9
            EBX: memman addr
            EAX: count of bytes needed
            EAX: addr of memory allocated
        */
        ecx = (ecx + 0x0f) & 0xfffffff0;    // 以16字节为单位取整
        reg[7] = memman_alloc((struct MEMMAN *) (ebx + ds_base), ecx);  // 利用eax返回分配的内存基址
        // 通过malloc分配内存后，mem指令显示的剩余内存并不会减少，这是因为mem_total是根据memman来计算剩余内存的；分配应用程序内存的memman和求算剩余内存的不是同一个结构体
    } else if (edx == 10) {
        // free
        /*
            EDX: 10
            EBX: memman addr
            EAX: memory addr need to be freed
            ECS: count of bytes need to be freed
        */
        ecx = (ecx + 0x0f) & 0xfffffff0;    // 以16字节为单位取整
        memman_free((struct MEMMAN *) (ebx + ds_base), eax, ecx);
    } else if (edx == 11) {
        // draw dot
        /*
            EDX: 11
            EBX: sheet句柄
            ESI: x position
            EDI: y position
            EAX: color
        */
        // 以后会默认指定一个比原有地址大1的基数，使后续不刷新，那么在此处就要用与运算来算出真正的sht地址
        sht = (struct SHEET *) (ebx & 0xfffffffe);
        sht->buf[sht->bxsize * edi + esi] = eax;
        if ((ebx & 1) == 0) {
            // 窗口地址为偶数才刷新
            sheet_refresh(sht, esi, edi, esi + 1, edi + 1);
        }
    } else if (edx == 12) {
        // manually refresh sheet
        /*
            EDX: 12
            EBX: sheet句柄
            EAX: x0
            ECX: y0
            ESI: x1
            EDI: y1
        */
        sht = (struct SHEET *) ebx;
        sheet_refresh(sht, eax, ecx, esi, edi);
    } else if (edx == 13) {
        // draw a line with given (x0, y0) and (x1, y1)
        /*
            EDX: 13
            EBX: sheet句柄
            EAX: x0
            ECX: y0
            ESI: x1
            EDI: y1
            EBP: color
        */
        sht = (struct SHEET *) (ebx & 0xfffffffe);
        hrb_api_linewin(sht, eax, ecx, esi, edi, ebp);
        // 不过还是会刷新一个矩形区域
        if ((ebx & 1) == 0) {
            sheet_refresh(sht, eax, ecx, esi + 1, edi + 1);
        }
    } else if (edx == 14) {
        // close a window
        /*
            EDX: 14
            EBX: sheet句柄
        */
        sheet_free((struct SHEET *) ebx);
    } else if (edx == 15) {
        // 应用程序从键盘获取输入
        /*
            EDX: 15
            EAX: eax = 0 --> 没有键盘输入时返回-1，且cons任务不休眠
                 eax = 1 --> 没有键盘输入时cons任务休眠，直到发生键盘输入（从taskA传入）
                 且EAX返回输入的字符编码
        */
        for (;;) {
            io_cli();
            if (fifo32_status(&task->fifo) == 0) {
                if (eax != 0) {
                    task_sleep(task);
                } else {
                    io_sti();
                    reg[7] = -1;
                    return 0;
                }
            }

            i = fifo32_get(&task->fifo);
            io_sti();
            if (i <= 1) {
                timer_init(cons->timer, &task->fifo, 1);
                timer_settime(cons->timer, 50);
            }
            if (i == 2) {
                cons->cur_c = COL8_FFFFFF;
            }
            if (i == 3) {
                cons->cur_c = -1;
            }
            if (i == 4) {
                timer_cancel(cons->timer);
                io_cli();
                fifo32_put(sys_fifo, cons->sht - shtctl->sheets0 + 2024);
                cons->sht = 0;
                io_sti();
            }
            if (256 <= i) { // 不但要接收来自键盘的数据，也要接收timer等数据
                reg[7] = i - 256;
                return 0;
            }
        }
    } else if (edx == 16) {
        // timer alloc
        /*
            EDX: 16
            EAX: timer句柄（由操作系统返回）
        */
        reg[7] = (int) timer_alloc();
        ((struct TIMER *) reg[7])->flags2 = 1;  // 需要随着应用程序结束自动取消
    } else if (edx == 17) {
        // timer init
        /*
            EDX: 17
            EBX: timer句柄
            EAX: timer data
        */
        timer_init((struct TIMER *) ebx, &task->fifo, eax + 256);   // 因为get_key的API中缓冲区中的数据会被减掉256
    } else if (edx == 18) {
        // set timer
        /*
            EDX: 18
            EBX: timer句柄
            EAX: period of time
        */
        timer_settime((struct TIMER *) ebx, eax);
    } else if (edx == 19) {
        // free timer
        /*
            EDX: 19
            EBX: timer句柄
        */
        timer_free((struct TIMER *) ebx);
    } else if (edx == 20) {
        // 蜂鸣器发声
        /*
            EDX: 20
            EAX: 发声频率（mHz），如eax=4400000，发出440Hz的声音（440Hz是中央C上的A音，即国际标准音）
                eax=0时表示停止发声
        */
        if (eax == 0) {
            i = io_in8(0x61);           // 向CPU发送0x61，表示控制蜂鸣器
            io_out8(0x61, i & 0x0d);    // 蜂鸣器OFF
        } else {
            i = 1193180000 / eax;
            io_out8(0x43, 0xb6);        // 开始设定音高
            io_out8(0x42, i & 0xff);    // 先设定音高值的低8位
            io_out8(0x42, i >> 8);      // 再设定音高值的高8位
            i = io_in8(0x61);           // 向CPU发送0x61，表示控制蜂鸣器
            io_out8(0x61, (i | 0x03) & 0x0f);   // 蜂鸣器ON
        }
    } else if (edx == 21) {
	// open file
	/*
	    EDX: 21
	    EBX: file name
	    EAX: 文件句柄
	*/
	for (i = 0; i < 8; i++) {
	    if (task->fhandle[i].buf == 0) {
		break;
	    }
	}
	fh = &task->fhandle[i];
	reg[7] = 0;
	if (i < 8) {
	    finfo = file_search((char *) ebx + ds_base, 
		    (struct FILEINFO *) (ADR_DISKIMG + 0x002600), 224);
	    if (finfo != 0) {
		reg[7] = (int) fh;
		fh->buf = (char *) memman_alloc_4k(memman, finfo->size);
		fh->size = finfo->size;
		fh->pos = 0;
		file_loadfile(finfo->clustno, finfo->size, fh->buf, task->fat, 
			    (char *) (ADR_DISKIMG + 0x003e00));
	    }
	}
    } else if (edx == 22) {
	// close file
	/*
	    EDX: 22
	    EAX: 文件句柄
	*/
	fh = (struct FILEHANDLE *) eax;
	memman_free_4k(memman, (int) fh->buf, fh->size);
	fh->buf = 0;
    } else if (edx == 23) {
	// 文件定位
	/*
	    EDX: 23
	    EAX: 文件句柄
	    ECX: 定位模式
		0: 定位起点为文件开头
		1: 定位起点为传入的偏移量开始
		2: 定位起点为文件末尾
	    EBX: 定位偏移量
	*/
	fh = (struct FILEHANDLE *) eax;
	if (ecx == 0) {
	    fh->pos = ebx;
	} else if (ecx == 1) {
	    fh->pos += ebx;
	} else if (ecx == 2) {
	    fh->pos = fh->size + ebx;
	}

	if (fh->pos > fh->size) {
	    fh->pos = 0;
	} else if (fh->pos > fh->size) {
	    fh->pos = fh->size;
	}
    } else if (edx == 24) {
	// get file size
	/*
	    EDX: 24
	    EAX: 文件句柄
	    ECX: 文件大小获取方式
		0: 普通文件大小
		1: 当前读取位置从文件开头起算的偏移量
		2: 当前读取位置从文件末尾起算的偏移量
	    EAX: 文件大小(returned by OS)
	*/
	fh = (struct FILEHANDLE *) eax;
	if (ecx == 0) {
	    reg[7] = fh->size;
	} else if (ecx == 1) {
	    reg[7] = fh->pos;
	} else if (ecx == 2) {
	    reg[7] = fh->pos - fh->size;
	}
    } else if (edx == 25) {
	// read file
	/*
	    EDX: 25
	    EAX: 文件句柄
	    EBX: buffer address
	    ECX: max size of reading
	    EAX: 本次读取到的字节数(returned by OS)
	*/
	fh = (struct FILEhANDLE *) eax;
	for (i = 0; i < ecx; i++) {
	    if (fh->pos == fh->size) {
		break;
	    }
	    *((char *) ebx + ds_base + i) = fh->buf[fh->pos];
	    fh->pos++;
	}
	reg[7] = i;
    }

    return 0;   // 应用程序还没结束，返回0
}

void hrb_api_linewin(struct SHEET *sht, int x0, int y0, int x1, int y1, int col) {
    /*
        for (i = 0; i < len; i++) {
            api_point(win, x / 1000, y / 1000, col);
            x += dx;
            y += dy;
        }
    */
    int i, x, y, len, dx, dy;
    dx = x1 - x0;
    dy = y1 - y0;
    // 左移10位, 为了让绘图时x和y能承载更多dx和dy的细节
    x = x0 << 10;
    y = y0 << 10;

    // 因为要比较变化量的大小，先都处理为正数
    if (dx < 0) {
        dx = -dx;
    }
    if (dy < 0) {
        dy = -dy;
    }
    
    // 计算len: 以dx和dy中较大的那个作为len; len需要+1是因为如果指定的2坐标重合，那么至少要画一个点
    // 计算dx和dy: 将dx和dy中较大的设为1024（最后都要除掉）; 另一个用变化量 * 1024的值去除len
    if (dx >= dy) {
        len = dx + 1;
        if (x0 > x1) {
            dx = -1024;
        } else {
            dx = 1024;
        }

        if (y0 <= y1) {
            dy = ((y1 - y0 + 1) << 10) / len;
        } else {
            dy = ((y1 - y0 - 1) << 10) / len;
        }
    } else {
        len = dy + 1;
        if (y0 > y1) {
            dy = -1024;
        } else {
            dy = 1024;
        }

        if (x0 <= x1) {
            dx = ((x1 - x0 + 1) << 10) / len;
        } else {
            dx = ((x1 - x0 - 1) << 10) / len;
        }
    }

    // len就是画多少个点
    for (i = 0; i < len; i++) {
        sht->buf[(y >> 10) * sht->bxsize + (x >> 10)] = col;
        x += dx;
        y += dy;
    }

    return;
}

// 处理栈异常的中断
int *inthandler0c(int *esp) {
    struct TASK *task = task_now();
    struct CONSOLE *cons = task->cons;
    char s[30];
    cons_putstring(cons, "[INT 0C]: \nStack Exception.\n", COL8_FFFFFF, COL8_840000, -1);
    sprintf(s, "EIP = %08X\n", esp[11]);    // esp指针的第11位是EIP，即指出哪跳汇编指令出问题
    cons_putstring(cons, s, COL8_FFFFFF, COL8_840000, -1);
    return &(task->tss.esp0);   // 强制结束程序
}

// 强制结束程序的中断处理
int *inthandler0d(int *esp) {
    struct TASK *task = task_now();
    struct CONSOLE *cons = task->cons;
    char s[30];
    cons_putstring(cons, "[INT 0D]: \nGeneral Protected Exception.\n", COL8_FFFFFF, COL8_840000, -1);
    sprintf(s, "EIP = %08X\n", esp[11]);
    cons_putstring(cons, s, COL8_FFFFFF, COL8_840000, -1);
    return &(task->tss.esp0);   // 强制结束程序
}

struct TASK *open_constask(struct SHEET *sht, unsigned int memtotal, int cons_size_x, int cons_size_y) {
    struct MEMMAN *memman = (struct MEMMAN *) MEMMAN_ADDR;
    struct TASK *task = task_alloc();
    int *cons_fifo = (int *) memman_alloc_4k(memman, 128 * 4);
    task->cons_stack = memman_alloc_4k(memman, 64 * 1024);  // 分配栈内存，将栈基址保存在task->cons_stack
    task->tss.esp = task->cons_stack + 64 * 1024 - 20;      // 让栈指针指向当前栈顶，因为后面要压4个参数进去，所以-20
    task->tss.eip = (int) &console_task;                    // 指令指针指向console_task
    task->tss.es = 1 * 8;
    task->tss.cs = 2 * 8;
    task->tss.ss = 1 * 8;
    task->tss.ds = 1 * 8;
    task->tss.fs = 1 * 8;
    task->tss.gs = 1 * 8;
    *((int *) (task->tss.esp + 4)) = (int) sht;  
    *((int *) (task->tss.esp + 8)) = memtotal;         // 将memtotal作为第二个参数传入cons任务的入口函数
    *((int *) (task->tss.esp + 12)) = cons_size_x;
    *((int *) (task->tss.esp + 16)) = cons_size_y;
    task_run(task, 2, 2);   // level = 2, priority = 2
    // 将console的fifo的初始化移动到这里进行
    fifo32_init(&task->fifo, 128, cons_fifo, task);
    
    return task;
}

struct SHEET *open_console( 
                            struct SHTCTL *shtctl, 
                            unsigned int memtotal, 
                            int cons_size_x, 
                            int cons_size_y
                            ) {
    struct MEMMAN *memman = (struct MEMMAN *) MEMMAN_ADDR;
    struct SHEET *sht = sheet_alloc(shtctl);
    unsigned char *buf = (unsigned char *) memman_alloc_4k(memman, (8 + cons_size_x + 8) * (28 + cons_size_y + 9));
    sheet_setbuf(sht, buf, 8 + cons_size_x + 8, 28 + cons_size_y + 9, -1);
    make_window8(buf, 8 + cons_size_x + 8, 28 + cons_size_y + 9, "console", 0);
    make_textbox8(sht, 8, 28, cons_size_x, cons_size_y, COL8_000000);
    sht->task = open_constask(sht, memtotal, cons_size_x, cons_size_y);
    sht->flags |= 0x20;     // 有光标

    return sht;
}

void close_cons_stack(struct TASK *task) {
    struct MEMMAN *memman = (struct MEMMAN *) MEMMAN_ADDR;
    task_sleep(task);
    memman_free_4k(memman, task->cons_stack, 64 * 1024);
    memman_free_4k(memman, (int) task->fifo.buf, 128 * 4);
    task->flags = 0;    // 代替task_free(task)

    return;
}

void close_console(struct SHEET *sht) {
    struct MEMMAN *memman = (struct MEMMAN *) MEMMAN_ADDR;
    struct TASK *task = sht->task;
    memman_free_4k(memman, (int) sht->buf, sht->bxsize * sht->bysize);
    sheet_free(sht);
    close_cons_stack(task);

    return;
}

// 修剪cmdline, 调整为标准的 cmd arg 形式
char * trim_cmdline_space(char * cmdline) {
    int i = 0, j = 0, len = strlen(cmdline);
    static char trimmed[256];

    while (cmdline[i] == ' ') {
        i++;
    }

    while (i < len) {
        trimmed[j++] = cmdline[i++];
    }

    while (j > 0 && trimmed[j - 1] == ' ') {
        j--;
    }
    trimmed[j] = 0x00;

    return trimmed;
}

char * remove_cmdline_middle_space(char *cmdline, char *cmd) {
    int i = 0, j = 0, end = 0, len = strlen(cmdline);
    static char trimmed[256];

    // 来到cmd之后的内容
    // 将cmdline的内容拷贝到trimmed
    end = i + strlen(cmd);
    while (i < end) {
        trimmed[j++] = cmdline[i++];
    }

    // cmd与arg之间的空格
    while (i < len && cmdline[i] == ' ') {
        i++;
    }

    // 拷贝参数
    trimmed[j++] = ' ';
    while (i < len && cmdline[i] != ' ') {
        trimmed[j++] = cmdline[i++];
    }
    trimmed[j] = 0x00;

    return trimmed;
}
