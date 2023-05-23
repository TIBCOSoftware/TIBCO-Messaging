/*
 * Copyright (c) 2020 Cloud Software Group, Inc.
 * Licensed under a BSD-style license. Refer to [LICENSE]
 *
 * $Id$
 *
 */

#include <stdlib.h>
#include <errno.h>

#include "thread.h"

void thread_start(thread_t *thread, thread_func_t thread_func, void* data)
{
#if defined(_WIN32)
    HANDLE thr = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)thread_func, data, 0, NULL);
    if (thr != NULL) {
        *thread = thr;
    }
#else
    pthread_create(thread, NULL, thread_func, data);
#endif
}

void thread_destroy(thread_t thread)
{
#if defined(_WIN32)
    CloseHandle(thread);
#endif
}

void thread_join(thread_t thread)
{
#if defined(_WIN32)
    WaitForSingleObject(thread, INFINITE);
#else
    pthread_join(thread, NULL);
#endif
}

void thread_detach(thread_t thread)
{
#if defined(_WIN32)
    CloseHandle(thread);
#else
    pthread_detach(thread);
#endif
}

void mutex_init(mutex_t* mutex)
{
#if defined(_WIN32)
    InitializeCriticalSection(mutex);
#else
    pthread_mutexattr_t attr;
    pthread_mutexattr_init(&attr);
    pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE);
    pthread_mutex_init(mutex, &attr);
#endif
}

void mutex_destroy(mutex_t* mutex)
{
#if defined(_WIN32)
    DeleteCriticalSection(mutex);
#else
    pthread_mutex_destroy(mutex);
#endif
}

void mutex_lock(mutex_t* mutex)
{
#if defined(_WIN32)
    EnterCriticalSection(mutex);
#else
    pthread_mutex_lock(mutex);
#endif
}

void mutex_unlock(mutex_t* mutex)
{
#if defined(_WIN32)
    LeaveCriticalSection(mutex);
#else
    pthread_mutex_unlock(mutex);
#endif
}

void thread_once(thread_once_t* once_control, void (*init_routine)(void))
{
#if defined(_WIN32)
    while (InterlockedExchange(&(once_control->lock), 1) != 0)
    {
        Sleep(1);
    }
    if (!once_control->state)
    {
        once_control->state = 1;
        init_routine();
    }
    InterlockedExchange(&(once_control->lock), 0);
#else
    pthread_once(once_control, init_routine);
#endif
}

semaphore_t semaphore_create(void)
{
    semaphore_t semaphore;
#if defined(_WIN32)
    semaphore = CreateSemaphore(NULL, 0, LONG_MAX, NULL);
#elif defined(__APPLE__)
    semaphore = dispatch_semaphore_create(0L);
#else
    semaphore = malloc(sizeof(sem_t));
    sem_init(semaphore, 0, 0);
#endif
    return semaphore;
}

void semaphore_destroy(semaphore_t semaphore)
{
#if defined(_WIN32)
    CloseHandle(semaphore);
#elif defined(__APPLE__)
    dispatch_release(semaphore);
#else
    sem_destroy(semaphore);
    free(semaphore);
#endif
}

int semaphore_wait(semaphore_t semaphore, int64_t timeout)
{
#if defined(_WIN32)
    if (timeout == WAIT_FOREVER)
        return WaitForSingleObject(semaphore, INFINITE);
    else
        return WaitForSingleObject(semaphore, timeout);
#elif defined(__APPLE__)
    if (timeout == WAIT_FOREVER)
        return dispatch_semaphore_wait(semaphore, DISPATCH_TIME_FOREVER);
    else
        return dispatch_semaphore_wait(semaphore, dispatch_time(DISPATCH_TIME_NOW, timeout*1000000L));
#else
    if (timeout == WAIT_FOREVER)
    {
        int rc;
        while ((rc = sem_wait(semaphore)) == -1 && errno == EINTR)
            continue;
        return rc;
    }
    else
    {
        int rc;
        struct timespec ts;
        clock_gettime(CLOCK_REALTIME, &ts);
        ts.tv_sec += timeout / 1000;
        ts.tv_nsec += (timeout % 1000) * 1000000;
        if (ts.tv_nsec >= 1000000000)
        {
            ts.tv_nsec -= 1000000000;
            ts.tv_sec += 1;
        }
        else if (ts.tv_nsec < 0)
        {
            ts.tv_nsec += 1000000000;
            ts.tv_sec -= 1;
        }
        while ((rc = sem_timedwait(semaphore, &ts)) == -1 && errno == EINTR)
            continue;
        return rc;
    }
#endif
}

void semaphore_post(semaphore_t semaphore)
{
#if defined(_WIN32)
    ReleaseSemaphore(semaphore, 1, NULL);
#elif defined(__APPLE__)
    dispatch_semaphore_signal(semaphore);
#else
    sem_post(semaphore);
#endif
}
