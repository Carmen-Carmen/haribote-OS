// 图层相关
#include "bootpack.h"

// 初始化SHTCTL结构体
// 由于此结构体内部有2个长256的32字节结构体（SHEET）数组，大小很大，于是需要为其分配内存
struct SHTCTL *shtctl_init(struct MEMMAN *memman, unsigned char *vram, int xsize, int ysize) {
    struct SHTCTL *ctl;
    int i;
    ctl = (struct SHTCTL *) memman_alloc_4k(memman, sizeof (struct SHTCTL));    // 让ctl指向分配出的内存的第一个地址

    if (ctl == 0) { goto err; } // ctl理应指向地址0
    
    // 为ctl中的map开辟一段内存，大小为画面大小
    ctl->map = (unsigned char *) memman_alloc_4k(memman, xsize * ysize);
    if (ctl->map == 0) {        // map理应指向这段内存的地址0，否则错误
        memman_free_4k(memman, (int) ctl, sizeof(struct SHTCTL));
        goto err;
    }

    ctl->vram = vram;
    ctl->xsize = xsize;
    ctl->ysize = ysize;
    ctl->top = -1;      // 刚开始一个SHEET也没有

    for (i = 0; i < MAX_SHEETS; i++) {
        ctl->sheets0[i].flags = 0;  // 将每一个SHEET都标记为未使用
        ctl->sheets0[i].ctl = ctl;  // 将每一个SHEET的ctl属性都设为当前初始化的ctl
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
            sht->task = 0;          // 不使用自动关闭功能
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
// 更新后，不需要传入ctl参数
void sheet_updown(struct SHEET *sht, int height) {
    struct SHTCTL *ctl = sht->ctl;
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
            sheet_refreshmap(   ctl, 
                                sht->vx0, 
                                sht->vy0, 
                                sht->vx0 + sht->bxsize, 
                                sht->vy0 + sht->bysize, 
                                height + 1);
            sheet_refreshsub(   ctl, 
                                sht->vx0, 
                                sht->vy0, 
                                sht->vx0 + sht->bxsize, 
                                sht->vy0 + sht->bysize, 
                                height + 1, 
                                old);       // 从当前高度刷新到原来高度
        } else {            // 1.2 想要隐藏图层（height = -1）
            if (ctl->top > old) {
                for (h = old; h < ctl->top; h++) {
                    ctl->sheets[h] = ctl->sheets[h + 1];
                    ctl->sheets[h]->height = h;
                }
            }
            ctl->top--;     // 处于显示状态的图层减少1个，因此最上面图层的高度下降1
            sheet_refreshmap(   ctl, 
                                sht->vx0, 
                                sht->vy0, 
                                sht->vx0 + sht->bxsize, 
                                sht->vy0 + sht->bysize, 
                                0);
            sheet_refreshsub(   ctl, 
                                sht->vx0, 
                                sht->vy0, 
                                sht->vx0 + sht->bxsize, 
                                sht->vy0 + sht->bysize, 
                                0, 
                                old - 1);       // 从最底部刷新到原来高度 - 1
        }

    // 2. 图层新高度 > 原先高度 
    } else if (old < height) {  
        if (old >= 0) {     // 2.1 原先就是已经显示的图层
            for (h = old; h < height; h++) {
                ctl->sheets[h] = ctl->sheets[h + 1];
                ctl->sheets[h]->height = h;
            }
            ctl->sheets[height] = sht;
        } else {            // 2.2 想要显示被隐藏的图层（old = -1)
            for (h = ctl->top; h >= height; h--) {
                ctl->sheets[h + 1] = ctl->sheets[h];
                ctl->sheets[h + 1]->height = h + 1;
            }
            ctl->sheets[height] = sht;
            ctl->top++;     // 处于显示状态的图层增加1个，因此最上面图层高度增加1
        }
        sheet_refreshmap(   ctl,
                            sht->vx0, 
                            sht->vy0, 
                            sht->vx0 + sht->bxsize, 
                            sht->vy0 + sht->bysize, 
                            height);
        sheet_refreshsub(   ctl,  
                            sht->vx0, 
                            sht->vy0, 
                            sht->vx0 + sht->bxsize, 
                            sht->vy0 + sht->bysize, 
                            height, 
                            height); // 当前图层为最高层，只需绘制自己就行
    }
    
