/*
 * Copyright (C) 2014, shuangping chen <mincore@163.com>
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include "taskqueue.h"

#define TASK_COUNT 20000000
#define DELAY_TASK_COUNT 20

struct mytask {
    struct task task;
    int index;
};

struct mydelaytask {
    struct delay_task delay_task;
    int index;
};

struct mytask g_task[TASK_COUNT];
struct mydelaytask g_delay_task[DELAY_TASK_COUNT];
int g_run = 0;
int g_delay_run = 0;

static void task_func(struct task *task)
{
    __sync_fetch_and_add(&g_run, 1);
}

static void delay_task_func(struct delay_task *delay_task)
{
    __sync_fetch_and_add(&g_delay_run, 1);
    struct mydelaytask *d = container_of(delay_task, struct mydelaytask, delay_task);
    printf("[%s] %d\n", __func__, d->index);
}

int main(int argc, char *argv[])
{
    struct taskqueue *tq = taskqueue_create(2);
    srand(time(NULL));

    for (int i=0; i<TASK_COUNT; i++) {
        struct mytask *mytask = &g_task[i];
        mytask->index = i;
        taskqueue_enqueue(tq, &mytask->task, task_func);
    }

    for (int i=0; i<DELAY_TASK_COUNT; i++) {
        struct mydelaytask *d = &g_delay_task[i];
        d->index = i;
        taskqueue_enqueue_timeout(tq, &d->delay_task, delay_task_func, i*10);
    }

    printf("enqueue done\n");
    sleep(3);

    struct task *left = taskqueue_destroy(tq);
    printf("task left:%p, task runned:%d, delay_task runned:%d\n",
            left, g_run, g_delay_run);

    return 0;
}
