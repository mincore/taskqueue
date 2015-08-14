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

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdint.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/time.h>
#include <time.h>
#include <pthread.h>
#include <assert.h>
#include <errno.h>

#include "taskqueue.h"

#define atomic_set(p, v)    (*(p) = (v))
#define atomic_read(p)      (*(p))

#define MAX_WAKEUP_TIME 3600000 // 1 hour

struct thread {
    pthread_t pthread;
    pthread_cond_t cond;
    struct taskqueue *tq;
    int sleeping;
    struct delay_task *delay_cache;
};

struct taskqueue {
    struct task *head;
    struct task **ptail;
    pthread_mutex_t task_list_lock;
    struct thread *threads;
    int thread_count;
    int quit;
    RB_HEAD(delay_task_tree, delay_task) delay_task_tree;
    pthread_mutex_t delay_task_tree_lock;
    unsigned long long delay_task_id;
};

static unsigned long long now()
{
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return ((unsigned long long)tv.tv_sec)*1000 + tv.tv_usec/1000;
}

static void timespec_init(struct timespec *ts, unsigned long long ms)
{
    ts->tv_sec = ms / 1000;
    ts->tv_nsec = (ms % 1000 ) * 1000 * 1000ULL;
}

static int delay_task_compare(struct delay_task *a, struct delay_task *b)
{
    if (a->delay != b->delay)
        return a->delay - b->delay;
    return a->id - b->id;
}

RB_GENERATE(delay_task_tree, delay_task, rb_entry, delay_task_compare);

static void wakeup_one(struct taskqueue *tq)
{
    for (int i = 0; i < tq->thread_count; i++) {
        if (atomic_read(&tq->threads[i].sleeping)) {
            pthread_cond_signal(&tq->threads[i].cond);
            break;
        }
    }
}

static void wakeup_all(struct taskqueue *tq)
{
    for (int i = 0; i < tq->thread_count; i++) {
        pthread_cond_signal(&tq->threads[i].cond);
    }
}

static struct task *taskqueue_dequeue(struct taskqueue *tq)
{
    struct task *task;

    pthread_mutex_lock(&tq->task_list_lock);
    task = tq->head;
    if (tq->head) {
        tq->head = tq->head->next;
        if (tq->head == NULL)
            tq->ptail = &tq->head;
    }
    pthread_mutex_unlock(&tq->task_list_lock);

    return task;
}

static int thread_should_quit(struct thread *thread)
{
    return atomic_read(&thread->tq->quit);
}

static void thread_wait(struct thread *thread)
{
    struct taskqueue *tq = thread->tq;
    struct timespec ts;
    unsigned long long delay = 0;

    pthread_mutex_lock(&tq->task_list_lock);
    if (tq->head == NULL) {
        if (thread->delay_cache == NULL)
            delay = now() + MAX_WAKEUP_TIME;
        else if (thread->delay_cache->delay > now())
            delay = thread->delay_cache->delay;

        if (delay) {
            timespec_init(&ts, delay);
            atomic_set(&thread->sleeping, 1);
            pthread_cond_timedwait(&thread->cond, &tq->task_list_lock, &ts);
            atomic_set(&thread->sleeping, 0);
        }
    }
    pthread_mutex_unlock(&tq->task_list_lock);
}

static void thread_run_delay_task(struct thread *thread)
{
    struct taskqueue *tq = thread->tq;
    struct delay_task *delay_task;

    thread->delay_cache = NULL;

    pthread_mutex_lock(&tq->delay_task_tree_lock);
    while (!thread_should_quit(thread)) {
        delay_task = RB_MIN(delay_task_tree, &tq->delay_task_tree);
        if (!delay_task || delay_task->delay > now()) {
            thread->delay_cache = delay_task;
            break;
        }

        RB_REMOVE(delay_task_tree, &tq->delay_task_tree, delay_task);
        pthread_mutex_unlock(&tq->delay_task_tree_lock);
        delay_task->func(delay_task);
        pthread_mutex_lock(&tq->delay_task_tree_lock);
    }
    pthread_mutex_unlock(&tq->delay_task_tree_lock);
}

static void thread_run_task(struct thread *thread)
{
    struct taskqueue *tq = thread->tq;
    struct task *task;

    task = taskqueue_dequeue(tq);
    if (task)
        task->func(task);
}

static void *thread_loop(void *p)
{
    struct thread *thread = p;

    while (!thread_should_quit(thread)) {
        thread_run_task(thread);
        thread_run_delay_task(thread);
        thread_wait(thread);
    }

    return NULL;
}

struct taskqueue *taskqueue_create(int thread_count)
{
    struct taskqueue *tq;

    tq = malloc(sizeof(struct taskqueue));
    memset(tq, 0, sizeof(struct taskqueue));
    pthread_mutex_init(&tq->task_list_lock, NULL);
    pthread_mutex_init(&tq->delay_task_tree_lock, NULL);
    tq->ptail = &tq->head;

    tq->thread_count = thread_count <= 0 ? 1 : thread_count;
    tq->threads = malloc(sizeof(struct thread) * thread_count);
    memset(tq->threads, 0, sizeof(struct thread) * thread_count);

    for (int i = 0; i < thread_count; i++) {
        tq->threads[i].tq = tq;
        pthread_cond_init(&tq->threads[i].cond, NULL);
        pthread_create(&tq->threads[i].pthread, NULL, thread_loop, &tq->threads[i]);
    }

    return tq;
}

struct task *taskqueue_destroy(struct taskqueue *tq)
{
    struct task *ret;

    if (tq == NULL)
        return NULL;

    atomic_set(&tq->quit, 1);
    wakeup_all(tq);
    for (int i = 0; i < tq->thread_count; i++) {
        pthread_join(tq->threads[i].pthread, NULL);
        pthread_cond_destroy(&tq->threads[i].cond);
    }
    free(tq->threads);
    pthread_mutex_destroy(&tq->task_list_lock);
    ret = tq->head;
    free(tq);

    return ret;
}

int taskqueue_enqueue(struct taskqueue *tq, struct task *task, task_func_t func)
{
    if (tq == NULL || task == NULL)
        return -1;

    task->func = func;
    task->next = NULL;

    pthread_mutex_lock(&tq->task_list_lock);
    *tq->ptail = task;
    tq->ptail = &task->next;
    wakeup_one(tq);
    pthread_mutex_unlock(&tq->task_list_lock);

    return 0;
}

int taskqueue_enqueue_timeout(struct taskqueue *tq, struct delay_task* delay_task,
        delay_task_func_t func, unsigned long long delay)
{
    if (tq == NULL || delay_task == NULL)
        return -1;

    delay_task->func = func;

    pthread_mutex_lock(&tq->delay_task_tree_lock);
    delay_task->delay = now() + delay;
    delay_task->id = tq->delay_task_id++;
    RB_INSERT(delay_task_tree, &tq->delay_task_tree, delay_task);
    wakeup_one(tq);
    pthread_mutex_unlock(&tq->delay_task_tree_lock);

    return 0;
}
