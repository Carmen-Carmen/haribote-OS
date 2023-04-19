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
    task->priority = 2;     // 0.02秒切换一次
    taskctl->running = 1;
    taskctl->now = 0;
    taskctl->tasks[0] = task;
    load_tr(task->sel);
    task_timer = timer_alloc();
    timer_settime(task_timer, task->priority);

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

/*
    task: 指定要运行的task
    priority:   为0，表示不改变当前已经设定的优先级（唤醒任务时使用）
                不为0，要改变指定任务的优先级
*/
void task_run(struct TASK *task, int priority) {
    if (priority > 0) {
        task->priority = priority;
    }
    if (task->flags != 2) {
        task->flags = 2;            // 正在运行标志
        // 把它排在有序数组中最新一个
        taskctl->tasks[taskctl->running] = task;
        taskctl->running++;
    }
    return;
}

void task_switch(void) {
    struct TASK *task;
    taskctl->now++;
    
    if (taskctl->now == taskctl->running) {
        taskctl->now = 0;
    }

    task = taskctl->tasks[taskctl->now];
    timer_settime(task_timer, task->priority);
    if (taskctl->running >= 2) {    // 判断当前任务数量是否在2个以上，否则不跳转
        farjmp(0, task->sel);
    }

    return;
}

/*
    将指定的TASK休眠
    本质是将其从tasks[]数组中删去，这样task_switch时就不会被now指中，也不会被跳转
*/
void task_sleep(struct TASK *task) {
    int i;
    char ts = 0;    // 是否需要进行任务切换的flag

    // 指定任务处于唤醒状态，才需要进行休眠
    if (task->flags == 2) {
        if (task == taskctl->tasks[taskctl->now]) { // 如果指定要休眠的任务正在运行
            ts = 1; // 则必须先进行任务切换才能休眠，先记录下来
        }

        // 先找在tasks[]数组中找到这个task的索引
        for (i = 0; i < taskctl->running; i++) {
            if (taskctl->tasks[i] == task) {
                break;
            }
        }
        taskctl->running--;
        if (i < taskctl->now) { // 目标task的索引不是now指向的
            taskctl->now--;
        }
        // 从tasks数组中删掉目标task
        for (; i < taskctl->running; i++) {
            taskctl->tasks[i] = taskctl->tasks[i + 1];
        }
        task->flags = 1;        // 置为不工作的状态

        // 需要进行任务切换的情况 
        if (ts != 0) {
            if (taskctl->now >= taskctl->running) {
                // 发现now的值出现异常，就进行修正
                taskctl->now = 0;   // 修正为tasks[]数组中第一个，总不会错
            }
            farjmp(0, taskctl->tasks[taskctl->now]->sel);   // 切换到now指向的任务
        }
    }

    return;
}

