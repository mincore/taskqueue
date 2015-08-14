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

#ifndef TASKQUEUE_H
#define TASKQUEUE_H

#include <bsd/sys/tree.h>

#ifndef container_of
#define container_of(ptr, type, member) \
        (type *)((char *)(ptr) - (char *) &((type *)0)->member)
#endif

#define TASK_PRIVATE struct task *next;

#define DELAY_TASK_PRIVATE         \
    RB_ENTRY(delay_task) rb_entry; \
    unsigned long long delay;      \
    unsigned long long id;

struct task;
struct delay_task;

typedef void (*task_func_t)(struct task *);
struct task {
    task_func_t func;
    TASK_PRIVATE
};

typedef void (*delay_task_func_t)(struct delay_task *);
struct delay_task {
    delay_task_func_t func;
    DELAY_TASK_PRIVATE
};

struct taskqueue *taskqueue_create(int thread_count);
struct task *taskqueue_destroy(struct taskqueue *);
int taskqueue_enqueue(struct taskqueue *, struct task *, task_func_t);
int taskqueue_enqueue_timeout(struct taskqueue *, struct delay_task*,
        delay_task_func_t, unsigned long long delay);

#endif