    return;
}

// 移动图层位置
/*
    vx0, vy0是目标位置的左上角坐标
*/
// 更新后不再需要传入ctl
void sheet_slide(struct SHEET *sht, int vx0, int vy0) {
    struct SHTCTL *ctl = sht->ctl;
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
        // 移动图层后，map中数据必发生改变
        // 先刷新map
        sheet_refreshmap(ctl, old_vx0, old_vy0, old_vx0 + sht->bxsize, old_vy0 + sht->bysize, 0);
        sheet_refreshmap(ctl, vx0, vy0, vx0 + sht->bxsize, vy0 + sht->bysize, sht->height);
        // 再调用refreshs
        sheet_refreshsub(ctl, old_vx0, old_vy0, old_vx0 + sht->bxsize, old_vy0 + sht->bysize, 0, sht->height - 1);  // 图层发生移动之后，原来所处的区域应当从0高度开始刷新，刷新到其下方一层
        sheet_refreshsub(ctl, vx0, vy0, vx0 + sht->bxsize, vy0 + sht->bysize, sht->height, sht->height);
    }
    return;
}

// 绘制所有图层
/*
    sht: 作为指定范围基准的图层
    bx0, by0, bx1, by1: buffer_x/y1/2？指定范围在该图层缓冲区内的坐标，有利于窗口的创建（可以相对于窗口的(0, 0)来写偏移量），调用时也不需要考虑图层在画面中所处位置了
*/
// 更新后不再需要传入ctl
void sheet_refresh(struct SHEET *sht, int bx0, int by0, int bx1, int by1) {
    // 如果该图层正在显示，就要刷新
    if (sht->height >= 0) {
        // 将指定的画面坐标转换为图层缓冲区内的坐标，传给sheet_refreshsub函数
        sheet_refreshsub(sht->ctl,   
                            sht->vx0 + bx0, 
                            sht->vy0 + by0, 
                            sht->vx0 + bx1, 
                            sht->vy0 + by1, 
                            sht->height, 
                            sht->height);   // 仅刷新当前图层
    };

    return;
}

// 绘制所有图层中(vx0, vy0) ~ (vx1, vy1)部分的局部矩形（vx, vy都是换算后画面中的坐标）
/*
    h0: 当前要刷新的图层的高度
    h1: 想要刷新到的最高高度
*/
void sheet_refreshsub(struct SHTCTL *ctl, int vx0, int vy0, int vx1, int vy1, int h0, int h1) {
    int h, bx, by, vx, vy, bx0, by0, bx1, by1;
    unsigned char   *buf, 
                    *vram = ctl->vram, 
                    *map = ctl->map, 
                    sid;
    struct SHEET *sht;

    // 不刷新画面之外的区域
    if (vx0 < 0) { vx0 = 0; }
    if (vy0 < 0) { vy0 = 0; }
    if (vx1 > ctl->xsize) { vx1 = ctl->xsize; }
    if (vy1 > ctl->ysize) { vy1 = ctl->ysize; }

    for (h = h0; h <= h1; h++) {   
        sht = ctl->sheets[h];
        buf = sht->buf;
        sid = sht - ctl->sheets0;    // 计算出当前循环到图层的sid
        
        // 用传入的vx, vy，反推出要渲染部分在当前图层的缓冲区内的坐标
        bx0 = vx0 - sht->vx0;
        by0 = vy0 - sht->vy0;
        bx1 = vx1 - sht->vx0;
        by1 = vy1 - sht->vy0;
        
        // 如欲刷新区域只有右下角与当前图层有交集
        if (bx0 < 0) { bx0 = 0; }
        if (by0 < 0) { by0 = 0; }

        // 如欲刷新区域只有左上角与当前图层有交集
        if (bx1 > sht->bxsize) { bx1 = sht->bxsize; }
        if (by1 > sht->bysize) { by1 = sht->bysize; }

        // 于是只需要循环遍历(bx0, by0) ~ (bx1, by1)的像素点，循环次数大大减少
        for (by = by0; by < by1; by++) {
            vy = sht->vy0 + by; // 确定当轮循环渲染的y坐标（画面中）
            for (bx = bx0; bx < bx1; bx++) {
                vx = sht->vx0 + bx; // 确定当轮循环渲染的x坐标（画面中）

                // 仅绘制map中属于当前图层sid的像素
                if (map[vy * ctl->xsize + vx] == sid) {
                    vram[vy * ctl->xsize + vx] = buf[by * sht->bxsize + bx];    // buf[by * sht->bxsize + bx]存储了当前坐标的像素颜色
                }
            }
        }
    }

    return;
}

