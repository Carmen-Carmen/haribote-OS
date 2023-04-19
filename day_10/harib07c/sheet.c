// 图层相关
#include "bootpack.h"

// 初始化SHTCTL结构体
// 由于此结构体内部有2个长256的32字节结构体（SHEET）数组，大小很大，于是需要为其分配内存
struct SHTCTL *shtctl_init(struct MEMMAN *memman, unsigned char *vram, int xsize, int ysize) {
    struct SHTCTL *ctl;
    int i;
    ctl = (struct SHTCTL *) memman_alloc_4k(memman, sizeof (struct SHTCTL));

    if (ctl == 0) { goto err; }

    ctl->vram = vram;
    ctl->xsize = xsize;
    ctl->ysize = ysize;
    ctl->top = -1;      // 刚开始一个SHEET也没有

    for (i = 0; i < MAX_SHEETS; i++) {
        ctl->sheets0[i].flags = 0;  // 将每一个SHEET都标记为未使用
    }

err: 
    return ctl;
}

// 获取新生成的未使用图层
struct SHEET *sheet_alloc(struct SHTCTL *ctl) {
    struct SHEET *sht;
    int i;
    for (i = 0; i < MAX_SHEETS; i++) {
        // 找到第一个未使用图层
        if (ctl->sheets0[i].flags == 0) {
            sht = &ctl->sheets0[i]; // 将sht指针指向这个图层*的地址*（指针只能被赋予地址）
            sht->flags = SHEET_USE; // 将其状态标记为正在使用
            sht->height = -1;       // 先隐藏此图层，后续自己更改高度
            return sht;
        }
    }

    return 0;   // 所有SHEET都处于正在使用状态
}

// 设定图层缓冲区大小和透明色
void sheet_setbuf(struct SHEET *sht, unsigned char *buf, int xsize, int ysize, int col_inv) {
    sht->buf = buf;
    sht->bxsize = xsize;
    sht->bysize = ysize;
    sht->col_inv = col_inv;
    return;
}

// 设定图层高度
void sheet_updown(struct SHTCTL *ctl, struct SHEET *sht, int height) {
    int h, old = sht->height;

    // 高度参数错误（过高或过低），进行修正
    if (height > ctl->top + 1) {    // 就是想设置成最高图层，比如传了999，那么要把999处理成当前最高图层高度+1
        height = ctl->top + 1;
    }
    if (height < -1) {
        height = -1;
    }
    sht->height = height;           // 设定正确的高度

    // 对sheets[]数组的重新排列
    // 1. 图层新高度 < 原先高度
    if (old > height) {         
        if (height >= 0) {  // 1.1 想要显示图层
            for (h = old; h > height; h--) {
                ctl->sheets[h] = ctl->sheets[h - 1];
                ctl->sheets[h]->height = h;
            }
            ctl->sheets[height] = sht;
        } else {            // 1.2 想要隐藏图层（height = -1）
            if (ctl->top > old) {
                for (h = old; h < ctl->top; h++) {
                    ctl->sheets[h] = ctl->sheets[h + 1];
                    ctl->sheets[h]->height = h;
                }
            }
            ctl->top--;     // 处于显示状态的图层减少1个，因此最上面图层的高度下降1
        }
        sheet_refreshsub(ctl,  sht->vx0, 
                            sht->vy0, 
                            sht->vx0 + sht->bxsize, 
                            sht->vy0 + sht->bysize); // 重新绘制所有图层

    // 2. 图层新高度 > 原先高度 
    } else if (old < height) {  
        if (old >= 0) {     // 2.1 原先就是已经显示的图层
            for (h = old; h < height; h++) {
                ctl->sheets[h] = ctl->sheets[h + 1];
                ctl->sheets[h]->height = h;
            }
        } else {            // 2.2 想要显示被隐藏的图层（old = -1)
            for (h = ctl->top; h >= height; h--) {
                ctl->sheets[h + 1] = ctl->sheets[h];
                ctl->sheets[h + 1]->height = h + 1;
            }
            ctl->sheets[height] = sht;
            ctl->top++;     // 处于显示状态的图层增加1个，因此最上面图层高度增加1
        }
        sheet_refreshsub(ctl,  sht->vx0, 
                            sht->vy0, 
                            sht->vx0 + sht->bxsize, 
                            sht->vy0 + sht->bysize); // 重新绘制所有图层
    }
    
    return;
}

