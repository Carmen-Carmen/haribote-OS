// FIFO型缓冲区的源文件
#include "bootpack.h"

#define FLAGS_OVERRUN       0x0001

// 初始化fifo型缓冲区
void fifo8_init(struct FIFO8 *fifo, int size, unsigned char *buf) {
    fifo->size = size;
    fifo->buf = buf;
    fifo->free = size;      // 缓冲区空闲大小
    fifo->flags = 0;
    fifo->p = 0;            // 队头
    fifo->q = 0;            // 队尾
    return;
}

// 向fifo型缓冲区中写入8位数据（char）
/*
    return: 0, 写入成功; -1, 失败
*/
int fifo8_put(struct FIFO8 *fifo, unsigned char data) {
    if (fifo->free == 0) {
        // 缓冲区已满，设置溢出标志
        fifo->flags |= FLAGS_OVERRUN;   // 按位或，结果就是fifo->flag = 1
        return -1;
    }
    
    fifo->buf[fifo->p] = data;
    fifo->p++;

    if (fifo->p == fifo->size) {
        fifo->p = 0;
    }

    fifo->free--;

    return 0;
}

// 从fifo型缓冲区读取8位数据
int fifo8_get(struct FIFO8 *fifo) {
    int data;
    if (fifo->free == fifo->size) {
        return -1;      // 缓冲区为空，取不到数据，返回-1
    }
    data = fifo->buf[fifo->q];
    fifo->q++;
    if (fifo->q == fifo->size) {
        fifo->q = 0;
    }
    fifo->free++;
    return data;
}


// 返回缓冲区空闲空间大小
int fifo8_status(struct FIFO8 *fifo) {
    return fifo->size - fifo->free;
}