// 刷新map数据
void sheet_refreshmap(struct SHTCTL *ctl, int vx0, int vy0, int vx1, int vy1, int h0) {
    int h, bx, by, vx, vy, bx0, by0, bx1, by1;
    unsigned char   *buf, 
                    sid,                // sheet_id
                    *map = ctl->map;
    struct SHEET *sht;
    
    // 不刷新画面以外的map
    if (vx0 < 0) { vx = 0; }
    if (vy0 < 0) { vy = 0; }
    if (vx1 > ctl->xsize) { vx1 = ctl->xsize; }
    if (vy1 > ctl->ysize) { vy1 = ctl->ysize; }

    for (h = h0; h <= ctl->top; h++) {   
        sht = ctl->sheets[h];
        sid = sht - ctl->sheets0;   // 将当前sheet的地址与ctl中sheets0数组的地址相减（数组中第n个元素的地址 - 第0个元素的地址），作为图层的id
        buf = sht->buf;
        
        bx0 = vx0 - sht->vx0;
        by0 = vy0 - sht->vy0;
        bx1 = vx1 - sht->vx0;
        by1 = vy1 - sht->vy0;
        
        if (bx0 < 0) { bx0 = 0; }
        if (by0 < 0) { by0 = 0; }
        if (bx1 > sht->bxsize) { bx1 = sht->bxsize; }
        if (by1 > sht->bysize) { by1 = sht->bysize; }

        if (sht->col_inv == -1) {
            // 没有透明色的图层，如窗口等
            for (by = by0; by < by1; by++) {
                vy = sht->vy0 + by; // 确定当轮循环渲染的y坐标（画面中）
                for (bx = bx0; bx < bx1; bx++) {
                    vx = sht->vx0 + bx; // 确定当轮循环渲染的x坐标（画面中）
                    map[vy * ctl->xsize + vx] = sid;    // 可以直接渲染，减少一次if判断，加快刷新速度
                    
                }
            }
        } else {
            // 有透明色的图层，如鼠标等
            for (by = by0; by < by1; by++) {
                vy = sht->vy0 + by; // 确定当轮循环渲染的y坐标（画面中）
                for (bx = bx0; bx < bx1; bx++) {
                    vx = sht->vx0 + bx; // 确定当轮循环渲染的x坐标（画面中）

                    // 如果当前像素不是透明的，就把sid写入map中其坐标对应的地址
                    if (buf[by * sht->bxsize + bx] != sht->col_inv) {
                        map[vy * ctl->xsize + vx] = sid;
                    }
                    
                }
            }
        }

    }

    return;
}

// 释放已使用图层
// 更新后不再需要传入ctl
void sheet_free(struct SHEET *sht) {
    // 如果要归还的图层正在显示，先将其隐藏
    if (sht->height >= 0) {
        sheet_updown(sht, -1); 
    }
    sht->flags = 0; // 重新设置为“未使用”
    return;
}

