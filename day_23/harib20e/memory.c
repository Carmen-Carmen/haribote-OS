// 内存管理相关
#include "bootpack.h"

void memman_init(struct MEMMAN *man) {
    man->frees = 0;     // 可用信息数目
    man->maxfrees = 0;  // 用于观察可用情况：frees的最大值
    man->lostsize = 0;  // 释放失败的内存的大小总和
    man->losts = 0;     // 释放失败次数
    return;
}

// 报告剩余内存大小的合计
unsigned int memman_total(struct MEMMAN *man) {
    unsigned int i, t = 0;
    for (i = 0; i < man->frees; i++) {
        t += man->free[i].size;
    }

    return t;
}

// 分配内存
unsigned int memman_alloc(struct MEMMAN *man, unsigned int size) {
    unsigned int i;
    unsigned int a = 0;  // a: address
    for (i = 0; i < man->frees; i++) {
        if (man->free[i].size >= size) {    // 如果找到了足够大的内存
            a = man->free[i].addr;
            man->free[i].addr += size;
            man->free[i].size -= size;
            if (man->free[i].size == 0) {   // 如果free[i]的大小变成0，就去掉这条信息
                man->frees--;
                // 把free数组中free[i]后所有元素向前移动1位
                for (; i < man->frees; i++) {
                    // 结构体数组也可以用指针直接
                    man->free[i] = man->free[i + 1];
                }
            }
            break;
        }
    }
    return a;   // 0
}

// 释放内存
int memman_free(struct MEMMAN *man, unsigned int addr, unsigned int size) {
    int i, j;
    // 为了便于归纳内存，将定位到free[]数组中，传入addr参数的前一个地址
    for (i = 0; i < man->frees; i++) {
        if (man->free[i].addr > addr) {
            break;
        }
    }
    // 此时 free[i - 1].addr < addr < free[i].addr
    
    // 1. 既可以与前一块内存合并，也可以与后一块内存合并
    if (i > 0) {
        // 前面有可用内存
        if (man->free[i - 1].addr + man->free[i - 1].size == addr) {
            // 前一个可用内存的地址 + 其大小 == 归还内存的起始地址
            // 可以与前面的可用内存归纳到一起
            man->free[i - 1].size += size;
            if (i < man->frees) {
                // 后面还有可用内存
                if (addr + size == man->free[i].addr) {
                    // 可以与后一个可用内存合并
                    man->free[i - 1].size += man->free[i].size;
                    // 删除man->free[i]
                    man->frees--;
                    for (; i < man->frees; i++) {
                        man->free[i] = man->free[i + 1];
                    }
                }
            }
            return 0;   // 完成
        }
    }
    
    // 2. 只能与后一块内存合并
    if (i < man->frees) {
        if (addr + size == man->free[i].addr) {
            man->free[i].addr = addr;
            man->free[i].size += size;
            return 0;
        }
    }

    // 3. 既不能与前一块合并，也不能与后一块合并
    if (man->frees < MEMMAN_FREES) {
        // 就要把free[i]及其之后的都向后移1位
        for (j = man->frees; j > i; j--) {
            man->free[j] = man->free[j - 1];
        }
        man->frees++;
        if (man->maxfrees < man->frees) {
            man->maxfrees = man->frees; // 更新maxfrees的值
        }
        // 空出来的free[i]放入当前归还内存的信息
        man->free[i].addr = addr;
        man->free[i].size = size;
        return 0;
    }

    // 4. 如果当前man->frees的值比定义的MEMMAN_FREES还大
    // 即已经生成太多内存片段，无法进一步分割了
    // 虽然内存已经释放，但是由于无法被MEMMAN管理到，因此视作损失的内存
    man->losts++;
    man->lostsize += size;
    return -1;  // 失败
}

// 以0x1000字节（4KB）为单位进行内存分配和释放，将指定的内存大小按0x1000字节为单位进行向上舍入（roundup）
// 分配
unsigned int memman_alloc_4k(struct MEMMAN *man, unsigned int size) {
    unsigned int a;
    // 对要分配的内存大小进行向上舍入
    size = (size + 0xfff) & 0xfffff000; // 除非最后3位是0x000，否则就向上补齐
    a = memman_alloc(man, size);
    return a;
}

// 释放
int memman_free_4k(struct MEMMAN *man, unsigned int addr, unsigned int size) {
    int i;
    size = (size + 0xfff) & 0xfffff000;
    i = memman_free(man, addr, size);
    return i;
}

// 返回值为出错的内存单元地址，即第一个读不到的内存单元，也就是整体内存的大小了
unsigned int memtest(unsigned int start, unsigned int end) {
    // 为了测试内存连接是否正常，采取向内存中随便写入一个值再马上读取
    // 然而由于缓存机制的存在，再次读回的信息是从缓存读取的，一定不会有错
    // 为此，需要把CPU的缓存功能暂时关闭（486及以上CPU有缓存功能）
    
    // 1. 禁止缓存
    char flg486 = 0;
    unsigned int eflg, cr0, i;

    // 确认CPU是386还是486及以上的
    eflg = io_load_eflags();            // 读取当前eflags信息
    eflg |= EFLAGS_AC_BIT;              // 与运算，将Allow Cache位（第18位）设为1
    io_store_eflags(eflg);              // 将设好AC位的eflags存回EFLAGS寄存器
    eflg = io_load_eflags();            // 再次读取eflags信息
    // 如果是386处理器，那么之前设置AC位为1的操作是无效的，再次读取的eflags信息中AC = 0
    if ((eflg & EFLAGS_AC_BIT) != 0) {  // 所以如果AC位是1，那么当前处理器位486
        flg486 = 1;
    }
    eflg &= ~EFLAGS_AC_BIT;             // 取反操作，将eflags的AC位设为0
    io_store_eflags(eflg);              // 存回EFLAGS寄存器

    if (flg486 != 0) {                  // 如果之前判断出是486处理器
        cr0 = load_cr0();
        cr0 |= CR0_CACHE_DISABLE;       // 将控制寄存器0 cr0中允许缓存功能关闭
        store_cr0(cr0);                 // 将设好的cr0信息存回cr0寄存器
    }
    
    // 2. 执行内存测试
    i = memtest_sub(start, end);        // 进行实际上的内存测试，返回值为出错的内存单元地址（汇编编写）

    // 3. 允许缓存
    if (flg486 != 0) {                  // 再把允许缓存打开
        cr0 = load_cr0();
        cr0 &= ~CR0_CACHE_DISABLE;
        store_cr0(cr0);
    }

    return i;
}
