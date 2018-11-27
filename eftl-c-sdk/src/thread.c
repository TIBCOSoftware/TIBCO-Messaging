/*
 * Copyright (c) 2018: 2018-04-09 22:46:40 -0500 (Mon, 09 Apr 2018) $ TIBCO Software Inc.
 * Licensed under a BSD-style license. Refer to [LICENSE]
 * For more information, please contact:
 * TIBCO Software Inc., Palo Alto, California, USA
 *
 * $Id: thread.c 100869 2018-04-10 03:46:40Z bpeterse $
 *
 */

#include <stdlib.h>

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
    pthread_mutex_init(mutex, NULL);
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
    semaphore = CreateEvent(NULL, FALSE, FALSE, NULL);
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

int semaphore_wait(semaphore_t semaphore, int timeout)
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
        return dispatch_semaphore_wait(semaphore, dispatch_time(DISPATCH_TIME_NOW, (int64_t)timeout*1000000L));
#else
    if (timeout == WAIT_FOREVER)
    {
        return sem_wait(semaphore);
    }
    else
    {
        struct timespec ts;
        clock_gettime(CLOCK_REALTIME, &ts);
        ts.tv_sec += timeout / 1000;
        ts.tv_nsec += (timeout % 1000) * 1000000;
        return sem_timedwait(semaphore, &ts);
    }
#endif
}

void semaphore_post(semaphore_t semaphore)
{
#if defined(_WIN32)
    SetEvent(semaphore);
#elif defined(__APPLE__)
    dispatch_semaphore_signal(semaphore);
#else
    sem_post(semaphore);
#endif
}
