// 多任务相关
#include "bootpack.h"

struct TASKCTL *taskctl;
struct TIMER *task_timer;

struct TASK *task_init(struct MEMMAN *memman) {
    int i;
    struct TASK *task, *idle;
    struct SEGMENT_DESCRIPTOR *gdt = (struct SEGMENT_DESCRIPTOR *) ADR_GDT;
    taskctl = (struct TASKCTL *) memman_alloc_4k(memman, sizeof (struct TASKCTL));
    // 初始化所有task（tasks0[]数组）
    for (i = 0; i < MAX_TASKS; i++) {
        taskctl->tasks0[i].flags = 0;
        taskctl->tasks0[i].sel = (TASK_GDT0 + i) * 8;
        set_segmdesc(gdt + TASK_GDT0 + i, 103, (int) &taskctl->tasks0[i].tss, AR_TSS32);
    }
    // 初始化所有level
    for (i = 0; i < MAX_TASKLEVELS; i++) {
        taskctl->level[i].running = 0;
        taskctl->level[i].now = 0;
    }
    task = task_alloc();    // 这个task是调用init的程序，也被当作一个task来管理
    task->flags = 2;        // 正在运行中的标志
    task->priority = 2;     // 0.02秒切换一次
    task->level = 0;        // 姑且先定为最高LEVEL
    task_add(task);
    task_switchsub();       // 切换到当前LEVEL
    load_tr(task->sel);
    task_timer = timer_alloc();
    timer_settime(task_timer, task->priority);

    // 创建一个idle task
    idle = task_alloc();
    idle->tss.esp = memman_alloc_4k(memman, 64 * 1024) + 64 * 1024; // 此时栈指针指向栈底（高地址）
    idle->tss.eip = (int) &task_idle;   // 指令指针指向入口函数地址
    idle->tss.es = 1 * 8;
    idle->tss.cs = 2 * 8;
    idle->tss.ss = 1 * 8;
    idle->tss.ds = 1 * 8;
    idle->tss.fs = 1 * 8;
    idle->tss.gs = 1 * 8;
    task_run(idle, MAX_TASKLEVELS - 1, 1);  // 让idle任务在最下层的level以最低优先级运行，这样即使所有任务都休眠了，操作系统也会继续运行，不会出现错误

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
    level: 传入-1表示不改变LEVEL
    priority:   为0，表示不改变当前已经设定的优先级（唤醒任务时使用）
                不为0，要改变指定任务的优先级
*/
void task_run(struct TASK *task, int level, int priority) {
    if (level < 0) {
        level = task->level;    // 不改变LEVEL
    }
    if (priority > 0) {
        task->priority = priority;
    }
    
    if (task->flags == 2 && task->level != level) {
        // 需改改变运行中的level
        task_remove(task);  // 先把task从原来的LEVEL中删除，执行完后flags会变为1，以下的if代码块也会被执行
    }
    
    // 唤醒休眠状态的task
    if (task->flags != 2) {
        task->level = level;// 为指定的task设置新的level
        task_add(task);
    }

    taskctl->lv_change = 1;     // 下次任务切换时检查LEVEL是否也需要切换
    return;
}

void task_switch(void) {
    // 先找到当前运行的LEVEL和其中运行的TASK
    struct TASKLEVEL *tl = &taskctl->level[taskctl->now_lv];
    struct TASK *new_task, 
                *now_task = tl->tasks[tl->now];
    tl->now++;
    if (tl->now == tl->running) {
        tl->now = 0;
    }

    // task_run中要求本次切换时检查LEVEL是否也需要切换
    if (taskctl->lv_change != 0) {
        task_switchsub();
        tl = &taskctl->level[taskctl->now_lv];
    }

    new_task = tl->tasks[tl->now];
    timer_settime(task_timer, new_task->priority);
    if (new_task != now_task) {
        farjmp(0, new_task->sel);
    }

    return;
}

/*
    将指定的TASK休眠
    本质是将其从tasks[]数组中删去，这样task_switch时就不会被now指中，也不会被跳转
*/
void task_sleep(struct TASK *task) {
    struct TASK *now_task;
    // 指定任务处于运行状态，才需要进行休眠
    if (task->flags == 2) {
        now_task = task_now();  // 先找到当前正在运行的TASK
        task_remove(task);      // 从指定TASK所属LEVEL的tasks[]数组中删掉，并置flags为1
        if (task == now_task) { // 如果指定的TASK就是正在运行的
            // 即，自己让自己休眠的情形；此时需要进行任务切换
            task_switchsub();
            now_task = task_now();  // 找到当前应当执行的TASK
            farjmp(0, now_task->sel);
        }
    }

    return;
}

// 返回当前运行的TASK的地址
struct TASK *task_now(void) {
    struct TASKLEVEL *tl = &taskctl->level[taskctl->now_lv];    // 先确定哪个层级在运行
    return tl->tasks[tl->now];                                  // 再返回这个层级中正在运行的那个任务
}

// 向TASKCTL的tasks0[]和TASKLEVEL的*tasks[]中添加任务
void task_add(struct TASK *task) {
    struct TASKLEVEL *tl = &taskctl->level[task->level];
    if (tl->running >= MAX_TASKS_LV) {
        return;
    }
    tl->tasks[tl->running] = task;
    tl->running++;
    task->flags = 2;    // 置为正在运行
    return;
}

// 从TASKLEVEL中删除指定任务
void task_remove(struct TASK *task) {
    int i;
    struct TASKLEVEL *tl = &taskctl->level[task->level];

    // 寻找指定task所在位置
    for (i = 0; i < tl->running; i++) {
        if (tl->tasks[i] == task) {
            break;
        }
    }

    tl->running--;
    if (i < tl->now) {
        tl->now--;
    }
    if (tl->now >= tl->running) {
        tl->now = 0;    // 修正错误的now值
    }
    task->flags = 1;    // 置为休眠

    // 从LEVEL的tasks[]中删除
    for (; i < tl->running; i++) {
        tl->tasks[i] = tl->tasks[i + 1];
    }

    return;
}

// 切换LEVEL的函数
void task_switchsub(void) {
    int i;
    // 寻找最上层的、有任务在运行的LEVEL
    for (i = 0; i < MAX_TASKLEVELS; i++) {
        if (taskctl->level[i].running > 0) {
            break;
        }
    }
    taskctl->now_lv = i;    // 切换当前lv
    taskctl->lv_change = 0; // 将切换标志置回
    
    return;
}

// 只执行io_hlt的"空闲任务"的入口函数，在所有任务都休眠时执行 
void task_idle(void) {
    for (;;) {
        io_hlt();
    }

    return;
}