// 移动图层位置
/*
    vx0, vy0是目标位置的左上角坐标
*/
void sheet_slide(struct SHTCTL *ctl, struct SHEET *sht, int vx0, int vy0) {
    // 记录图层原先坐标
    int old_vx0 = sht->vx0, 
        old_vy0 = sht->vy0;
    // 将图层坐标更新为(vx0, vy0)
    sht->vx0 = vx0;
    sht->vy0 = vy0;
    // 如果该图层正在显示，就刷新2个矩形
    // 移动前的区域(old_vx0, old_vy0) ~ (old_vx0 + width, old_vy0 + height)
    // 移动后的区域(vx0, vy0) ~ (vx0 + width, vy0 + height)
    if (sht->height >= 0) {
        sheet_refreshsub(ctl, old_vx0, old_vy0, old_vx0 + sht->bxsize, old_vy0 + sht->bysize);
        sheet_refreshsub(ctl, vx0, vy0, vx0 + sht->bxsize, vy0 + sht->bysize);
    }
    return;
}

// 绘制所有图层
/*
    sht: 作为指定范围基准的图层
    bx0, by0, bx1, by1: buffer_x/y1/2？指定范围在该图层缓冲区内的坐标，有利于窗口的创建（可以相对于窗口的(0, 0)来写偏移量），调用时也不需要考虑图层在画面中所处位置了
*/
void sheet_refresh(struct SHTCTL *ctl, struct SHEET *sht, int bx0, int by0, int bx1, int by1) {
    // 如果该图层正在显示，就要刷新
    if (sht->height >= 0) {
        // 将指定的画面坐标转换为图层缓冲区内的坐标，传给sheet_refreshsub函数
        sheet_refreshsub(ctl,   sht->vx0 + bx0, 
                                sht->vy0 + by0, 
                                sht->vx0 + bx1, 
                                sht->vy0 + by1);
    };

    return;
}

// 绘制所有图层中(vx0, vy0) ~ (vx1, vy1)部分的局部矩形（vx, vy都是换算后画面中的坐标）
void sheet_refreshsub(struct SHTCTL *ctl, int vx0, int vy0, int vx1, int vy1) {
    int h, bx, by, vx, vy;
    unsigned char *buf, c, *vram = ctl->vram;   // 只给最后一个变量赋值了
    struct SHEET *sht;
    // 从0高度开始，从下往上绘制所有图层，height大的图层显示在上层
    // 因此高度为-1的图层都是不显示的
    for (h = 0; h <= ctl->top; h++) {   
        sht = ctl->sheets[h];
        buf = sht->buf;
        for (by = 0; by < sht->bysize; by++) {
            vy = sht->vy0 + by; // 确定当轮循环渲染的y坐标
            for (bx = 0; bx < sht->bxsize; bx++) {
                vx = sht->vx0 + bx; // 确定当轮循环渲染的x坐标

                // 确定当轮循环的(x, y)是否处于需要刷新的矩形内部
                if (vx >= vx0 && vx < vx1 && vy >= vy0 && vy < vy1) {
                    c = buf[by * sht->bxsize + bx];
                // 如果不是要隐藏的颜色，才需要绘制出来
                    if (c != sht->col_inv) {
                        vram[vy * ctl->xsize + vx] = c;
                    }
                }
            }
        }
    }

    return;
}

// 释放已使用图层
void sheet_free(struct SHTCTL *ctl, struct SHEET *sht) {
    // 如果要归还的图层正在显示，先将其隐藏
    if (sht->height >= 0) {
        sheet_updown(ctl, sht, -1); 
    }
    sht->flags = 0; // 重新设置为“未使用”
    return;
}

