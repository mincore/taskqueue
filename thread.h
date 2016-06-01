#ifndef THREAD_H
#define THREAD_H

#ifdef WINDOWS
#include <windows.h>
#include <sys/time.h>

typedef HANDLE pthread_t;
typedef void pthread_attr_t;
typedef void pthread_mutexattr_t;
typedef void pthread_condattr_t;
typedef CRITICAL_SECTION pthread_mutex_t;
typedef CONDITION_VARIABLE pthread_cond_t;

struct thread_func_t {
    void *(*run) (void *);
    void *arg;
};

static DWORD thread_func(PVOID p)
{
    struct thread_func_t *f = (struct thread_func_t *)p;
    f->run(f->arg);
    return 0;
}

static inline int pthread_create(pthread_t *thread, const pthread_attr_t *attr, void *(*start_routine) (void *), void *arg)
{
    DWORD id;
    struct thread_func_t p = {start_routine, arg};
    *thread = CreateThread (NULL, 0, thread_func, &p, 0, &id);
    return 0;
}

static inline int pthread_join(pthread_t thread, void **retval)
{
    WaitForSingleObject(thread, INFINITE);
    *retval = 0;
    return 0;
}

static inline int pthread_mutex_init(pthread_mutex_t *restrict mutex, const pthread_mutexattr_t *restrict attr)
{
    attr;
    InitializeCriticalSection(mutex);
    return 0;
}

static inline int pthread_mutex_destroy(pthread_mutex_t *mutex)
{
    mutex;
    return 0;
}

static inline int pthread_cond_init(pthread_cond_t *restrict cond, const pthread_condattr_t *restrict attr)
{
    attr;
    InitializeConditionVariable(cond);
    return 0;
}

static inline int pthread_cond_destroy(pthread_cond_t *cond)
{
    cond;
    return 0;
}

static inline int pthread_cond_signal(pthread_cond_t *cond)
{
    WakeConditionVariable(cond);
}

struct timespec {
    long tv_sec;
    long tv_nsec;
};

int gettimeofday(struct timeval *restrict tp, void *restrict tzp)
{
    time_t clock;
    struct tm tm;
    SYSTEMTIME wtm;

    GetLocalTime(&wtm);
    tm.tm_year  = wtm.wYear - 1900;
    tm.tm_mon   = wtm.wMonth - 1;
    tm.tm_mday  = wtm.wDay;
    tm.tm_hour  = wtm.wHour;
    tm.tm_min   = wtm.wMinute;
    tm.tm_sec   = wtm.wSecond;
    tm.tm_isdst = -1;
    clock       = mktime(&tm);
    tp->tv_sec  = clock;
    tp->tv_usec = wtm.wMilliseconds * 1000;
    return 0;
}

static inline int pthread_cond_timedwait(pthread_cond_t *restrict cond, pthread_mutex_t *restrict mutex, const struct timespec *restrict abstime)
{
    DWORD time = ;
    SleepConditionVariableCS(cond, mutex, time);
    return 0;
}

#else
#include <pthread.h>
#endif

#endif
