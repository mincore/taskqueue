taskqueue
==================

features
------------------

- support multithread
- support timer

usage
------------------

create a taskqueue with N thread backend:
```c
const int thread_count = 2;
struct taskqueue *tq = taskqueue_create(thread_count);
```

declare your task like this:
```c
struct mytask {
    struct task task;
    int x;
    int y;
};
```

declare a callback function to handle mytask:
```c
void process_mytask(struct task *ptr)
{
    struct mytask *t = container_of(ptr, struct mytask, task);
    // do some thing
}
```

queue a task to the taskqueue:
```c
taskqueue_enqueue(tq, &mytask->task, process_mytask);
```

for timer, see test_taskqueue.c

