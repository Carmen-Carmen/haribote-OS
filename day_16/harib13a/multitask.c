// 多任务相关
#include "bootpack.h"

struct TASKCTL *taskctl;
struct TIMER *task_timer;

struct TASK *task_init(struct MEMMAN *memman) {
    int i;
    struct TASK *task;
    struct SEGMENT_DESCRIPTOR *gdt = (struct SEGMENT_DESCRIPTOR *) ADR_GDT;
    taskctl = (struct TASKCTL *) memman_alloc_4k(memman, sizeof (struct TASKCTL));
    for (i = 0; i < MAX_TASKS; i++) {
        taskctl->tasks0[i].flags = 0;
        taskctl->tasks0[i].sel = (TASK_GDT0 + i) * 8;
        set_segmdesc(gdt + TASK_GDT0 + i, 103, (int) &taskctl->tasks0[i].tss, AR_TSS32);
    }
    task = task_alloc();    // 这个task是调用init的程序，也被当作一个task来管理
    task->flags = 2;        // 正在运行中的标志
    taskctl->running = 1;
    taskctl->now = 0;
    taskctl->tasks[0] = task;
    load_tr(task->sel);
    task_timer = timer_alloc();
    timer_settime(task_timer, 1);

    return task;
}

struct TASK *task_alloc(void) {
    int i;
    struct TASK *task;
    for (i = 0; i < MAX_TASKS; i++) {
        // 找到第一个没有运行的task
        if (taskctl->tasks0[i].flags == 0) {
            task = &taskctl->tasks0[i];
            task->flags = 1;     // 正在使用中
            task->tss.eflags = 0x00000202;  // interrupt flag = 1
            task->tss.eax = 0;  // 都先置为0
            task->tss.ebx = 0;
            task->tss.ecx = 0;
            task->tss.edx = 0;
            task->tss.ebp = 0;
            task->tss.esi = 0;
            task->tss.edi = 0;
            task->tss.es = 0;
            task->tss.ds = 0;
            task->tss.fs = 0;
            task->tss.gs = 0;
            task->tss.ldtr = 0;
            task->tss.iomap = 0x40000000;
            return task;

        }
    }

    return 0;                   // 没有能分配的空闲task
}

void task_run(struct TASK *task) {
    task->flags = 2;            // 正在运行标志
    // 把它排在有序数组中最新一个
    taskctl->tasks[taskctl->running] = task;
    taskctl->running++;
    return;
}

void task_switch(void) {
    timer_settime(task_timer, 1);
    if (taskctl->running >= 2) {    // 如果只有1个任务，不用进行切换
        taskctl->now++;             // 向后遍历一个task
        if (taskctl->now == taskctl->running) {
            // 所有running的task都遍历过一遍，就把now置回0
            taskctl->now = 0;
        }
        // 任务切换
        farjmp(0, taskctl->tasks[taskctl->now]->sel);   // jmp sel:0 
    }

    return;
}
